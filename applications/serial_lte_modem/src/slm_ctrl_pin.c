/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/kernel.h>
#include <assert.h>
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_regulators.h>
#include <zephyr/sys/reboot.h>
#include "slm_at_host.h"
#include "slm_defines.h"
#include "slm_util.h"
#include "slm_ctrl_pin.h"

LOG_MODULE_REGISTER(slm_ctrl_pin, CONFIG_SLM_LOG_LEVEL);

#if POWER_PIN_IS_ENABLED

static const struct gpio_dt_spec power_pin_member =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(slm_gpio_pins), power_gpios, {0});
static const struct gpio_dt_spec indicate_pin_member =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(slm_gpio_pins), indicate_gpios, {0});
static const int power_pin_debounce_ms =
	DT_PROP(DT_NODELABEL(slm_gpio_pins), power_gpios_debounce_time_ms);
static const int indicate_pin_time_ms =
	DT_PROP(DT_NODELABEL(slm_gpio_pins), indicate_gpios_active_time_ms);

#endif

#if POWER_PIN_IS_ENABLED
static struct gpio_callback gpio_cb;
static struct k_work_delayable indicate_work;
static atomic_t callback_wakeup_running;
#else
BUILD_ASSERT(!IS_ENABLED(CONFIG_SLM_START_SLEEP),
	"CONFIG_SLM_START_SLEEP requires slm_gpio_pins to be defined in devicetree.");
#endif

static int ext_xtal_control(bool xtal_on)
{
	int err = 0;
#if defined(CONFIG_SLM_EXTERNAL_XTAL)
	static struct onoff_manager *clk_mgr;

	if (xtal_on) {
		struct onoff_client cli = {};

		/* request external XTAL for UART */
		clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
		sys_notify_init_spinwait(&cli.notify);
		err = onoff_request(clk_mgr, &cli);
		if (err < 0) {
			LOG_ERR("Clock request failed: %d", err);
			return err;
		}
		while (sys_notify_fetch_result(&cli.notify, &err) < 0) {
			/*empty*/
		}
	} else {
		/* release external XTAL for UART */
		err = onoff_release(clk_mgr);
		if (err < 0) {
			LOG_ERR("Clock release failed: %d", err);
			return err;
		}
	}
#endif

	return err;
}

#if POWER_PIN_IS_ENABLED

static int configure_power_pin_interrupt(gpio_callback_handler_t handler, gpio_flags_t flags)
{
	int err;

	/* First disable the previously configured interrupt. Somehow when in idle mode if
	 * the wake-up interrupt is configured to be on an edge the power consumption
	 * drastically increases (3x), which is why it is configured to be level-triggered.
	 * When entering idle for some reason first disabling the previously edge-level
	 * configured interrupt is also needed to keep the power consumption down.
	 */
	err = gpio_pin_interrupt_configure_dt(&power_pin_member, GPIO_INT_DISABLE);
	if (err) {
		LOG_ERR("Failed to configure %s (0x%x) on power pin. (%d)",
			"interrupt", GPIO_INT_DISABLE, err);
	}

	err = gpio_pin_interrupt_configure_dt(&power_pin_member, flags);
	if (err) {
		LOG_ERR("Failed to configure %s (0x%x) on power pin. (%d)",
			"interrupt", flags, err);
		return err;
	}

	gpio_init_callback(&gpio_cb, handler, BIT(power_pin_member.pin));

	err = gpio_add_callback_dt(&power_pin_member, &gpio_cb);
	if (err) {
		LOG_ERR("Failed to configure %s (0x%x) on power pin. (%d)", "callback", flags, err);
		return err;
	}

	LOG_DBG("Configured interrupt (0x%x) on power pin (%u) with handler (%p).",
		flags, power_pin_member.pin, (void *)handler);
	return 0;
}

static void slm_ctrl_pin_enter_sleep_work_fn(struct k_work *)
{
	slm_ctrl_pin_enter_sleep();
}

static void power_pin_callback_poweroff(const struct device *dev,
					struct gpio_callback *gpio_callback, uint32_t)
{
	static K_WORK_DEFINE(work, slm_ctrl_pin_enter_sleep_work_fn);

	LOG_INF("Power off triggered.");
	gpio_remove_callback_dt(&power_pin_member, gpio_callback);
	k_work_submit(&work);
}

#endif /* POWER_PIN_IS_ENABLED */

#if (INDICATE_PIN_IS_ENABLED)

static void indicate_stop(void)
{
#if (INDICATE_PIN_IS_ENABLED)
	if (gpio_pin_set_dt(&indicate_pin_member, 0) != 0) {
		LOG_WRN("GPIO_0 set error");
	}
	LOG_DBG("Stop indicating");
#endif
}

static void indicate_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	indicate_stop();
}

#endif

#if POWER_PIN_IS_ENABLED

static void power_pin_callback_enable_poweroff_fn(struct k_work *)
{
	LOG_INF("Enabling poweroff interrupt.");
	configure_power_pin_interrupt(power_pin_callback_poweroff, GPIO_INT_EDGE_RISING);
}

static K_WORK_DELAYABLE_DEFINE(work_poweroff, power_pin_callback_enable_poweroff_fn);

static void power_pin_callback_enable_poweroff(const struct device *dev,
					       struct gpio_callback *gpio_callback, uint32_t)
{
	LOG_INF("Enabling the poweroff interrupt after debounce time %d...", power_pin_debounce_ms);
	gpio_remove_callback_dt(&power_pin_member, gpio_callback);

	k_work_reschedule(&work_poweroff, K_MSEC(power_pin_debounce_ms));
}

static void power_pin_callback_wakeup_work_fn(struct k_work *)
{
	int err;

	LOG_INF("Resuming from idle.");
	if (k_work_delayable_is_pending(&indicate_work)) {
		k_work_cancel_delayable(&indicate_work);
		indicate_stop();
	}

	err = ext_xtal_control(true);
	if (err < 0) {
		LOG_WRN("Failed to enable ext XTAL: %d", err);
	}
	err = slm_at_host_power_on();
	if (err) {
		LOG_ERR("Failed to power on uart: %d. Resetting SLM.", err);
		gpio_remove_callback_dt(&power_pin_member, &gpio_cb);
		slm_reset();
		return;
	}

	atomic_set(&callback_wakeup_running, false);
}

static void power_pin_callback_wakeup(const struct device *dev,
				      struct gpio_callback *gpio_callback, uint32_t)
{
	static K_WORK_DEFINE(work, power_pin_callback_wakeup_work_fn);

	/* Prevent level triggered interrupt running this multiple times. */
	if (!atomic_cas(&callback_wakeup_running, false, true)) {
		return;
	}

	LOG_INF("Resuming from idle shortly...");
	gpio_remove_callback_dt(&power_pin_member, gpio_callback);

	/* Enable the poweroff interrupt only when the pin will be back to a nonactive state. */
	configure_power_pin_interrupt(power_pin_callback_enable_poweroff, GPIO_INT_EDGE_FALLING);

	k_work_submit(&work);
}

int slm_ctrl_pin_indicate(void)
{
	int err = 0;

#if (INDICATE_PIN_IS_ENABLED)
	if (k_work_delayable_is_pending(&indicate_work)) {
		return 0;
	}
	LOG_DBG("Start indicating for %d ms", indicate_pin_time_ms);
	err = gpio_pin_set_dt(&indicate_pin_member, 1);
	if (err) {
		LOG_ERR("GPIO_0 set error: %d", err);
	} else {
		k_work_reschedule(&indicate_work, K_MSEC(indicate_pin_time_ms));
	}
#endif
	return err;
}

void slm_ctrl_pin_enter_idle(void)
{
	LOG_INF("Entering idle.");
	int err;

	gpio_remove_callback_dt(&power_pin_member, &gpio_cb);

	err = configure_power_pin_interrupt(power_pin_callback_wakeup, GPIO_INT_LEVEL_LOW);
	if (err) {
		return;
	}

	err = ext_xtal_control(false);
	if (err < 0) {
		LOG_WRN("Failed to disable ext XTAL: %d", err);
	}
}

void slm_ctrl_pin_enter_sleep(void)
{
	slm_at_host_uninit();

	/* Only power off the modem if it has not been put
	 * in flight mode to allow reducing NVM wear.
	 */
	if (!slm_is_modem_functional_mode(LTE_LC_FUNC_MODE_OFFLINE)) {
		slm_power_off_modem();
	}
	slm_ctrl_pin_enter_sleep_no_uninit();
}

void slm_ctrl_pin_enter_sleep_no_uninit(void)
{
	LOG_INF("Entering sleep.");
	LOG_PANIC();
	nrf_gpio_cfg_sense_set(power_pin_member.pin, NRF_GPIO_PIN_SENSE_LOW);

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
	assert(false);
}

#endif /* POWER_PIN_IS_ENABLED */

void slm_ctrl_pin_enter_shutdown(void)
{
	LOG_INF("Entering shutdown.");

	/* De-configure GPIOs */
#if POWER_PIN_IS_ENABLED
	gpio_pin_interrupt_configure_dt(&power_pin_member, GPIO_INT_DISABLE);
	gpio_pin_configure_dt(&power_pin_member, GPIO_DISCONNECTED);
#endif
#if INDICATE_PIN_IS_ENABLED
	gpio_pin_configure_dt(&indicate_pin_member, GPIO_DISCONNECTED);
#endif

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
	assert(false);
}

void slm_ctrl_pin_init_gpios(void)
{
#if POWER_PIN_IS_ENABLED
	if (!gpio_is_ready_dt(&power_pin_member)) {
		LOG_ERR("GPIO controller not ready");
		return;
	}

	LOG_WRN("power_pin_member %d", power_pin_member.pin);
	/* TODO: Flags into DTS */
	gpio_pin_configure_dt(&power_pin_member, GPIO_INPUT | GPIO_PULL_UP);
#endif

#if INDICATE_PIN_IS_ENABLED
	LOG_WRN("indicate_pin_member %d", indicate_pin_member.pin);
	gpio_pin_configure_dt(&indicate_pin_member, GPIO_OUTPUT_INACTIVE);
#endif
}

int slm_ctrl_pin_init(void)
{
	int err;

	err = ext_xtal_control(true);
	if (err < 0) {
		LOG_ERR("Failed to enable ext XTAL: %d", err);
		return err;
	}
#if POWER_PIN_IS_ENABLED
	k_work_init_delayable(&indicate_work, indicate_wk);

	/* Do not directly enable the poweroff interrupt so that only a full toggle triggers
	 * power off. This is because power on is triggered on low level, so if the pin is held
	 * down until SLM is fully initialized releasing it would directly trigger the power off.
	 */
	err = configure_power_pin_interrupt(power_pin_callback_enable_poweroff,
					    GPIO_INT_EDGE_FALLING);
#endif
	return err;
}

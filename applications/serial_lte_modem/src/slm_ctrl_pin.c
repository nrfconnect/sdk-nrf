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

#define POWER_PIN_DEBOUNCE_MS 50

static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

#if POWER_PIN_IS_ENABLED
static struct gpio_callback gpio_cb;
#else
BUILD_ASSERT(!IS_ENABLED(CONFIG_SLM_START_SLEEP),
	"CONFIG_SLM_START_SLEEP requires CONFIG_SLM_POWER_PIN to be defined.");
#endif

#if INDICATE_PIN_IS_ENABLED
static struct k_work_delayable indicate_work;
#endif
#if POWER_PIN_IS_ENABLED
static atomic_t callback_wakeup_running;
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

#if POWER_PIN_IS_ENABLED || INDICATE_PIN_IS_ENABLED

static int configure_gpio(gpio_pin_t pin, gpio_flags_t flags)
{
	const int err = gpio_pin_configure(gpio_dev, pin, flags);

	if (err) {
		LOG_ERR("Failed to configure GPIO pin P0.%d. (%d)", pin, err);
		return err;
	}

	return 0;
}
#endif

#if POWER_PIN_IS_ENABLED

static int configure_power_pin_interrupt(gpio_callback_handler_t handler, gpio_flags_t flags)
{
	int err;
	const gpio_pin_t pin = CONFIG_SLM_POWER_PIN;

	/* First disable the previously configured interrupt. Somehow when in idle mode if
	 * the wake-up interrupt is configured to be on an edge the power consumption
	 * drastically increases (3x), which is why it is configured to be level-triggered.
	 * When entering idle for some reason first disabling the previously edge-level
	 * configured interrupt is also needed to keep the power consumption down.
	 */
	err = gpio_pin_interrupt_configure(gpio_dev, pin, GPIO_INT_DISABLE);
	if (err) {
		LOG_ERR("Failed to configure %s (0x%lx) on power pin. (%d)",
			"interrupt", GPIO_INT_DISABLE, err);
	}

	err = gpio_pin_interrupt_configure(gpio_dev, pin, flags);
	if (err) {
		LOG_ERR("Failed to configure %s (0x%x) on power pin. (%d)",
			"interrupt", flags, err);
		return err;
	}

	gpio_init_callback(&gpio_cb, handler, BIT(pin));

	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		LOG_ERR("Failed to configure %s (0x%x) on power pin. (%d)", "callback", flags, err);
		return err;
	}

	LOG_DBG("Configured interrupt (0x%x) on power pin (%u) with handler (%p).",
		flags, pin, (void *)handler);
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
	gpio_remove_callback(dev, gpio_callback);
	k_work_submit(&work);
}

#endif /* POWER_PIN_IS_ENABLED */

#if INDICATE_PIN_IS_ENABLED

static void indicate_stop(void)
{
	if (gpio_pin_set(gpio_dev, CONFIG_SLM_INDICATE_PIN, 0) != 0) {
		LOG_WRN("GPIO_0 set error");
	}
	LOG_DBG("Stop indicating");
}

static void indicate_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	indicate_stop();
}

#endif /* INDICATE_PIN_IS_ENABLED */

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
	LOG_INF("Enabling the poweroff interrupt shortly...");
	gpio_remove_callback(dev, gpio_callback);

	k_work_reschedule(&work_poweroff, K_MSEC(POWER_PIN_DEBOUNCE_MS));
}

static void power_pin_callback_wakeup_work_fn(struct k_work *)
{
	int err;

	LOG_INF("Resuming from idle.");

#if INDICATE_PIN_IS_ENABLED
	if (k_work_delayable_is_pending(&indicate_work)) {
		k_work_cancel_delayable(&indicate_work);
		indicate_stop();
	}
#endif /* INDICATE_PIN_IS_ENABLED */

	err = ext_xtal_control(true);
	if (err < 0) {
		LOG_WRN("Failed to enable ext XTAL: %d", err);
	}
	err = slm_at_host_power_on();
	if (err) {
		LOG_ERR("Failed to power on uart: %d. Resetting SLM.", err);
		gpio_remove_callback(gpio_dev, &gpio_cb);
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
	gpio_remove_callback(dev, gpio_callback);

	/* Enable the poweroff interrupt only when the pin will be back to a nonactive state. */
	configure_power_pin_interrupt(power_pin_callback_enable_poweroff, GPIO_INT_EDGE_FALLING);

	k_work_submit(&work);
}

void slm_ctrl_pin_enter_idle(void)
{
	LOG_INF("Entering idle.");
	int err;

	gpio_remove_callback(gpio_dev, &gpio_cb);

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
	nrf_gpio_cfg_sense_set(CONFIG_SLM_POWER_PIN, NRF_GPIO_PIN_SENSE_LOW);

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
	assert(false);
}

#endif /* POWER_PIN_IS_ENABLED */

int slm_ctrl_pin_indicate(void)
{
	int err = 0;

#if INDICATE_PIN_IS_ENABLED
	if (k_work_delayable_is_pending(&indicate_work)) {
		return 0;
	}
	LOG_DBG("Start indicating");
	err = gpio_pin_set(gpio_dev, CONFIG_SLM_INDICATE_PIN, 1);
	if (err) {
		LOG_ERR("GPIO_0 set error: %d", err);
	} else {
		k_work_reschedule(&indicate_work, K_MSEC(CONFIG_SLM_INDICATE_TIME));
	}
#endif
	return err;
}

void slm_ctrl_pin_enter_shutdown(void)
{
	LOG_INF("Entering shutdown.");

	/* De-configure GPIOs */
#if POWER_PIN_IS_ENABLED
	gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_POWER_PIN, GPIO_INT_DISABLE);
	gpio_pin_configure(gpio_dev, CONFIG_SLM_POWER_PIN, GPIO_DISCONNECTED);
#endif
#if INDICATE_PIN_IS_ENABLED
	gpio_pin_configure(gpio_dev, CONFIG_SLM_INDICATE_PIN, GPIO_DISCONNECTED);
#endif

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
	assert(false);
}

void slm_ctrl_pin_init_gpios(void)
{
	if (!device_is_ready(gpio_dev)) {
		LOG_ERR("GPIO controller not ready");
		return;
	}

#if POWER_PIN_IS_ENABLED
	(void)configure_gpio(CONFIG_SLM_POWER_PIN, GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_LOW);
#endif

#if INDICATE_PIN_IS_ENABLED
	(void)configure_gpio(CONFIG_SLM_INDICATE_PIN, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
#endif
}

int slm_ctrl_pin_init(void)
{
	int err;

#if INDICATE_PIN_IS_ENABLED
	k_work_init_delayable(&indicate_work, indicate_wk);
#endif

	err = ext_xtal_control(true);
	if (err < 0) {
		LOG_ERR("Failed to enable ext XTAL: %d", err);
		return err;
	}
#if POWER_PIN_IS_ENABLED
	/* Do not directly enable the poweroff interrupt so that only a full toggle triggers
	 * power off. This is because power on is triggered on low level, so if the pin is held
	 * down until SLM is fully initialized releasing it would directly trigger the power off.
	 */
	err = configure_power_pin_interrupt(power_pin_callback_enable_poweroff,
					    GPIO_INT_EDGE_FALLING);
#endif
	return err;
}

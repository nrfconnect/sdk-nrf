/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/kernel.h>
#include <assert.h>
#include <stdio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <nrf_modem.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <net/fota_download.h>
#include "slm_at_host.h"
#include "slm_at_fota.h"
#include "slm_settings.h"
#include "slm_util.h"

LOG_MODULE_REGISTER(slm, CONFIG_SLM_LOG_LEVEL);

#define SLM_WQ_STACK_SIZE	KB(4)
#define SLM_WQ_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(slm_wq_stack_area, SLM_WQ_STACK_SIZE);

static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#if POWER_PIN_IS_ENABLED
static struct gpio_callback gpio_cb;
#else
BUILD_ASSERT(!IS_ENABLED(CONFIG_SLM_START_SLEEP),
	"CONFIG_SLM_START_SLEEP requires CONFIG_SLM_POWER_PIN to be defined.");
#endif
static struct k_work_delayable indicate_work;

struct k_work_q slm_work_q;

/* Forward declarations */
static void indicate_wk(struct k_work *work);

NRF_MODEM_LIB_ON_INIT(lwm2m_init_hook, on_modem_lib_init, NULL);
NRF_MODEM_LIB_ON_DFU_RES(main_dfu_hook, on_modem_dfu_res, NULL);

static void on_modem_lib_init(int ret, void *ctx)
{
	ARG_UNUSED(ctx);

	/** ret: Zero on success, a positive value @em nrf_modem_dfu when executing
	 *  Modem firmware updates, and negative errno on other failures.
	 */
	LOG_INF("lib_modem init: %d", ret);
}

#if defined(CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC)
static void on_modem_failure_shutdown(struct k_work *item);
static void on_modem_failure_reinit(struct k_work *item);

K_WORK_DELAYABLE_DEFINE(modem_failure_shutdown_work, on_modem_failure_shutdown);
K_WORK_DELAYABLE_DEFINE(modem_failure_reinit_work, on_modem_failure_reinit);

void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	rsp_send("\r\n#XMODEM: FAULT,0x%x,0x%x\r\n", fault_info->reason,
		fault_info->program_counter);
	/* For now we wait 10 ms to give the trace handler time to process trace data. */
	k_work_reschedule(&modem_failure_shutdown_work, K_MSEC(10));
}

static void on_modem_failure_shutdown(struct k_work *work)
{
	int ret = nrf_modem_lib_shutdown();

	ARG_UNUSED(work);
	rsp_send("\r\n#XMODEM: SHUTDOWN,%d\r\n", ret);
	k_work_reschedule(&modem_failure_reinit_work, K_MSEC(10));
}

static void on_modem_failure_reinit(struct k_work *work)
{
	int ret = nrf_modem_lib_init();

	ARG_UNUSED(work);
	rsp_send("\r\n#XMODEM: INIT,%d\r\n", ret);
}
#endif /* CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC */

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
	const gpio_pin_t pin = CONFIG_SLM_POWER_PIN;

	/* First disable the previously configured interrupt. Somehow when in idle mode if
	 * the wake-up interrupt is configured to be on an edge the power consumption
	 * drastically increases (3x), which is is why it is configured to be level-triggered.
	 * When entering idle for some reason first disabling the previously edge-level
	 * configured interrupt is also needed to keep the power consumption down.
	 */
	gpio_pin_interrupt_configure(gpio_dev, pin, GPIO_INT_DISABLE);

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

	LOG_DBG("Configured interrupt (0x%x) on power pin (%u).", flags, pin);
	return 0;
}

static void power_pin_callback_poweroff(const struct device *, struct gpio_callback *, uint32_t)
{
	LOG_INF("Power off triggered.");
	slm_enter_sleep();
}

static void poweroff_interrupt_enabler(struct k_work *)
{
	configure_power_pin_interrupt(power_pin_callback_poweroff, GPIO_INT_EDGE_RISING);
}

#endif /* POWER_PIN_IS_ENABLED */

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

static int init_gpios(void)
{
	if (!device_is_ready(gpio_dev)) {
		LOG_ERR("GPIO controller not ready");
		return -ENODEV;
	}

#if POWER_PIN_IS_ENABLED
	if (configure_gpio(CONFIG_SLM_POWER_PIN, GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_LOW)) {
		return -1;
	}
#endif

#if INDICATE_PIN_IS_ENABLED
	if (configure_gpio(CONFIG_SLM_INDICATE_PIN, GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW)) {
		return -1;
	}
#endif

	return 0;
}

int slm_indicate(void)
{
	int err = 0;

#if (INDICATE_PIN_IS_ENABLED)
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

static void indicate_stop(void)
{
#if (INDICATE_PIN_IS_ENABLED)
	if (gpio_pin_set(gpio_dev, CONFIG_SLM_INDICATE_PIN, 0) != 0) {
		LOG_WRN("GPIO_0 set error");
	}
	LOG_DBG("Stop indicating");
#endif
}

#if POWER_PIN_IS_ENABLED

static void power_pin_callback_enable_poweroff(const struct device *dev,
					       struct gpio_callback *gpio_callback, uint32_t)
{
	static K_WORK_DELAYABLE_DEFINE(work, poweroff_interrupt_enabler);

	LOG_DBG("Enabling the poweroff interrupt shortly...");
	gpio_remove_callback(dev, gpio_callback);

	/* Enable the poweroff interrupt after a small delay
	 * so that it doesn't fire right away (which it does if enabled here).
	 */
	k_work_schedule(&work, K_MSEC(1));
}

static void power_pin_callback_wakeup(const struct device *dev,
				      struct gpio_callback *gpio_callback, uint32_t)
{
	static atomic_t callback_running;
	int err;

	/* Prevent level triggered interrupt running this multiple times. */
	if (!atomic_cas(&callback_running, false, true)) {
		return;
	}

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
		atomic_set(&callback_running, false);
		LOG_ERR("Failed to power on uart: %d", err);
		return;
	}

	gpio_remove_callback(dev, gpio_callback);

	/* Enable the poweroff interrupt only when the pin will be back to a nonactive state. */
	configure_power_pin_interrupt(power_pin_callback_enable_poweroff, GPIO_INT_EDGE_RISING);

	atomic_set(&callback_running, false);
}

void slm_enter_idle(void)
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

static void enter_sleep_no_uninit(void)
{
	LOG_INF("Entering sleep.");
	LOG_PANIC();
	nrf_gpio_cfg_sense_set(CONFIG_SLM_POWER_PIN, NRF_GPIO_PIN_SENSE_LOW);

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
	assert(false);
}

void slm_enter_sleep(void)
{
	slm_at_host_uninit();

	/* Only power off the modem if it has not been put
	 * in flight mode to allow reducing NVM wear.
	 */
	if (!slm_is_modem_functional_mode(LTE_LC_FUNC_MODE_OFFLINE)) {
		slm_power_off_modem();
	}
	enter_sleep_no_uninit();
}

#endif /* POWER_PIN_IS_ENABLED */

void slm_enter_shutdown(void)
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

static void on_modem_dfu_res(int dfu_res, void *ctx)
{
	slm_fota_type = DFU_TARGET_IMAGE_TYPE_MODEM_DELTA;
	slm_fota_stage = FOTA_STAGE_COMPLETE;
	slm_fota_status = FOTA_STATUS_ERROR;
	slm_fota_info = dfu_res;

	switch (dfu_res) {
	case NRF_MODEM_DFU_RESULT_OK:
		LOG_INF("Modem update OK. Running new firmware.");
		slm_fota_status = FOTA_STATUS_OK;
		slm_fota_info = 0;
		break;
	case NRF_MODEM_DFU_RESULT_UUID_ERROR:
	case NRF_MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("Modem update failed (0x%x). Running old firmware.", dfu_res);
		break;
	case NRF_MODEM_DFU_RESULT_HARDWARE_ERROR:
	case NRF_MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("Fatal error (0x%x) encountered during modem update.", dfu_res);
		break;
	case NRF_MODEM_DFU_RESULT_VOLTAGE_LOW:
		LOG_ERR("Modem update postponed due to low voltage. "
			"Reset the modem once you have sufficient power.");
		slm_fota_stage = FOTA_STAGE_ACTIVATE;
		break;
	default:
		LOG_ERR("Unhandled nrf_modem DFU result code 0x%x.", dfu_res);
		break;
	}
}

static void check_app_fota_status(void)
{
	/** When a TEST image is swapped to primary partition and booted by MCUBOOT,
	 * the API mcuboot_swap_type() will return BOOT_SWAP_TYPE_REVERT. By this type
	 * MCUBOOT means that the TEST image is booted OK and, if it's not confirmed
	 * next, it'll be swapped back to secondary partition and original application
	 * image will be restored to the primary partition (so-called Revert).
	 */
	const int type = mcuboot_swap_type();

	switch (type) {
	/** Attempt to boot the contents of slot 0. */
	case BOOT_SWAP_TYPE_NONE:
		/* Normal reset, nothing happened, do nothing. */
		return;
	/** Swap to slot 1. Absent a confirm command, revert back on next boot. */
	case BOOT_SWAP_TYPE_TEST:
	/** Swap to slot 1, and permanently switch to booting its contents. */
	case BOOT_SWAP_TYPE_PERM:
	/** Swap failed because image to be run is not valid. */
	case BOOT_SWAP_TYPE_FAIL:
		slm_fota_status = FOTA_STATUS_ERROR;
		slm_fota_info = type;
		break;
	/** Swap back to alternate slot. A confirm changes this state to NONE. */
	case BOOT_SWAP_TYPE_REVERT:
		/* Happens on a successful application FOTA. */
		const int ret = boot_write_img_confirmed();

		slm_fota_info = ret;
		slm_fota_status = ret ? FOTA_STATUS_ERROR : FOTA_STATUS_OK;
		break;
	}
	slm_fota_type = DFU_TARGET_IMAGE_TYPE_MCUBOOT;
	slm_fota_stage = FOTA_STAGE_COMPLETE;
}

int lte_auto_connect(void)
{
	int err = 0;
#if defined(CONFIG_SLM_AUTO_CONNECT)
	int stat;
	struct network_config {
		/* Refer to AT command manual of %XSYSTEMMODE for system mode settings */
		int lte_m_support;     /* 0 ~ 1 */
		int nb_iot_support;    /* 0 ~ 1 */
		int gnss_support;      /* 0 ~ 1 */
		int lte_preference;    /* 0 ~ 4 */
		/* Refer to AT command manual of +CGDCONT and +CGAUTH for PDN configuration */
		bool pdp_config;       /* PDP context definition required or not */
		char *pdn_fam;         /* PDP type: "IP", "IPV6", "IPV4V6", "Non-IP" */
		char *pdn_apn;         /* Access point name */
		int pdn_auth;          /* PDN authentication protocol 0(None), 1(PAP), 2(CHAP) */
		char *pdn_username;    /* PDN connection authentication username */
		char *pdn_password;    /* PDN connection authentication password */
	};
	const struct network_config cfg = {
#include "slm_auto_connect.h"
	};

	err = slm_util_at_scanf("AT+CEREG?", "+CEREG: %d", &stat);
	if (err != 1 || (stat == 1 || stat == 5)) {
		return 0;
	}

	LOG_INF("lte auto connect");
	err = slm_util_at_printf("AT%%XSYSTEMMODE=%d,%d,%d,%d", cfg.lte_m_support,
				  cfg.nb_iot_support, cfg.gnss_support, cfg.lte_preference);
	if (err) {
		LOG_ERR("Failed to configure system mode: %d", err);
		return err;
	}
	if (cfg.pdp_config) {
		err = slm_util_at_printf("AT+CGDCONT=0,%s,%s", cfg.pdn_fam, cfg.pdn_apn);
		if (err) {
			LOG_ERR("Failed to configure PDN: %d", err);
			return err;
		}
	}
	if (cfg.pdp_config && cfg.pdn_auth != 0) {
		err = slm_util_at_printf("AT+CGAUTH=0,%d,%s,%s", cfg.pdn_auth,
					  cfg.pdn_username, cfg.pdn_password);
		if (err) {
			LOG_ERR("Failed to configure AUTH: %d", err);
			return err;
		}
	}
	err = slm_util_at_printf("AT+CFUN=1");
	if (err) {
		LOG_ERR("Failed to turn on radio: %d", err);
		return err;
	}
#endif /* CONFIG_SLM_AUTO_CONNECT */

	return err;
}

int start_execute(void)
{
	int err;

	LOG_INF("Serial LTE Modem");
	err = ext_xtal_control(true);
	if (err < 0) {
		LOG_ERR("Failed to enable ext XTAL: %d", err);
		return err;
	}
	err = slm_at_host_init();
	if (err) {
		LOG_ERR("Failed to init at_host: %d", err);
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
	if (err) {
		return err;
	}
	k_work_queue_start(&slm_work_q, slm_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(slm_wq_stack_area),
			   SLM_WQ_PRIORITY, NULL);
	(void)lte_auto_connect();

	return 0;
}

static void indicate_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	indicate_stop();
}

int main(void)
{
	const uint32_t rr = nrf_power_resetreas_get(NRF_POWER_NS);

	nrf_power_resetreas_clear(NRF_POWER_NS, 0x70017);
	LOG_DBG("RR: 0x%08x", rr);
	k_work_init_delayable(&indicate_work, indicate_wk);

	if (init_gpios() != 0) {
		LOG_WRN("Failed to init GPIO pins.");
	}
	/* Init and load settings */
	if (slm_settings_init() != 0) {
		LOG_WRN("Failed to init slm settings");
	}

#if defined(CONFIG_SLM_FULL_FOTA)
	if (slm_modem_full_fota) {
		slm_finish_modem_full_fota();
		slm_fota_type = DFU_TARGET_IMAGE_TYPE_FULL_MODEM;
	}
#endif

	const int ret = nrf_modem_lib_init();

	if (ret) {
		LOG_ERR("Modem library init failed, err: %d", ret);
		if (ret != -EAGAIN && ret != -EIO) {
			return ret;
		} else if (ret == -EIO) {
			LOG_ERR("Please program full modem firmware with the bootloader or "
				"external tools");
		}
	}

	check_app_fota_status();

#if defined(CONFIG_SLM_START_SLEEP)

	if (!(rr & NRF_POWER_RESETREAS_OFF_MASK)) { /* DETECT signal from GPIO */

		enter_sleep_no_uninit();
	}
#endif /* CONFIG_SLM_START_SLEEP */

	return start_execute();
}

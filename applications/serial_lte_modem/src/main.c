/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include <zephyr/kernel.h>
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
#include "slm_at_host.h"
#include "slm_at_fota.h"

LOG_MODULE_REGISTER(slm, CONFIG_SLM_LOG_LEVEL);

#define SLM_WQ_STACK_SIZE	KB(4)
#define SLM_WQ_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(slm_wq_stack_area, SLM_WQ_STACK_SIZE);

static const struct device *gpio_dev;
static struct gpio_callback gpio_cb;
static struct k_work_delayable indicate_work;

/* global variable used across different files */
struct k_work_q slm_work_q;

/* global variable defined in different files */
extern uint8_t fota_type;
extern uint8_t fota_stage;
extern uint8_t fota_status;
extern int32_t fota_info;

/* global functions defined in different files */
int poweron_uart(void);
int slm_settings_init(void);
int slm_setting_fota_save(void);

/* Forward declarations */
static void indicate_wk(struct k_work *work);

BUILD_ASSERT(CONFIG_SLM_WAKEUP_PIN >= 0, "Wake up pin not configured");

#if defined(CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC)
static void on_modem_failure_shutdown(struct k_work *item);
static void on_modem_failure_reinit(struct k_work *item);

K_WORK_DELAYABLE_DEFINE(modem_failure_shutdown_work, on_modem_failure_shutdown);
K_WORK_DELAYABLE_DEFINE(modem_failure_reinit_work, on_modem_failure_reinit);

void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	char rsp[64];

	sprintf(rsp, "#XMODEM: FAULT,0x%x,0x%x", fault_info->reason, fault_info->program_counter);
	rsp_send(rsp, strlen(rsp));
	/* For now we wait 10 ms to give the trace handler time to process trace data. */
	k_work_reschedule(&modem_failure_shutdown_work, K_MSEC(10));
}

static void on_modem_failure_shutdown(struct k_work *work)
{
	char rsp[32];
	int ret = nrf_modem_lib_shutdown();

	ARG_UNUSED(work);
	sprintf(rsp, "#XMODEM: SHUTDOWN,%d", ret);
	rsp_send(rsp, strlen(rsp));
	k_work_reschedule(&modem_failure_reinit_work, K_MSEC(10));
}

static void on_modem_failure_reinit(struct k_work *work)
{
	char rsp[32];
	int ret = nrf_modem_lib_init(NORMAL_MODE);

	ARG_UNUSED(work);
	sprintf(rsp, "#XMODEM: INIT,%d", ret);
	rsp_send(rsp, strlen(rsp));
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

int indicate_start(void)
{
	int err = 0;

#if (CONFIG_SLM_INDICATE_PIN >= 0)
	if (k_work_delayable_is_pending(&indicate_work)) {
		return 0;
	}
	LOG_DBG("Start indicating");
	gpio_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
	if (gpio_dev == NULL) {
		LOG_ERR("GPIO_0 bind error");
		return -EAGAIN;
	}
	err = gpio_pin_configure(gpio_dev, CONFIG_SLM_INDICATE_PIN, GPIO_OUTPUT);
	if (err) {
		LOG_ERR("GPIO_0 config error: %d", err);
		return -EAGAIN;
	}
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
#if (CONFIG_SLM_INDICATE_PIN >= 0)
	if (gpio_pin_set(gpio_dev, CONFIG_SLM_INDICATE_PIN, 0) != 0) {
		LOG_WRN("GPIO_0 set error");
	}
	gpio_pin_configure(gpio_dev, CONFIG_SLM_INDICATE_PIN, GPIO_DISCONNECTED);
	LOG_DBG("Stop indicating");
#endif
}

static void gpio_cb_func(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	int err;

	if ((BIT(CONFIG_SLM_WAKEUP_PIN) & pins) == 0) {
		return;
	}
	LOG_INF("Exit idle");
	if (k_work_delayable_is_pending(&indicate_work)) {
		(void)k_work_cancel_delayable(&indicate_work);
		indicate_stop();
	}

	err = ext_xtal_control(true);
	if (err < 0) {
		LOG_WRN("Failed to enable ext XTAL: %d", err);
	}
	err = poweron_uart();
	if (err) {
		LOG_ERR("Failed to power on uart: %d", err);
		return;
	}

	gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_WAKEUP_PIN, GPIO_INT_DISABLE);
	gpio_remove_callback(gpio_dev, gpio_cb);
	gpio_pin_configure(gpio_dev, CONFIG_SLM_WAKEUP_PIN, GPIO_DISCONNECTED);
}

void enter_idle(void)
{
	int err;

	gpio_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
	if (gpio_dev == NULL) {
		LOG_ERR("GPIO_0 bind error");
		return;
	}
	err = gpio_pin_configure(gpio_dev, CONFIG_SLM_WAKEUP_PIN, GPIO_INPUT | GPIO_PULL_UP);
	if (err) {
		LOG_ERR("GPIO_0 config error: %d", err);
		return;
	}
	gpio_init_callback(&gpio_cb, gpio_cb_func, BIT(CONFIG_SLM_WAKEUP_PIN));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		LOG_ERR("GPIO_0 add callback error: %d", err);
		return;
	}
	err = gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_WAKEUP_PIN, GPIO_INT_LEVEL_LOW);
	if (err) {
		LOG_ERR("GPIO_0 enable callback error: %d", err);
		return;
	}

	err = ext_xtal_control(false);
	if (err < 0) {
		LOG_WRN("Failed to disable ext XTAL: %d", err);
	}
}

void enter_sleep(void)
{
	/*
	 * Due to errata 4, Always configure PIN_CNF[n].INPUT before PIN_CNF[n].SENSE.
	 */
	nrf_gpio_cfg_input(CONFIG_SLM_WAKEUP_PIN, NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(CONFIG_SLM_WAKEUP_PIN, NRF_GPIO_PIN_SENSE_LOW);

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
}

static void handle_nrf_modem_lib_init_ret(void)
{
	int ret = nrf_modem_lib_get_init_ret();

	/* Handle return values relating to modem firmware update */
	switch (ret) {
	case 0:
		return; /* Initialization successful, no action required. */
	case MODEM_DFU_RESULT_OK:
		LOG_INF("MODEM UPDATE OK. Will run new firmware");
		fota_stage = FOTA_STAGE_COMPLETE;
		fota_status = FOTA_STATUS_OK;
		fota_info = 0;
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("MODEM UPDATE ERROR %d. Will run old firmware", ret);
		fota_status = FOTA_STATUS_ERROR;
		fota_info = ret;
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("MODEM UPDATE FATAL ERROR %d. Modem failure", ret);
		fota_status = FOTA_STATUS_ERROR;
		fota_info = ret;
		break;
	default:
		/* All non-zero return codes other than DFU result codes are
		 * considered irrecoverable and a reboot is needed.
		 */
		LOG_ERR("nRF modem lib initialization failed, error: %d", ret);
		fota_status = FOTA_STATUS_ERROR;
		fota_info = ret;
		break;
	}

	slm_setting_fota_save();
	LOG_WRN("Rebooting...");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}

void handle_mcuboot_swap_ret(void)
{
	int err;

	/** When a TEST image is swapped to primary partition and booted by MCUBOOT,
	 * the API mcuboot_swap_type() will return BOOT_SWAP_TYPE_REVERT. By this type
	 * MCUBOOT means that the TEST image is booted OK and, if it's not confirmed
	 * next, it'll be swapped back to secondary partition and original application
	 * image will be restored to the primary partition (so-called Revert).
	 */
	int type = mcuboot_swap_type();

	fota_stage = FOTA_STAGE_COMPLETE;
	switch (type) {
	/** Attempt to boot the contents of slot 0. */
	case BOOT_SWAP_TYPE_NONE:
	/** Swap to slot 1. Absent a confirm command, revert back on next boot. */
	case BOOT_SWAP_TYPE_TEST:
	/** Swap to slot 1, and permanently switch to booting its contents. */
	case BOOT_SWAP_TYPE_PERM:
		fota_status = FOTA_STATUS_ERROR;
		fota_info = -EBADF;
		break;
	/** Swap back to alternate slot. A confirm changes this state to NONE. */
	case BOOT_SWAP_TYPE_REVERT:
		err = boot_write_img_confirmed();
		if (err) {
			fota_status = FOTA_STATUS_ERROR;
			fota_info = err;
		} else {
			fota_status = FOTA_STATUS_OK;
			fota_info = 0;
		} break;
		break;
	/** Swap failed because image to be run is not valid */
	case BOOT_SWAP_TYPE_FAIL:
	default:
		break;
	}
}

int start_execute(void)
{
	int err;
	static bool slm_started;

	if (!slm_started) {
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
		k_work_queue_start(&slm_work_q, slm_wq_stack_area,
				   K_THREAD_STACK_SIZEOF(slm_wq_stack_area),
				   SLM_WQ_PRIORITY, NULL);
		slm_started = true;
	}

	return 0;
}

static void indicate_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	indicate_stop();
	(void)start_execute();
}

int main(void)
{
	uint32_t rr = nrf_power_resetreas_get(NRF_POWER_NS);

	nrf_power_resetreas_clear(NRF_POWER_NS, 0x70017);
	LOG_DBG("RR: 0x%08x", rr);
	k_work_init_delayable(&indicate_work, indicate_wk);

	/* Init and load settings */
	if (slm_settings_init() != 0) {
		LOG_WRN("Failed to init slm settings");
	}
	/* Post-FOTA handling */
	if (fota_stage != FOTA_STAGE_INIT) {
		if (fota_type == DFU_TARGET_IMAGE_TYPE_MODEM_DELTA) {
			handle_nrf_modem_lib_init_ret();
		} else if (fota_type == DFU_TARGET_IMAGE_TYPE_MCUBOOT) {
			handle_mcuboot_swap_ret();
		} else {
			LOG_ERR("Unknown DFU type: %d", fota_type);
			fota_status = FOTA_STATUS_ERROR;
			fota_info = -EAGAIN;
		}
		return start_execute();
	}

#if defined(CONFIG_SLM_START_SLEEP)
	if ((rr & NRF_POWER_RESETREAS_OFF_MASK) ||     /* DETECT signal from GPIO*/
	    (rr & NRF_POWER_RESETREAS_DIF_MASK)) {     /* Entering debug interface mode */
		return start_execute();
	}
	enter_sleep();
	return 0;
#else
#if (CONFIG_SLM_INDICATE_PIN >= 0)
	if ((rr & NRF_POWER_RESETREAS_DOG_MASK) ||     /* watch dog reset */
	    (rr & NRF_POWER_RESETREAS_SREQ_MASK) ||    /* software reset */
	    (rr & NRF_POWER_RESETREAS_LOCKUP_MASK)) {  /* CPU lockup reset */
		if (indicate_start() == 0) {
			return 0;
		}
	}
#endif /* CONFIG_SLM_INDICATE_PIN >= 0 */
	return start_execute();
#endif /* CONFIG_SLM_START_SLEEP */
}

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <logging/log.h>
#include <logging/log_ctrl.h>

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <string.h>
#include <nrf_modem.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include <modem/nrf_modem_lib.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <sys/reboot.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include "slm_at_host.h"
#include "slm_at_fota.h"

LOG_MODULE_REGISTER(slm, CONFIG_SLM_LOG_LEVEL);

#define SLM_WQ_STACK_SIZE	KB(2)
#define SLM_WQ_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(slm_wq_stack_area, SLM_WQ_STACK_SIZE);

static const struct device *gpio_dev;
static struct gpio_callback gpio_cb;
static struct k_work exit_idle_work;
static bool full_idle_mode;

/* global variable used across different files */
struct k_work_q slm_work_q;

/* global variable defined in different files */
extern uint8_t fota_stage;
extern uint8_t fota_status;
extern int32_t fota_info;

/* global functions defined in different files */
int poweron_uart(void);
int slm_settings_init(void);
int slm_setting_fota_save(void);

/**@brief Recoverable modem library error. */
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	LOG_ERR("Modem library recoverable error: %u", err);
}

static void exit_idle(struct k_work *work)
{
	int err;

	LOG_INF("Exit idle, full mode: %d", full_idle_mode);
	gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_INTERFACE_PIN, GPIO_INT_DISABLE);
	gpio_remove_callback(gpio_dev, &gpio_cb);
	/* Do the same as nrf_gpio_cfg_default() */
	gpio_pin_configure(gpio_dev, CONFIG_SLM_INTERFACE_PIN, GPIO_INPUT);

	if (full_idle_mode) {
		/* Restart SLM services */
		err = slm_at_host_init();
		if (err) {
			LOG_ERR("Failed to init at_host: %d", err);
		}
	} else {
		/* Power on UART only */
		err = poweron_uart();
		if (err) {
			LOG_ERR("Failed to wake up uart: %d", err);
		}
	}
}

static void gpio_callback(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	k_work_submit_to_queue(&slm_work_q, &exit_idle_work);
}

void enter_idle(bool full_idle)
{
	int err;

	gpio_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
	if (gpio_dev == NULL) {
		LOG_ERR("GPIO_0 bind error");
		return;
	}
	err = gpio_pin_configure(gpio_dev, CONFIG_SLM_INTERFACE_PIN, GPIO_INPUT | GPIO_PULL_UP);
	if (err) {
		LOG_ERR("GPIO_0 config error: %d", err);
		return;
	}
	gpio_init_callback(&gpio_cb, gpio_callback, BIT(CONFIG_SLM_INTERFACE_PIN));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		LOG_ERR("GPIO_0 add callback error: %d", err);
		return;
	}
	err = gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_INTERFACE_PIN, GPIO_INT_LEVEL_LOW);
	if (err) {
		LOG_ERR("GPIO_0 enable callback error: %d", err);
		return;
	}

	full_idle_mode = full_idle;
}

void enter_sleep(void)
{
	/*
	 * Due to errata 4, Always configure PIN_CNF[n].INPUT before PIN_CNF[n].SENSE.
	 */
	nrf_gpio_cfg_input(CONFIG_SLM_INTERFACE_PIN, NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(CONFIG_SLM_INTERFACE_PIN, NRF_GPIO_PIN_SENSE_LOW);

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
		LOG_ERR("MODEM UPDATE FATAL ERROR %d. Modem failiure", ret);
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
	int type = mcuboot_swap_type();

	fota_stage = FOTA_STAGE_COMPLETE;
	switch (type) {
	/** Swap failed because image to be run is not valid */
	case BOOT_SWAP_TYPE_FAIL:
		LOG_INF("BOOT_SWAP_TYPE_FAIL");
		fota_status = FOTA_STATUS_ERROR;
		fota_info = -EBADF;
		break;
	/** Swap to slot 1. Absent a confirm command, revert back on next boot. */
	case BOOT_SWAP_TYPE_TEST:
		LOG_INF("BOOT_SWAP_TYPE_TEST");
		err = boot_write_img_confirmed();
		if (err) {
			fota_status = FOTA_STATUS_ERROR;
			fota_info = err;
		} else {
			fota_status = FOTA_STATUS_OK;
			fota_info = 0;
		} break;
	/** Swap back to alternate slot. A confirm changes this state to NONE. */
	case BOOT_SWAP_TYPE_REVERT:
		LOG_INF("BOOT_SWAP_TYPE_REVERT");
		boot_write_img_confirmed();
		fota_status = FOTA_STATUS_REVERTED;
		fota_info = 0;
		break;
	/** Attempt to boot the contents of slot 0. */
	case BOOT_SWAP_TYPE_NONE:
	/** Swap to slot 1, and permanently switch to booting its contents. */
	case BOOT_SWAP_TYPE_PERM:
	default:
		break;
	}
}

void start_execute(void)
{
	int err;
#if defined(CONFIG_SLM_EXTERNAL_XTAL)
	struct onoff_manager *clk_mgr;
	struct onoff_client cli = {};
#endif

	LOG_INF("Serial LTE Modem");

#if defined(CONFIG_SLM_EXTERNAL_XTAL)
	/* request external XTAL for UART */
	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	sys_notify_init_spinwait(&cli.notify);
	err = onoff_request(clk_mgr, &cli);
	if (err) {
		LOG_ERR("Clock request failed: %d", err);
		return;
	}
#endif

	/* Init and load settings */
	err = slm_settings_init();
	if (err) {
		LOG_ERR("Failed to init slm settings: %d", err);
		return;
	}

	/* Post-FOTA handling */
	if (fota_stage != FOTA_STAGE_INIT) {
		handle_nrf_modem_lib_init_ret();
		handle_mcuboot_swap_ret();
	}

	err = slm_at_host_init();
	if (err) {
		LOG_ERR("Failed to init at_host: %d", err);
		return;
	}

	k_work_queue_start(&slm_work_q, slm_wq_stack_area,
			   K_THREAD_STACK_SIZEOF(slm_wq_stack_area),
			   SLM_WQ_PRIORITY, NULL);
	k_work_init(&exit_idle_work, exit_idle);
}

#if defined(CONFIG_SLM_START_SLEEP)
int main(void)
{
	uint32_t rr = nrf_power_resetreas_get(NRF_POWER_NS);

	LOG_DBG("RR: 0x%08x", rr);
	if (rr & NRF_POWER_RESETREAS_OFF_MASK) {
		nrf_power_resetreas_clear(NRF_POWER_NS, 0x70017);
		start_execute();
	} else {
		LOG_INF("Sleep");
		enter_sleep();
	}

	return 0;
}
#else
int main(void)
{
	start_execute();

	return 0;
}
#endif	/* CONFIG_SLM_GPIO_WAKEUP */

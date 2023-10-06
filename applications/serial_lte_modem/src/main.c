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
#include <pm_config.h>
#include <net/fota_download.h>
#include "slm_at_host.h"
#include "slm_at_fota.h"
#include "slm_settings.h"
#include "slm_uart_handler.h"
#if defined(CONFIG_SLM_CARRIER)
#include "slm_at_carrier.h"
#endif

LOG_MODULE_REGISTER(slm, CONFIG_SLM_LOG_LEVEL);

#define SLM_WQ_STACK_SIZE	KB(4)
#define SLM_WQ_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(slm_wq_stack_area, SLM_WQ_STACK_SIZE);

static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static struct gpio_callback gpio_cb;
static atomic_t gpio_cb_running; /* Protect the gpio_cb. */
static struct k_work_delayable indicate_work;

struct k_work_q slm_work_q;

/* Forward declarations */
static void indicate_wk(struct k_work *work);

BUILD_ASSERT(CONFIG_SLM_WAKEUP_PIN >= 0, "Wake up pin not configured");

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

/* This function is safe to call twice after reset */
static int init_gpio(void)
{
	int err;

	if (!device_is_ready(gpio_dev)) {
		LOG_ERR("GPIO controller not ready");
		return -ENODEV;
	}
	err = gpio_pin_configure(gpio_dev, CONFIG_SLM_WAKEUP_PIN,
				 GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_LOW);
	if (err) {
		LOG_ERR("GPIO_0 config error: %d", err);
		return err;
	}
#if (CONFIG_SLM_INDICATE_PIN >= 0)
	err = gpio_pin_configure(gpio_dev, CONFIG_SLM_INDICATE_PIN,
				 GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (err) {
		LOG_ERR("GPIO_0 config error: %d", err);
		return err;
	}
#endif

	return 0;
}

int indicate_start(void)
{
	int err = 0;

#if (CONFIG_SLM_INDICATE_PIN >= 0)
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
#if (CONFIG_SLM_INDICATE_PIN >= 0)
	if (gpio_pin_set(gpio_dev, CONFIG_SLM_INDICATE_PIN, 0) != 0) {
		LOG_WRN("GPIO_0 set error");
	}
	LOG_DBG("Stop indicating");
#endif
}

static void gpio_cb_func(const struct device *dev, struct gpio_callback *gpio_callback,
			 uint32_t pins)
{
	int err;

	if ((BIT(CONFIG_SLM_WAKEUP_PIN) & pins) == 0) {
		return;
	}

	/* Prevent level triggered interrupt running this multiple times. */
	if (!atomic_cas(&gpio_cb_running, false, true)) {
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
	err = slm_uart_power_on();
	if (err) {
		(void)atomic_set(&gpio_cb_running, false);
		LOG_ERR("Failed to power on uart: %d", err);
		return;
	}

	gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_WAKEUP_PIN, GPIO_INT_DISABLE);
	gpio_remove_callback(gpio_dev, gpio_callback);
	(void)atomic_set(&gpio_cb_running, false);
}

void enter_idle(void)
{
	int err;

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
	 * At this moment WAKEUP_PIN has already been configured as INPUT at init_gpio().
	 */
	nrf_gpio_cfg_sense_set(CONFIG_SLM_WAKEUP_PIN, NRF_GPIO_PIN_SENSE_LOW);

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
}

void enter_shutdown(void)
{
	/* De-configure GPIOs */
	gpio_pin_interrupt_configure(gpio_dev, CONFIG_SLM_WAKEUP_PIN, GPIO_INT_DISABLE);
	gpio_pin_configure(gpio_dev, CONFIG_SLM_WAKEUP_PIN, GPIO_DISCONNECTED);
	gpio_pin_configure(gpio_dev, CONFIG_SLM_INDICATE_PIN, GPIO_DISCONNECTED);

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
}

static void on_modem_dfu_res(int dfu_res, void *ctx)
{
	switch (dfu_res) {
	case NRF_MODEM_DFU_RESULT_OK:
		LOG_INF("MODEM UPDATE OK. Running new firmware");
		slm_fota_stage = FOTA_STAGE_COMPLETE;
		slm_fota_status = FOTA_STATUS_OK;
		slm_fota_info = 0;
		break;
	case NRF_MODEM_DFU_RESULT_UUID_ERROR:
	case NRF_MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("MODEM UPDATE ERROR %d. Running old firmware", dfu_res);
		slm_fota_status = FOTA_STATUS_ERROR;
		slm_fota_info = dfu_res;
		break;
	case NRF_MODEM_DFU_RESULT_HARDWARE_ERROR:
	case NRF_MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("MODEM UPDATE FATAL ERROR %d. Modem failure", dfu_res);
		slm_fota_status = FOTA_STATUS_ERROR;
		slm_fota_info = dfu_res;
		break;
	case NRF_MODEM_DFU_RESULT_VOLTAGE_LOW:
		LOG_ERR("MODEM UPDATE CANCELLED %d.", dfu_res);
		LOG_ERR("Please reboot once you have sufficient power for the DFU");
		slm_fota_stage = FOTA_STAGE_ACTIVATE;
		slm_fota_status = FOTA_STATUS_ERROR;
		slm_fota_info = dfu_res;
		break;
	default:
		LOG_WRN("Unhandled nrf_modem DFU result code %d.", dfu_res);

		/* All unknown return codes are considered irrecoverable and reprogramming
		 * the modem is needed.
		 */
		LOG_ERR("nRF modem lib initialization failed, error: %d", dfu_res);
		slm_fota_status = FOTA_STATUS_ERROR;
		slm_fota_info = dfu_res;
		slm_settings_fota_save();
		break;
	}
}

static void handle_mcuboot_swap_ret(void)
{
	int err;

	/** When a TEST image is swapped to primary partition and booted by MCUBOOT,
	 * the API mcuboot_swap_type() will return BOOT_SWAP_TYPE_REVERT. By this type
	 * MCUBOOT means that the TEST image is booted OK and, if it's not confirmed
	 * next, it'll be swapped back to secondary partition and original application
	 * image will be restored to the primary partition (so-called Revert).
	 */
	int type = mcuboot_swap_type();

	slm_fota_stage = FOTA_STAGE_COMPLETE;
	switch (type) {
	/** Swap to slot 1. Absent a confirm command, revert back on next boot. */
	case BOOT_SWAP_TYPE_TEST:
	/** Swap to slot 1, and permanently switch to booting its contents. */
	case BOOT_SWAP_TYPE_PERM:
		slm_fota_status = FOTA_STATUS_ERROR;
		slm_fota_info = -EBADF;
		break;
	/** Swap back to alternate slot. A confirm changes this state to NONE. */
	case BOOT_SWAP_TYPE_REVERT:
		err = boot_write_img_confirmed();
		if (err) {
			slm_fota_status = FOTA_STATUS_ERROR;
			slm_fota_info = err;
		} else {
			slm_fota_status = FOTA_STATUS_OK;
			slm_fota_info = 0;
		}
		break;
	/** Swap failed because image to be run is not valid */
	case BOOT_SWAP_TYPE_FAIL:
	default:
		break;
	}
}

int lte_auto_connect(void)
{
	int err = 0;

#if defined(CONFIG_SLM_CARRIER)
	int stat;

	if (!slm_carrier_auto_connect) {
		return 0;
	}
	err = nrf_modem_at_scanf("AT+CEREG?", "+CEREG: %d", &stat);
	if (err != 1 || (stat == 1 || stat == 5)) {
		return 0;
	}

	LOG_INF("auto connect");
#if defined(CONFIG_LWM2M_CARRIER_SERVER_BINDING_N)
	err = nrf_modem_at_printf("AT%%XSYSTEMMODE=0,1,0,0");
	if (err) {
		LOG_ERR("Failed to configure RAT: %d", err);
		return err;
	}
#endif /* CONFIG_LWM2M_CARRIER_SERVER_BINDING_N */
	err = nrf_modem_at_printf("AT+CFUN=1");
	if (err) {
		LOG_ERR("Failed to turn on radio: %d", err);
		return err;
	}
#endif /* CONFIG_SLM_CARRIER */

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
	uint32_t rr = nrf_power_resetreas_get(NRF_POWER_NS);
	enum fota_stage fota_stage;

	nrf_power_resetreas_clear(NRF_POWER_NS, 0x70017);
	LOG_DBG("RR: 0x%08x", rr);
	k_work_init_delayable(&indicate_work, indicate_wk);

	/* Init GPIOs */
	if (init_gpio() != 0) {
		LOG_WRN("Failed to init gpio");
	}
	/* Init and load settings */
	if (slm_settings_init() != 0) {
		LOG_WRN("Failed to init slm settings");
	}

	/* The fota stage is updated in the dfu_callback during modem initialization.
	 * We store it here to see if a fota was performed during this modem init.
	 */
	fota_stage = slm_fota_stage;

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


	/* Post-FOTA handling */
	if (fota_stage == FOTA_STAGE_ACTIVATE) {
		if (slm_fota_type == DFU_TARGET_IMAGE_TYPE_FULL_MODEM) {
#if defined(CONFIG_SLM_FULL_FOTA)
			slm_finish_modem_full_dfu();
#endif
		} else if (slm_fota_type == DFU_TARGET_IMAGE_TYPE_MCUBOOT) {
			handle_mcuboot_swap_ret();
		} else if (slm_fota_type != DFU_TARGET_IMAGE_TYPE_MODEM_DELTA) {
			LOG_ERR("Unknown DFU type: %d", slm_fota_type);
			slm_fota_status = FOTA_STATUS_ERROR;
			slm_fota_info = -EAGAIN;
		}
	}
#if defined(CONFIG_SLM_START_SLEEP)
	else {
		if ((rr & NRF_POWER_RESETREAS_OFF_MASK) ||  /* DETECT signal from GPIO*/
			(rr & NRF_POWER_RESETREAS_DIF_MASK)) {  /* Entering debug interface mode */
			return start_execute();
		}
		enter_sleep();
		return 0;
	}
#endif /* CONFIG_SLM_START_SLEEP */

	return start_execute();
}

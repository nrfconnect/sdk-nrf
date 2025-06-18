/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <nrf_modem.h>
#include <hal/nrf_power.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <zephyr/sys/reboot.h>
#include <net/fota_download.h>
#include "slm_at_host.h"
#include "slm_at_fota.h"
#include "slm_settings.h"
#include "slm_util.h"
#include "slm_ctrl_pin.h"

LOG_MODULE_REGISTER(slm, CONFIG_SLM_LOG_LEVEL);

#define SLM_WQ_STACK_SIZE	KB(4)
#define SLM_WQ_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(slm_wq_stack_area, SLM_WQ_STACK_SIZE);

struct k_work_q slm_work_q;

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
static struct nrf_modem_fault_info modem_fault_info;

static void on_modem_failure(struct k_work *)
{
	int ret;

	rsp_send("\r\n#XMODEM: FAULT,0x%x,0x%x\r\n", modem_fault_info.reason,
		 modem_fault_info.program_counter);

	ret = nrf_modem_lib_shutdown();
	rsp_send("\r\n#XMODEM: SHUTDOWN,%d\r\n", ret);

	ret = nrf_modem_lib_init();
	rsp_send("\r\n#XMODEM: INIT,%d\r\n", ret);
}
K_WORK_DEFINE(modem_failure_work, on_modem_failure);

void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	modem_fault_info = *fault_info;

	k_work_submit(&modem_failure_work);
}
#endif /* CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC */

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
	int ret;
	int n;
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

	ret = slm_util_at_scanf("AT+CEREG?", "+CEREG: %d,%d", &n, &stat);
	if (ret != 2 || (stat == 1 || stat == 5)) {
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

	slm_ctrl_pin_init();

	/* This will send "READY" or "INIT ERROR" to UART so after this nothing
	 * should be done that can fail
	 */
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

int main(void)
{
	const uint32_t rr = nrf_power_resetreas_get(NRF_POWER_NS);

	nrf_power_resetreas_clear(NRF_POWER_NS, 0x70017);
	LOG_DBG("RR: 0x%08x", rr);

	slm_ctrl_pin_init_gpios();

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

	int ret = nrf_modem_lib_init();

	if (ret) {
		LOG_ERR("Modem library init failed, err: %d", ret);
		if (ret != -EAGAIN && ret != -EIO) {
			goto exit;
		} else if (ret == -EIO) {
			LOG_ERR("Please program full modem firmware with the bootloader or "
				"external tools");
		}
	}

	check_app_fota_status();

#if defined(CONFIG_SLM_START_SLEEP)

	if (!(rr & NRF_POWER_RESETREAS_OFF_MASK)) { /* DETECT signal from GPIO */

		slm_ctrl_pin_enter_sleep_no_uninit();
	}
#endif /* CONFIG_SLM_START_SLEEP */

	ret = start_execute();
exit:
	if (ret) {
		LOG_ERR("Failed to start SLM (%d). It's not operational!!!", ret);
	}
	return ret;
}

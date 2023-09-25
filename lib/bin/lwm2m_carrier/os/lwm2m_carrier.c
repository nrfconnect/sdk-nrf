/*
 * Copyright (c) 2019-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <lwm2m_carrier.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem.h>
#include <nrf_errno.h>

#define LWM2M_CARRIER_THREAD_STACK_SIZE 4096
#define LWM2M_CARRIER_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

static int nrf_modem_dfu_result;

NRF_MODEM_LIB_ON_DFU_RES(lwm2m_carrier_dfu_hook, on_modem_lib_dfu, NULL);
NRF_MODEM_LIB_ON_INIT(lwm2m_carrier_init_hook, on_modem_lib_init, NULL);
NRF_MODEM_LIB_ON_SHUTDOWN(lwm2m_carrier_shutdown_hook, on_modem_lib_shutdown, NULL);

#if defined(CONFIG_LTE_LINK_CONTROL)
#include <modem/lte_lc.h>

__weak int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	int err;

	/* Minimal event handling required by the LwM2M carrier library.
	 * The application may replace this handler to have better control of the LTE link.
	 */
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP:
		err = lte_lc_connect_async(NULL);
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN:
		err = lte_lc_offline();
		break;
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		err = lte_lc_power_off();
		break;
	case LWM2M_CARRIER_EVENT_MODEM_INIT:
		err = nrf_modem_lib_init();
		break;
	case LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN:
		err = nrf_modem_lib_shutdown();
		break;
	default:
		break;
	}

	return 0;
}
#else
#include <nrf_modem_at.h>

__weak int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	int err;

	/* Minimal event handling required by the LwM2M carrier library.
	 * The application may replace this handler to have better control of the LTE link.
	 */
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP:
		err = nrf_modem_at_printf("AT+CFUN=1");
		if (err) {
			printk("Failed to set the modem to normal functional mode.\n");
		}
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN:
		err = nrf_modem_at_printf("AT+CFUN=4");
		if (err) {
			printk("Failed to set the modem to offline functional mode\n");
		}
		break;
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		err = nrf_modem_at_printf("AT+CFUN=0");
		if (err) {
			printk("Failed to set the modem to powered off functional mode\n");
		}
		break;
	case LWM2M_CARRIER_EVENT_MODEM_INIT:
		err = nrf_modem_lib_init();
		break;
	case LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN:
		err = nrf_modem_lib_shutdown();
		break;
	default:
		break;
	}

	return 0;
}
#endif

static void on_modem_lib_dfu(int32_t dfu_res, void *ctx)
{
	switch (dfu_res) {
	case NRF_MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful.\n");
		printk("Modem is running the new firmware.\n");
		nrf_modem_dfu_result = LWM2M_CARRIER_MODEM_INIT_UPDATED;
		break;
	case NRF_MODEM_DFU_RESULT_UUID_ERROR:
	case NRF_MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed.\n");
		printk("Modem is running non-updated firmware.\n");
		nrf_modem_dfu_result = LWM2M_CARRIER_MODEM_INIT_UPDATE_FAILED;
		break;
	case NRF_MODEM_DFU_RESULT_HARDWARE_ERROR:
	case NRF_MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed.\n");
		printk("Fatal error.\n");
		nrf_modem_dfu_result = LWM2M_CARRIER_MODEM_INIT_UPDATE_FAILED;
		break;
	case NRF_MODEM_DFU_RESULT_VOLTAGE_LOW:
		printk("Modem firmware update failed.\n");
		printk("Low voltage.\n");
		nrf_modem_dfu_result = LWM2M_CARRIER_MODEM_INIT_UPDATE_FAILED;
		break;
	default:
		printk("Modem update result %d. Assuming failure\n", dfu_res);
		nrf_modem_dfu_result = LWM2M_CARRIER_MODEM_INIT_UPDATE_FAILED;
		break;
	}
}

static void on_modem_lib_init(int ret, void *ctx)
{
	ARG_UNUSED(ctx);

	int result;

	if (nrf_modem_dfu_result) {
		result = nrf_modem_dfu_result;
	} else if (ret == -NRF_EPERM) {
		printk("Modem already initialized\n");
		return;
	} else if (ret == 0) {
		result = LWM2M_CARRIER_MODEM_INIT_SUCCESS;
	} else {
		printk("Could not initialize modem library.\n");
		printk("Fatal error.\n");
		result = -EIO;
	}

	/* Reset for a normal init without DFU. */
	nrf_modem_dfu_result = 0;

	lwm2m_carrier_on_modem_init(result);
}

static void on_modem_lib_shutdown(void *ctx)
{
	ARG_UNUSED(ctx);

	lwm2m_carrier_on_modem_shutdown();
}

__weak int lwm2m_carrier_custom_init(lwm2m_carrier_config_t *config)
{
	/* This function is not in use here. */
	ARG_UNUSED(config);

	return 0;
}

void lwm2m_carrier_thread_run(void)
{
	int err;

	lwm2m_carrier_config_t config = {0};

#ifdef CONFIG_LWM2M_CARRIER_GENERIC
	config.carriers_enabled |= LWM2M_CARRIER_GENERIC;
#endif

#ifdef CONFIG_LWM2M_CARRIER_VERIZON
	config.carriers_enabled |= LWM2M_CARRIER_VERIZON;
#endif

#ifdef CONFIG_LWM2M_CARRIER_ATT
	config.carriers_enabled |= LWM2M_CARRIER_ATT;
#endif

#ifdef CONFIG_LWM2M_CARRIER_LG_UPLUS
	config.carriers_enabled |= LWM2M_CARRIER_LG_UPLUS;
#endif

#ifdef CONFIG_LWM2M_CARRIER_T_MOBILE
	config.carriers_enabled |= LWM2M_CARRIER_T_MOBILE;
#endif

#ifdef CONFIG_LWM2M_CARRIER_SOFTBANK
	config.carriers_enabled |= LWM2M_CARRIER_SOFTBANK;
#endif

#ifndef CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD
	config.disable_bootstrap_from_smartcard = true;
#endif

	config.server_uri = CONFIG_LWM2M_CARRIER_CUSTOM_URI;

#ifdef CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER
	config.is_bootstrap_server = true;
#else
	config.server_lifetime = CONFIG_LWM2M_CARRIER_SERVER_LIFETIME;
#endif

	config.server_sec_tag = CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG;
	config.apn = CONFIG_LWM2M_CARRIER_CUSTOM_APN;
	config.pdn_type = CONFIG_LWM2M_CARRIER_PDN_TYPE;
	config.coap_con_interval = CONFIG_LWM2M_CARRIER_COAP_CON_INTERVAL;
	config.server_binding = CONFIG_LWM2M_SERVER_BINDING;
	config.manufacturer = CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER;
	config.model_number = CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER;
	config.device_type = CONFIG_LWM2M_CARRIER_DEVICE_TYPE;
	config.hardware_version = CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION;
	config.software_version = CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION;

#ifdef CONFIG_LWM2M_CARRIER_LG_UPLUS
	config.lg_uplus.service_code = CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE;

	if (IS_ENABLED(CONFIG_LWM2M_CARRIER_LG_UPLUS_2DID)) {
		config.lg_uplus.device_serial_no_type =
			LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_2DID;
	} else {
		config.lg_uplus.device_serial_no_type =
			LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_IMEI;
	}
#endif

#ifdef CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT
	config.session_idle_timeout = CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT;
#endif

	err = lwm2m_carrier_custom_init(&config);
	if (err != 0) {
		printk("Failed to initialize custom config settings. Error %d\n", err);
		return;
	}

	/* Run the LwM2M carrier library.
	 *
	 * Note: can also pass NULL to initialize with default settings
	 * (no certification mode or additional settings required by operator)
	 */
	err = lwm2m_carrier_main(&config);
	if (err != 0) {
		printk("Failed to run the LwM2M carrier library. Error %d\n", err);
	}
}

K_THREAD_DEFINE(lwm2m_carrier_thread, LWM2M_CARRIER_THREAD_STACK_SIZE,
		lwm2m_carrier_thread_run, NULL, NULL, NULL,
		LWM2M_CARRIER_THREAD_PRIORITY, 0, 0);

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>

#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <net/nrf_provisioning.h>
#include <net/nrf_cloud_coap.h>
#include "nrf_provisioning_at.h"

LOG_MODULE_REGISTER(nrf_provisioning_sample, CONFIG_NRF_PROVISIONING_SAMPLE_LOG_LEVEL);

K_SEM_DEFINE(nrf_provisioning_done_sem, 0, 1);

#if defined(CONFIG_NRF_CLOUD_COAP)
static void coap_connect(void)
{
	int ret;

	ret = nrf_cloud_coap_init();
	if (ret) {
		LOG_ERR("nrf_cloud_coap_init, error: %d", ret);
		return ret;
	}

	LOG_INF("Connecting to nRF Cloud CoAP...");

	ret = nrf_cloud_coap_connect(NULL);
	if (ret == 0) {
		LOG_INF("nRF Cloud CoAP connection successful");
	} else if (ret == -EACCES || ret == -ENOEXEC) {
		LOG_WRN("nRF Cloud CoAP connection failed, unauthorized");
	} else {
		LOG_WRN("nRF Cloud CoAP connection refused");
	}
}
#endif /* CONFIG_NRF_CLOUD_COAP */

static void nrf_provisioning_callback(const struct nrf_provisioning_callback_data *event)
{
	int ret;

	switch (event->type) {
	case NRF_PROVISIONING_EVENT_START:
		LOG_INF("Provisioning started");
		break;
	case NRF_PROVISIONING_EVENT_STOP:
		LOG_INF("Provisioning stopped");
		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED:
		LOG_INF("nRF Provisioning requires device to deactivate network");

		ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
		if (ret) {
			LOG_ERR("lte_lc_func_mode_set, error: %d", ret);
			return;
		}

		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED:
		LOG_INF("nRF Provisioning requires device to activate network");

		ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
		if (ret) {
			LOG_ERR("lte_lc_func_mode_set, error: %d", ret);
			return;
		}

		break;
	case NRF_PROVISIONING_EVENT_FAILED:
		LOG_ERR("Provisioning failed, try again...");
		break;
	case NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED:
		LOG_WRN("Provisioning failed, device not claimed");
		LOG_WRN("Claim the device using the device's attestation token on nrfcloud.com");

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_PROVIDE_ATTESTATION_TOKEN)) {
			LOG_WRN("Attestation token:\r\n\n%.*s.%.*s\r\n", event->token->attest_sz,
						       			 event->token->attest,
						       			 event->token->cose_sz,
						       			 event->token->cose);
		}

		break;
	case NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA:
		LOG_ERR("Provisioning failed, wrong root CA certificate for nRF Cloud provisioned");
		break;
	case NRF_PROVISIONING_EVENT_FAILED_NO_VALID_DATETIME:
		LOG_ERR("Provisioning failed, no valid datetime reference");
		break;
	case NRF_PROVISIONING_EVENT_FATAL_ERROR:
		LOG_ERR("Provisioning error, irrecoverable");
		break;
	case NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING:
		LOG_INF("Provisioning scheduled, next attempt in %lld seconds",
			event->next_attempt_time_seconds);
		break;
	case NRF_PROVISIONING_EVENT_DONE:
		LOG_INF("Provisioning done");
		LOG_INF("The device can now connect to the provisionined cloud service");
#if defined(CONFIG_NRF_CLOUD_COAP)
		coap_connect();
#endif /* CONFIG_NRF_CLOUD_COAP */
		break;
	default:
		LOG_WRN("Unknown event type: %d", event->type);
		break;
	}
}

int main(void)
{
	int ret;

	LOG_INF("nRF Device Provisioning Sample");

	ret = nrf_modem_lib_init();
	if (ret < 0) {
		LOG_ERR("Unable to init modem library (%d)", ret);
		return ret;
	}

	LOG_INF("Establishing LTE link ...");

	ret = lte_lc_connect();
	if (ret) {
		LOG_ERR("LTE link could not be established (%d)", ret);
		return ret;
	}

	if (!IS_ENABLED(CONFIG_NRF_PROVISIONING_AUTO_INIT)) {
		ret = nrf_provisioning_init(nrf_provisioning_callback);
		if (ret) {
			LOG_ERR("Failed to initialize provisioning client");
			return ret;
		}
	}

	return 0;
}

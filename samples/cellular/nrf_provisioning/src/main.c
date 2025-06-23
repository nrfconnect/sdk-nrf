/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <modem/lte_lc.h>
#include <net/nrf_provisioning.h>

#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif /* CONFIG_NRF_CLOUD_COAP */

LOG_MODULE_REGISTER(nrf_provisioning_sample, CONFIG_NRF_PROVISIONING_SAMPLE_LOG_LEVEL);

/* Macros used to subscribe to specific Zephyr NET management events. */
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)
#define L4_EVENT_MASK										\
	(NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED | NET_EVENT_L4_IPV6_CONNECTED |	\
	 NET_EVENT_L4_IPV4_CONNECTED)

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()					\
	LOG_ERR("Fatal error! Rebooting the device.");	\
	LOG_PANIC();					\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;

/* Function used to connect to nRF Cloud CoAP after provisioning. */
#if defined(CONFIG_NRF_CLOUD_COAP)
static void coap_connect(void)
{
	int ret;

	ret = nrf_cloud_coap_init();
	if (ret) {
		LOG_ERR("nrf_cloud_coap_init, error: %d", ret);
		FATAL_ERROR();
		return;
	}

	LOG_INF("Connecting to nRF Cloud CoAP...");

	ret = nrf_cloud_coap_connect(NULL);
	if (ret == 0) {
		LOG_INF("nRF Cloud CoAP connection successful");
	} else if (ret == -EACCES || ret == -ENOEXEC) {
		LOG_ERR("nRF Cloud CoAP connection failed, unauthorized");
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

		ret = conn_mgr_all_if_disconnect(true);
		if (ret) {
			LOG_ERR("conn_mgr_all_if_disconnect, error: %d", ret);

			FATAL_ERROR();
			return;
		}

		/* conn_mgr disables only LTE connectivity. GNSS may remain active.
		 * Ensure GNSS is also disabled before storing credentials to the modem.
		 */
		enum lte_lc_func_mode current_mode;

		if (lte_lc_func_mode_get(&current_mode)) {
			LOG_ERR("Failed to read modem functional mode");

			FATAL_ERROR();
			return;
		}

		if (current_mode == LTE_LC_FUNC_MODE_ACTIVATE_GNSS) {
			LOG_WRN("GNSS is still active. Deactivating GNSS.");

			if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_GNSS)) {
				LOG_ERR("Failed to deactivate GNSS");

				FATAL_ERROR();
				return;
			}
		}

		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED:
		LOG_INF("nRF Provisioning requires device to activate network");

		ret = conn_mgr_all_if_connect(true);
		if (ret) {
			LOG_ERR("conn_mgr_all_if_connect, error: %d", ret);
			FATAL_ERROR();
			return;
		}

		break;
	case NRF_PROVISIONING_EVENT_FAILED_TOO_MANY_COMMANDS:
		LOG_ERR("Provisioning failed, too many commands for the device to handle");
		LOG_ERR("Increase CONFIG_NRF_PROVISIONING_CBOR_RECORDS to allow more commands");
		LOG_ERR("It also might be needed to increase the system heap size");
		break;
	case NRF_PROVISIONING_EVENT_NO_COMMANDS:
		LOG_INF("Provisioning done, no commands received from the server");
		break;
	case NRF_PROVISIONING_EVENT_FAILED:
		LOG_ERR("Provisioning failed, try again...");
		break;
	case NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED:
		LOG_WRN("Provisioning failed, device not claimed");
		LOG_WRN("Claim the device using the device's attestation token on nrfcloud.com");

		if (IS_ENABLED(CONFIG_NRF_PROVISIONING_PROVIDE_ATTESTATION_TOKEN)) {
			LOG_WRN("Attestation token:\r\n\n%.*s.%.*s\r\n",
				event->token->attest_sz,
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
		FATAL_ERROR();
		break;
	case NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING:
		LOG_INF("Provisioning scheduled, next attempt in %lld seconds",
			event->next_attempt_time_seconds);
		break;
	case NRF_PROVISIONING_EVENT_DONE:
		LOG_INF("Provisioning done");
		LOG_INF("The device can now connect to the provisioned cloud service");
#if defined(CONFIG_NRF_CLOUD_COAP)
		coap_connect();
#endif /* CONFIG_NRF_CLOUD_COAP */
		break;
	default:
		LOG_WRN("Unknown event type: %d", event->type);
		break;
	}
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint64_t event, struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		break;
	case NET_EVENT_L4_IPV6_CONNECTED:
		LOG_INF("IPv6 connectivity established");
		break;
	case NET_EVENT_L4_IPV4_CONNECTED:
		LOG_INF("IPv4 connectivity established");
		break;
	default:
		/* Don't care */
		return;
	}
}

int main(void)
{
	int ret;

	LOG_INF("nRF Device Provisioning Sample");

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	LOG_INF("Bringing network interface up and connecting to the network");

	ret = nrf_provisioning_init(nrf_provisioning_callback);
	if (ret) {
		LOG_ERR("Failed to initialize provisioning client");
		return ret;
	}

	ret = conn_mgr_all_if_up(true);
	if (ret) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", ret);
		return ret;
	}

	ret = conn_mgr_all_if_connect(true);
	if (ret) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", ret);
		return ret;
	}

	return 0;
}

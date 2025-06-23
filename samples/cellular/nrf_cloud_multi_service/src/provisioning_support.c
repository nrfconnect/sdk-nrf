/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/conn_mgr_connectivity.h>

#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <net/nrf_provisioning.h>

#include "cloud_connection.h"
#include "sample_reboot.h"

LOG_MODULE_REGISTER(cloud_provisioning, CONFIG_MULTI_SERVICE_LOG_LEVEL);

#define PROVISIONING_IDLE		BIT(2)

static K_EVENT_DEFINE(prov_events);

/* Delayable work item for marking provisioning as idle. */
static void mark_provisioning_idle_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("Provisioning is idle.");
	k_event_post(&prov_events, PROVISIONING_IDLE);
}

static K_WORK_DELAYABLE_DEFINE(provisioning_idle_work, mark_provisioning_idle_work_fn);

static void nrf_provisioning_callback(const struct nrf_provisioning_callback_data *event)
{
	int err;

	switch (event->type) {
	case NRF_PROVISIONING_EVENT_START:
		/* Called when the provisioning library begins checking for provisioning commands.
		 */
		LOG_DBG("NRF_PROVISIONING_EVENT_START");

		/* Mark provisioning as active. */
		LOG_INF("Provisioning is active.");
		k_event_clear(&prov_events, PROVISIONING_IDLE);
		break;
	case NRF_PROVISIONING_EVENT_STOP:
		/* Called when the current provisioning command check has completed. */
		LOG_DBG("NRF_PROVISIONING_EVENT_STOP");

		/* Mark provisioning as idle after a small delay.
		 * This delay is to prevent false starts if the provisioning library performs an
		 * immediate retry.
		 */
		k_work_reschedule(&provisioning_idle_work, K_SECONDS(5));
		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED:
		LOG_INF("Provisioning library requests offline mode");

		/* The provisioning library wants to install or generate credentials.
		 * Deactivate LTE and GNSS to allow this.
		 */

		/* Shut down LTE */
		err = conn_mgr_all_if_disconnect(true);
		if (err) {
			LOG_ERR("Failed to disconnect from LTE network, error: %d", err);
			sample_reboot_error();
			return;
		}

		/* Shut down GNSS */
		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_GNSS);
		if (err) {
			LOG_ERR("Failed to deactivate GNSS, error: %d", err);
			sample_reboot_error();
			return;
		}

		break;
	case NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED:
		LOG_INF("Provisioning library requests normal mode");

		/* We are done installing credentials, reactivate GNSS and network */

		/* Reactivate GNSS */
		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS);
		if (err) {
			LOG_ERR("Failed to activate GNSS, error: %d", err);
			sample_reboot_error();
			return;
		}

		/* Reactivate LTE */
		err = conn_mgr_all_if_connect(true);
		if (err) {
			LOG_ERR("Failed to connect to LTE network, error: %d", err);
			sample_reboot_error();
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
		sample_reboot_error();
		break;
	case NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING:
		LOG_INF("Provisioning scheduled, next attempt in %lld seconds",
			event->next_attempt_time_seconds);
		break;
	case NRF_PROVISIONING_EVENT_DONE:
		/* Called when there are no further provisioning commands,
		 * and a reboot is needed.
		 */
		LOG_DBG("NRF_PROVISIONING_EVENT_DONE");

		LOG_INF("Provisioning completed.");
		sample_reboot_normal();
		break;
	default:
		LOG_WRN("Unknown event type: %d", event->type);
		break;
	}
}

/* Callback to track network connectivity */
static struct net_mgmt_event_callback l4_callback;

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint64_t event, struct net_if *iface)
{
	if (event == NET_EVENT_L4_CONNECTED) {
		int err = nrf_provisioning_init(nrf_provisioning_callback);

		if (err == -EALREADY) {
			/* The provisioning library is already initialized, ignoring. */
			return;
		} else if (err) {
			LOG_ERR("Failed to initialize provisioning library, error: %d", err);
			sample_reboot_error();
			return;
		}
	} else if (event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network connectivity lost!");
	}
}

/* Set up any requirements for provisioning on boot */
static int prepare_provisioning(void)
{
	/* Start tracking network availability */
	net_mgmt_init_event_callback(
		&l4_callback, l4_event_handler, NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED
	);
	net_mgmt_add_event_callback(&l4_callback);
	return 0;
}

SYS_INIT(prepare_provisioning, APPLICATION, 0);

bool await_provisioning_idle(k_timeout_t timeout)
{
	return k_event_wait(&prov_events, PROVISIONING_IDLE, false, timeout) != 0;
}

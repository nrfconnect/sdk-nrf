/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/pdn.h>
#include <modem/lte_lc.h>

#include "network_common.h"

LOG_MODULE_REGISTER(network, CONFIG_MEMFAULT_SAMPLE_LOG_LEVEL);

/* Handler that is used to notify the application about LTE link specific events. */
static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	if ((evt->type == LTE_LC_EVT_MODEM_EVENT) &&
	    (evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP)) {
		LOG_WRN("The modem has detected a reset loop.");
		LOG_WRN("LTE network attach is now "
			"restricted for the next 30 minutes.");
		LOG_WRN("Power-cycle the device to "
			"circumvent this restriction.");
		LOG_WRN("For more information see the nRF91 AT Commands - Command "
			"Reference Guide v2.0 - chpt. 5.36");
	}
}

/* Handler that notifies the application of events related to the default PDN context, CID 0. */
void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	ARG_UNUSED(cid);

	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		LOG_DBG("Event: PDP context %d, %s", cid, pdn_esm_strerror(reason));
		return;
	case PDN_EVENT_ACTIVATED:
		LOG_INF("PDN connection activated");
		network_connected();
		break;
	case PDN_EVENT_DEACTIVATED:
		LOG_INF("PDN connection deactivated");
		network_disconnected();
		break;
	case PDN_EVENT_IPV6_UP:
		LOG_DBG("PDN_EVENT_IPV6_UP");
		return;
	case PDN_EVENT_IPV6_DOWN:
		LOG_DBG("PDN_EVENT_IPV6_DOWN");
		return;
	default:
		LOG_ERR("Unexpected PDN event: %d", event);
		return;
	}
}

static void modem_configure(void)
{
	int err;

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Modem library could not be initialized, err %d", err);
		return;
	}

	/* Setup a callback for the default PDP context. */
	err = pdn_default_ctx_cb_reg(pdn_event_handler);

	if (err) {
		LOG_ERR("pdn_default_ctx_cb_reg, error: %d", err);
		return;
	}

	/* Register handler to receive LTE link specific events. */
	lte_lc_register_handler(lte_event_handler);

	/* Subscribe to modem domain events (AT%MDMEV).
	 * Modem domain events is received in the lte_event_handler().
	 *
	 * This function fails for modem firmware versions < 1.3.0 due to not being supported.
	 * Therefore we ignore its return value.
	 */
	(void)lte_lc_modem_events_enable();

	LOG_INF("Connecting to LTE network...");

	/* Initialize the link controller and connect to LTE network. */
	err = lte_lc_init_and_connect();
	if (err) {
		LOG_ERR("lte_lc_init_and_connect, error: %d", err);
	}
}

void network_init(void)
{
	modem_configure();

}

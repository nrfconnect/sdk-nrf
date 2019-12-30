/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "nrf_cloud_fsm.h"
#include "nrf_cloud_codec.h"
#include "nrf_cloud_mem.h"

#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_fsm, CONFIG_NRF_CLOUD_LOG_LEVEL);

/**@brief Identifier for cloud state request.
 * Can be any unique unsigned 16-bit integer value except zero.
 */
#define CLOUD_STATE_REQ_ID 5678

/**@brief Identifier for message sent to report status in UA_COMPLETE state.
 * Can be any unique unsigned 16-bit integer value except zero.
 */
#define PAIRING_STATUS_REPORT_ID 7890

/**@brief Default message identifier.
 * Can be any unique unsigned 16-bit integer value except zero.
 */
#define DEFAULT_REPORT_ID 1

typedef int (*fsm_transition)(const struct nct_evt *nct_evt);

static int drop_event_handler(const struct nct_evt *nct_evt);
static int connection_handler(const struct nct_evt *nct_evt);
static int disconnection_handler(const struct nct_evt *nct_evt);
static int cc_connection_handler(const struct nct_evt *nct_evt);
static int cc_tx_ack_handler(const struct nct_evt *nct_evt);
static int cc_tx_ack_in_state_requested_handler(const struct nct_evt *nct_evt);
static int cc_disconnection_handler(const struct nct_evt *nct_evt);
static int dc_connection_handler(const struct nct_evt *nct_evt);
static int dc_rx_data_handler(const struct nct_evt *nct_evt);
static int dc_tx_ack_handler(const struct nct_evt *nct_evt);
static int dc_disconnection_handler(const struct nct_evt *nct_evt);
static int cc_rx_data_handler(const struct nct_evt *nct_evt);
static int handle_pin_complete(const struct nct_evt *nct_evt);
static int handle_device_config_update(const struct nct_evt *const evt,
				       bool *const config_found);

/* Drop all the events. */
static const fsm_transition not_implemented_fsm_transition[NCT_EVT_TOTAL];

static const fsm_transition initialized_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_CONNECTED] = connection_handler,
};

static const fsm_transition connected_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

static const fsm_transition cc_connecting_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_CC_CONNECTED] = cc_connection_handler,
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

static const fsm_transition cc_connected_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_CC_RX_DATA] = cc_rx_data_handler,
	[NCT_EVT_CC_DISCONNECTED] = cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

static const fsm_transition cloud_requested_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_CC_RX_DATA] = cc_rx_data_handler,
	[NCT_EVT_CC_TX_DATA_ACK] = cc_tx_ack_in_state_requested_handler,
	[NCT_EVT_CC_DISCONNECTED] = cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

static const fsm_transition ua_complete_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_CC_RX_DATA] = cc_rx_data_handler,
	[NCT_EVT_CC_TX_DATA_ACK] = cc_tx_ack_handler,
	[NCT_EVT_CC_DISCONNECTED] = cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

static const fsm_transition dc_connecting_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_DC_CONNECTED] = dc_connection_handler,
	[NCT_EVT_CC_RX_DATA] = cc_rx_data_handler,
	[NCT_EVT_CC_TX_DATA_ACK] = cc_tx_ack_handler,
	[NCT_EVT_CC_DISCONNECTED] = cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

static const fsm_transition dc_connected_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_CC_RX_DATA] = cc_rx_data_handler,
	[NCT_EVT_CC_TX_DATA_ACK] = cc_tx_ack_handler,
	[NCT_EVT_DC_RX_DATA] = dc_rx_data_handler,
	[NCT_EVT_DC_TX_DATA_ACK] = dc_tx_ack_handler,
	[NCT_EVT_CC_DISCONNECTED] = cc_disconnection_handler,
	[NCT_EVT_DC_DISCONNECTED] = dc_disconnection_handler,
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

static const fsm_transition *state_event_handlers[] = {
	[STATE_IDLE] = not_implemented_fsm_transition,
	[STATE_INITIALIZED] = initialized_fsm_transition,
	[STATE_CONNECTED] = connected_fsm_transition,
	[STATE_CC_CONNECTING] = cc_connecting_fsm_transition,
	[STATE_CC_CONNECTED] = cc_connected_fsm_transition,
	[STATE_CLOUD_STATE_REQUESTED] = cloud_requested_fsm_transition,
	[STATE_UA_PIN_WAIT] = cloud_requested_fsm_transition,
	[STATE_UA_PIN_COMPLETE] = ua_complete_fsm_transition,
	[STATE_DC_CONNECTING] = dc_connecting_fsm_transition,
	[STATE_DC_CONNECTED] = dc_connected_fsm_transition,
	[STATE_READY] = not_implemented_fsm_transition,
	[STATE_DISCONNECTING] = not_implemented_fsm_transition,
	[STATE_ERROR] = not_implemented_fsm_transition,
};
BUILD_ASSERT(ARRAY_SIZE(state_event_handlers) == STATE_TOTAL);

int nfsm_init(void)
{
	return 0;
}

int nfsm_handle_incoming_event(const struct nct_evt *nct_evt,
			       enum nfsm_state state)
{
	int err;

	if ((nct_evt == NULL) || (nct_evt->type >= NCT_EVT_TOTAL) ||
	    (state >= STATE_TOTAL)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	if (state_event_handlers[state][nct_evt->type] != NULL) {
		err = state_event_handlers[state][nct_evt->type](nct_evt);

		if (err) {
			LOG_ERR("Handler failed! state: %d, type: %d", state,
				nct_evt->type);
		}
	} else {
		err = drop_event_handler(nct_evt);
	}

	return err;
}

static int state_ua_pin_wait(void)
{
	int err;
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = DEFAULT_REPORT_ID,
	};

	/* Publish report to the cloud on current status. */
	err = nrf_cloud_encode_state(STATE_UA_PIN_WAIT, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_encode_state failed %d", err);
		return err;
	}

	err = nct_cc_send(&msg);
	if (err) {
		LOG_ERR("nct_cc_send failed %d", err);
		nrf_cloud_free((void *)msg.data.ptr);
		return err;
	}

	nrf_cloud_free((void *)msg.data.ptr);

	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST,
	};

	nfsm_set_current_state_and_notify(STATE_UA_PIN_WAIT, &evt);

	return 0;
}

static int handle_device_config_update(const struct nct_evt *const evt,
				       bool *const config_found)
{
	int err;
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = DEFAULT_REPORT_ID,
	};

	struct nrf_cloud_evt cloud_evt = {
		.type = NRF_CLOUD_EVT_RX_DATA
	};

	if ((evt == NULL) || (config_found == NULL)) {
		return -EINVAL;
	}

	if (evt->param.cc == NULL) {
		return -ENOENT;
	}

	err = nrf_cloud_encode_config_response(&evt->param.cc->data, &msg.data,
					       config_found);
	if ((err) && (err != -ESRCH)) {
		LOG_ERR("nrf_cloud_encode_config_response failed %d", err);
		return err;
	}

	if (*config_found == false) {
		return 0;
	}

	if (msg.data.ptr) {
		err = nct_cc_send(&msg);
		nrf_cloud_free((void *)msg.data.ptr);

		if (err) {
			LOG_ERR("nct_cc_send failed %d", err);
		}
	}

	cloud_evt.data = evt->param.cc->data;
	nfsm_set_current_state_and_notify(nfsm_get_current_state(), &cloud_evt);

	return err;
}

static int state_ua_pin_complete(void)
{
	int err;
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = PAIRING_STATUS_REPORT_ID,
	};

	err = nrf_cloud_encode_state(STATE_UA_PIN_COMPLETE, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_encode_state failed %d", err);
		return err;
	}

	err = nct_cc_send(&msg);
	if (err) {
		LOG_ERR("nct_cc_send failed %d", err);
		nrf_cloud_free((void *)msg.data.ptr);
		return err;
	}

	nrf_cloud_free((void *)msg.data.ptr);

	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_USER_ASSOCIATED,
	};

	nfsm_set_current_state_and_notify(STATE_UA_PIN_COMPLETE, &evt);

	return err;
}

static int drop_event_handler(const struct nct_evt *nct_evt)
{
	LOG_DBG("Dropping FSM transition %d, current state %d", nct_evt->type,
		nfsm_get_current_state());
	return 0;
}

static int connection_handler(const struct nct_evt *nct_evt)
{
	int err;
	struct nrf_cloud_evt evt;

	/* Notify the application of the connection event.
	 * State transitions according to the event result.
	 */
	if (nct_evt->status != 0) {
		evt.type = NRF_CLOUD_EVT_ERROR;
		nfsm_set_current_state_and_notify(STATE_CONNECTED, &evt);
		return 0;
	}

	evt.type = NRF_CLOUD_EVT_TRANSPORT_CONNECTED;
	nfsm_set_current_state_and_notify(STATE_CONNECTED, &evt);

	/* Connect the control channel now. */
	err = nct_cc_connect();
	if (err) {
		return err;
	}

	nfsm_set_current_state_and_notify(STATE_CC_CONNECTING, NULL);

	return 0;
}

static int disconnection_handler(const struct nct_evt *nct_evt)
{
	/* Set the state to INITIALIZED and notify the application of
	 * disconnection.
	 */
	const struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED
	};

	nfsm_set_current_state_and_notify(STATE_INITIALIZED, &evt);

	return 0;
}

static int cc_connection_handler(const struct nct_evt *nct_evt)
{
	/* Set the state according to the status of the event.
	 * If status the connection, request state synchronization.
	 */
	static const struct nct_cc_data get_request = {
		.opcode = NCT_CC_OPCODE_GET_REQ,
		.id = CLOUD_STATE_REQ_ID,
	};

	int err;
	const struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_ERROR,
	};

	if (nct_evt->status != 0) {
		nfsm_set_current_state_and_notify(STATE_ERROR, &evt);
		return 0;
	}

	nfsm_set_current_state_and_notify(STATE_CC_CONNECTED, NULL);

	/* Request the shadow state now. */
	err = nct_cc_send(&get_request);
	if (err) {
		nfsm_set_current_state_and_notify(STATE_CONNECTED, &evt);
		return err;
	}

	nfsm_set_current_state_and_notify(STATE_CLOUD_STATE_REQUESTED, NULL);

	return 0;
}

static int handle_pin_complete(const struct nct_evt *nct_evt)
{
	int err;
	const struct nrf_cloud_data *payload = &nct_evt->param.cc->data;
	struct nrf_cloud_data rx;
	struct nrf_cloud_data tx;
	struct nrf_cloud_data endpoint;

	err = nrf_cloud_decode_data_endpoint(payload, &tx, &rx, &endpoint);
	if (err) {
		LOG_ERR("nrf_cloud_decode_data_endpoint failed %d", err);
		return err;
	}

	/* Set the endpoint information. */
	nct_dc_endpoint_set(&tx, &rx, &endpoint);

	return state_ua_pin_complete();
}

static int cc_rx_data_handler(const struct nct_evt *nct_evt)
{
	int err;
	enum nfsm_state new_state;
	const struct nrf_cloud_data *payload = &nct_evt->param.cc->data;
	bool config_found = false;
	const enum nfsm_state current_state = nfsm_get_current_state();

	handle_device_config_update(nct_evt, &config_found);

	err = nrf_cloud_decode_requested_state(payload, &new_state);

	if (err) {
		if (!config_found) {
			LOG_ERR("nrf_cloud_decode_requested_state Failed %d",
				err);
			return err;
		}

		/* Config only, nothing else to do */
		return 0;
	}

	switch (current_state) {
	case STATE_CC_CONNECTED:
	case STATE_CLOUD_STATE_REQUESTED:
	case STATE_UA_PIN_WAIT:
	case STATE_UA_PIN_COMPLETE:
		if (new_state == STATE_UA_PIN_COMPLETE) {
			return handle_pin_complete(nct_evt);
		} else if (new_state == STATE_UA_PIN_WAIT) {
			return state_ua_pin_wait();
		}
		break;
	case STATE_DC_CONNECTING:
	case STATE_DC_CONNECTED:
		if (new_state == STATE_UA_PIN_WAIT) {
			(void)nct_dc_disconnect();
			return state_ua_pin_wait();
		}
		break;
	default:
		break;
	}

	return 0;
}

static int cc_tx_ack_handler(const struct nct_evt *nct_evt)
{
	int err;

	if (nct_evt->param.data_id == CLOUD_STATE_REQ_ID) {
		nfsm_set_current_state_and_notify(STATE_CLOUD_STATE_REQUESTED,
						  NULL);
		return 0;
	}

	if (nct_evt->param.data_id == PAIRING_STATUS_REPORT_ID) {
		err = nct_dc_connect();
		if (err) {
			return err;
		}

		nfsm_set_current_state_and_notify(STATE_DC_CONNECTING, NULL);
	}

	return 0;
}

static int cc_tx_ack_in_state_requested_handler(const struct nct_evt *nct_evt)
{
	if (nct_evt->param.data_id == CLOUD_STATE_REQ_ID) {
		nfsm_set_current_state_and_notify(STATE_CLOUD_STATE_REQUESTED,
						  NULL);
	}
	return 0;
}

static int cc_disconnection_handler(const struct nct_evt *nct_evt)
{
	return 0; /* Nothing to do */
}

static int dc_connection_handler(const struct nct_evt *nct_evt)
{
	if (nct_evt->status == 0) {
		struct nrf_cloud_evt evt = {
			.type = NRF_CLOUD_EVT_READY,
		};

		nfsm_set_current_state_and_notify(STATE_DC_CONNECTED, &evt);
	}
	return 0;
}

static int dc_rx_data_handler(const struct nct_evt *nct_evt)
{
	struct nrf_cloud_evt cloud_evt = {
		.type = NRF_CLOUD_EVT_RX_DATA,
		.data = nct_evt->param.dc->data,
	};

	/* All data is forwared to the app */
	nfsm_set_current_state_and_notify(nfsm_get_current_state(), &cloud_evt);

	return 0;
}

static int dc_tx_ack_handler(const struct nct_evt *nct_evt)
{
	return 0; /* Nothing to do */
}

static int dc_disconnection_handler(const struct nct_evt *nct_evt)
{
	return 0; /* Nothing to do */
}

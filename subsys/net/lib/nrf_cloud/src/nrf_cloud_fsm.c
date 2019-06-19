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

/**@brief Identifier for message sent to report status in UA_INITIATE state.
 * Can be any unique unsigned 16-bit integer value except zero.
 */
#define INITIATE_STATUS_REPORT_ID 6789

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
static int initiate_n_complete_request_handler(const struct nct_evt *nct_evt);
static int initiate_cmd_handler(const struct nct_evt *nct_evt);
static int initiate_cmd_in_dc_conn_handler(const struct nct_evt *nct_evt);
static int all_ua_request_handler(const struct nct_evt *nct_evt);
static int cc_tx_cnf_handler(const struct nct_evt *nct_evt);
static int cc_tx_cnf_in_state_requested_handler(const struct nct_evt *nct_evt);
static int cc_disconnection_handler(const struct nct_evt *nct_evt);
static int dc_connection_handler(const struct nct_evt *nct_evt);
static int dc_rx_data_handler(const struct nct_evt *nct_evt);
static int dc_tx_cnf_handler(const struct nct_evt *nct_evt);
static int dc_disconnection_handler(const struct nct_evt *nct_evt);

static const fsm_transition not_implemented_fsm_transition[] = {
	/* Drop all the events. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_CC_DISCONNECTED]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
	[NCT_EVT_DISCONNECTED]		= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(not_implemented_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition initialized_fsm_transition[] = {
	[NCT_EVT_CONNECTED]		= connection_handler,
	/* Drop all the rest/ */
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_CC_DISCONNECTED]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
	[NCT_EVT_DISCONNECTED]		= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(initialized_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition connected_fsm_transition[] = {
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_CC_DISCONNECTED]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(connected_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition cc_connecting_fsm_transition[] = {
	[NCT_EVT_CC_CONNECTED]		= cc_connection_handler,
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_CC_DISCONNECTED]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(cc_connecting_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition cc_connected_fsm_transition[] = {
	[NCT_EVT_CC_RX_DATA]		= initiate_n_complete_request_handler,
	[NCT_EVT_CC_DISCONNECTED]	= cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(cc_connected_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition cloud_requested_fsm_transition[] = {
	[NCT_EVT_CC_RX_DATA]		= initiate_n_complete_request_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= cc_tx_cnf_in_state_requested_handler,
	[NCT_EVT_CC_DISCONNECTED]	= cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(cloud_requested_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition ua_initiate_fsm_transition[] = {
	[NCT_EVT_CC_RX_DATA]		= all_ua_request_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= cc_tx_cnf_in_state_requested_handler,
	[NCT_EVT_CC_DISCONNECTED]	= cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(ua_initiate_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition ua_pattern_wait_fsm_transition[] = {
	[NCT_EVT_CC_RX_DATA]		= all_ua_request_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= cc_tx_cnf_handler,
	[NCT_EVT_CC_DISCONNECTED]	= cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(ua_pattern_wait_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition ua_complete_fsm_transition[] = {
	[NCT_EVT_CC_RX_DATA]		= initiate_cmd_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= cc_tx_cnf_handler,
	[NCT_EVT_CC_DISCONNECTED]	= cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(ua_complete_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition dc_connecting_fsm_transition[] = {
	[NCT_EVT_DC_CONNECTED]		= dc_connection_handler,
	[NCT_EVT_CC_RX_DATA]		= initiate_cmd_in_dc_conn_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= cc_tx_cnf_handler,
	[NCT_EVT_CC_DISCONNECTED]	= cc_disconnection_handler,
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_RX_DATA]		= drop_event_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= drop_event_handler,
	[NCT_EVT_DC_DISCONNECTED]	= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(dc_connecting_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition dc_connected_fsm_transition[] = {
	[NCT_EVT_CC_RX_DATA]		= initiate_cmd_in_dc_conn_handler,
	[NCT_EVT_CC_TX_DATA_CNF]	= cc_tx_cnf_handler,
	[NCT_EVT_DC_RX_DATA]		= dc_rx_data_handler,
	[NCT_EVT_DC_TX_DATA_CNF]	= dc_tx_cnf_handler,
	[NCT_EVT_CC_DISCONNECTED]	= cc_disconnection_handler,
	[NCT_EVT_DC_DISCONNECTED]	= dc_disconnection_handler,
	[NCT_EVT_DISCONNECTED]		= disconnection_handler,
	/* Drop all the rest. */
	[NCT_EVT_CONNECTED]		= drop_event_handler,
	[NCT_EVT_CC_CONNECTED]		= drop_event_handler,
	[NCT_EVT_DC_CONNECTED]		= drop_event_handler,
};
BUILD_ASSERT(ARRAY_SIZE(dc_connected_fsm_transition) == NCT_EVT_TOTAL);

static const fsm_transition *state_event_handlers[] = {
	[STATE_IDLE]			= not_implemented_fsm_transition,
	[STATE_INITIALIZED]		= initialized_fsm_transition,
	[STATE_CONNECTED]		= connected_fsm_transition,
	[STATE_CC_CONNECTING]		= cc_connecting_fsm_transition,
	[STATE_CC_CONNECTED]		= cc_connected_fsm_transition,
	[STATE_CLOUD_STATE_REQUESTED]	= cloud_requested_fsm_transition,
	[STATE_UA_PIN_WAIT]		= cloud_requested_fsm_transition,
	[STATE_UA_PIN_COMPLETE]		= ua_complete_fsm_transition,
	[STATE_UA_INITIATE]		= ua_initiate_fsm_transition,
	[STATE_UA_INPUT_WAIT]		= ua_pattern_wait_fsm_transition,
	[STATE_UA_INPUT_MISMATCH]	= not_implemented_fsm_transition,
	[STATE_UA_INPUT_TIMEOUT]	= not_implemented_fsm_transition,
	[STATE_UA_COMPLETE]		= ua_complete_fsm_transition,
	[STATE_DC_CONNECTING]		= dc_connecting_fsm_transition,
	[STATE_DC_CONNECTED]		= dc_connected_fsm_transition,
	[STATE_READY]			= not_implemented_fsm_transition,
	[STATE_DISCONNECTING]		= not_implemented_fsm_transition,
	[STATE_ERROR]			= not_implemented_fsm_transition,
};
BUILD_ASSERT(ARRAY_SIZE(state_event_handlers) == STATE_TOTAL);


int nfsm_init(void)
{
	return 0;
}

int nfsm_handle_incoming_event(const struct nct_evt *nct_evt,
			       enum nfsm_state state)
{
	if (state < STATE_TOTAL) {
		int err = state_event_handlers[state][nct_evt->type](nct_evt);

		if (err) {
			LOG_ERR("Handler failed! state: %d, type: %d",
				state, nct_evt->type);
		}
		return err;
	}

	__ASSERT_NO_MSG(false);

	return -EINVAL;
}

static int state_ua_initiate(void)
{
	int err;
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = INITIATE_STATUS_REPORT_ID,
	};

	/* Publish report to the cloud on current status. */
	err = nrf_cloud_encode_state(STATE_UA_INITIATE, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_encode_state failed %d", err);
		return err;
	}

	err = nct_cc_send(&msg);
	nrf_cloud_free((void *)msg.data.ptr);

	nfsm_set_current_state_and_notify(STATE_UA_INITIATE, NULL);

	return err;
}

static int state_ua_input_wait(void)
{
	int err;
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = DEFAULT_REPORT_ID,
	};

	/* Publish report to the cloud on current status. */
	err = nrf_cloud_encode_state(STATE_UA_INPUT_WAIT, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_encode_state failed %d", err);
		return err;
	}

	err = nct_cc_send(&msg);
	nrf_cloud_free((void *)msg.data.ptr);

	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST,
		/* TODO: Extract instead of hard coding. */
		.param.ua_req.sequence.len = 6,
	};

	nfsm_set_current_state_and_notify(STATE_UA_INPUT_WAIT, &evt);

	return err;
}

static int state_ua_complete(void)
{
	int err;
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = PAIRING_STATUS_REPORT_ID,
	};

	err = nrf_cloud_encode_state(STATE_UA_COMPLETE, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_encode_state failed %d", err);
		return err;
	}

	err = nct_cc_send(&msg);
	nrf_cloud_free((void *)msg.data.ptr);

	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_USER_ASSOCIATED,
	};

	nfsm_set_current_state_and_notify(STATE_UA_COMPLETE, &evt);

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
		.param.ua_req.sequence.len = 0,
	};

	nfsm_set_current_state_and_notify(STATE_UA_PIN_WAIT, &evt);

	return 0;
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

static int state_ua_mismatch(void)
{
	int err;
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = DEFAULT_REPORT_ID,
	};

	/* Publish report to the cloud on current status. */
	err = nrf_cloud_encode_state(STATE_UA_INPUT_MISMATCH, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_encode_state failed %d", err);
		return err;
	}

	err = nct_cc_send(&msg);
	nrf_cloud_free((void *)msg.data.ptr);

	/* Not setting the state to error to allow retries. */
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST,
		/* TODO: Extract instead of hard coding. */
		.param.ua_req.sequence.len = 6,
	};

	nfsm_set_current_state_and_notify(STATE_UA_INPUT_WAIT, &evt);

	return err;
}

static int drop_event_handler(const struct nct_evt *nct_evt)
{
	LOG_DBG("Dropping FSM transition %d", nct_evt->type);
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

/**@brief  Handles incoming data on the control channel in the
 * STATE_CC_CONNECTED.
 *
 * @details This handler allows transition to one of the following states.
 *          a. STATE_UA_INITIATE.
 *          c. STATE_UA_COMPLETE.
 *          STATE_PATTERN_MISMATCH and STATE_PATTERN_TIMEOUT are not handled.
 */
static int initiate_n_complete_request_handler(const struct nct_evt *nct_evt)
{
	int err;
	enum nfsm_state expected_state;
	const struct nrf_cloud_data *payload = &nct_evt->param.cc->data;

	err = nrf_cloud_decode_requested_state(payload, &expected_state);
	if (err) {
		LOG_ERR("nrf_cloud_decode_requested_state failed %d", err);
		return err;
	}

	/* Validate expected state and take appropriate action. */
	switch (expected_state) {
	case STATE_UA_PIN_WAIT: {
		return state_ua_pin_wait();
	}
	case STATE_UA_PIN_COMPLETE: {
		struct nrf_cloud_data rx;
		struct nrf_cloud_data tx;
		struct nrf_cloud_data m_endpoint;

		err = nrf_cloud_decode_data_endpoint(payload, &tx, &rx,
						     &m_endpoint);
		if (err) {
			LOG_ERR("nrf_cloud_decode_data_endpoint failed %d",
				err);
			return err;
		}

		/* Set the endpoint information. */
		nct_dc_endpoint_set(&tx, &rx, &m_endpoint);

		return state_ua_pin_complete();
	}
	case STATE_UA_INITIATE: {
		return state_ua_initiate();
	}
	case STATE_UA_INPUT_WAIT: {
		return state_ua_input_wait();
	}
	case STATE_UA_COMPLETE: {
		struct nrf_cloud_data rx;
		struct nrf_cloud_data tx;

		err = nrf_cloud_decode_data_endpoint(payload, &tx, &rx, NULL);
		if (err) {
			LOG_ERR("nrf_cloud_decode_data_endpoint failed %d",
				err);
			return err;
		}

		/* Set the endpoint information. */
		nct_dc_endpoint_set(&tx, &rx, NULL);

		return state_ua_complete();
	}
	default: {
		/* Any other state is ignored. */
		return 0;
	}
	}
}

static int all_ua_request_handler(const struct nct_evt *nct_evt)
{
	int err;
	enum nfsm_state expected_state;
	const struct nrf_cloud_data *payload = &nct_evt->param.cc->data;

	err = nrf_cloud_decode_requested_state(payload, &expected_state);
	if (err) {
		LOG_ERR("nrf_cloud_decode_requested_state Failed %d", err);
		return err;
	}

	/* Validate expected state and take appropriate action. */
	switch (expected_state) {
	case STATE_UA_PIN_WAIT: {
		return state_ua_pin_wait();
	}
	case STATE_UA_PIN_COMPLETE: {
		struct nrf_cloud_data rx;
		struct nrf_cloud_data tx;
		struct nrf_cloud_data m_endpoint;

		err = nrf_cloud_decode_data_endpoint(payload, &tx, &rx,
						     &m_endpoint);
		if (err) {
			LOG_ERR("nrf_cloud_decode_data_endpoint failed %d",
				err);
			return err;
		}

		/* Set the endpoint information. */
		nct_dc_endpoint_set(&tx, &rx, &m_endpoint);

		err = state_ua_pin_complete();
		if (err) {
			LOG_ERR("state_ua_pin_complete failed %d", err);
			return err;
		}

		/* Disconnect the link. Must connect back. */
		(void)nct_disconnect();

		return err;
	}
	case STATE_UA_INITIATE: {
		return state_ua_initiate();
	}
	case STATE_UA_INPUT_WAIT: {
		return state_ua_input_wait();
	}
	case STATE_UA_COMPLETE: {
		struct nrf_cloud_data rx;
		struct nrf_cloud_data tx;

		err = nrf_cloud_decode_data_endpoint(payload, &tx, &rx, NULL);
		if (err) {
			LOG_ERR("nrf_cloud_decode_data_endpoint Failed %d",
				err);
			return err;
		}
		nct_dc_endpoint_set(&tx, &rx, NULL);

		err = state_ua_complete();
		/* Disconnect the link. Must connect back. */
		(void) nct_disconnect();
		return err;
	}
	case STATE_UA_INPUT_MISMATCH: {
		return state_ua_mismatch();
	}
	case STATE_UA_INPUT_TIMEOUT: {
		/* Not allowing retry on timeout. */
		const struct nrf_cloud_evt evt = {
			.type = NRF_CLOUD_EVT_ERROR,
		};
		nfsm_set_current_state_and_notify(STATE_ERROR, &evt);
		return 0;
	}
	default: {
		/* Any other state is ignored. */
		return 0;
	}
	}
}

static int initiate_cmd_handler(const struct nct_evt *nct_evt)
{
	int err;
	enum nfsm_state expected_state;
	const struct nrf_cloud_data *payload = &nct_evt->param.cc->data;

	err = nrf_cloud_decode_requested_state(payload, &expected_state);
	if (err) {
		LOG_ERR("nrf_cloud_decode_requested_state Failed %d", err);
		return err;
	}

	/* Validate expected state and take appropriate action. */
	if (expected_state == STATE_UA_INITIATE) {
		return state_ua_initiate();
	} else if (expected_state == STATE_UA_PIN_WAIT) {
		return state_ua_pin_wait();
	}

	/* Any other state is ignored. */
	return 0;
}

static int initiate_cmd_in_dc_conn_handler(const struct nct_evt *nct_evt)
{
	int err;
	enum nfsm_state expected_state;
	const struct nrf_cloud_data *payload = &nct_evt->param.cc->data;

	err = nrf_cloud_decode_requested_state(payload, &expected_state);
	if (err) {
		LOG_ERR("nrf_cloud_decode_requested_state Failed %d", err);
		return err;
	}

	/* Validate expected state and take appropriate action. */
	if (expected_state == STATE_UA_INITIATE) {
		/* Disconnect data channel to stop further data transfer. */
		(void) nct_dc_disconnect();
		return state_ua_initiate();
	} else if (expected_state == STATE_UA_PIN_WAIT) {
		(void) nct_dc_disconnect();
		return state_ua_pin_wait();
	}

	/* Any other state is ignored. */
	return 0;
}

static int cc_tx_cnf_handler(const struct nct_evt *nct_evt)
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

static int cc_tx_cnf_in_state_requested_handler(const struct nct_evt *nct_evt)
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
		.param.data = nct_evt->param.dc->data,
	};

	/* All data is forwared to the app */
	nfsm_set_current_state_and_notify(nfsm_get_current_state(), &cloud_evt);

	return 0;
}

static int dc_tx_cnf_handler(const struct nct_evt *nct_evt)
{
	return 0; /* Nothing to do */
}

static int dc_disconnection_handler(const struct nct_evt *nct_evt)
{
	return 0; /* Nothing to do */
}

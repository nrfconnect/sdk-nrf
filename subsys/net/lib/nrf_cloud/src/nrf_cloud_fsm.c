/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_fsm.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_transport.h"
#include <zephyr/kernel.h>
#include <net/nrf_cloud_alert.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_log.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_NRF_CLOUD_AGNSS)
#include <net/nrf_cloud_agnss.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif

LOG_MODULE_REGISTER(nrf_cloud_fsm, CONFIG_NRF_CLOUD_LOG_LEVEL);

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

static const fsm_transition idle_fsm_transition[NCT_EVT_TOTAL] = {
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

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
	[NCT_EVT_PINGRESP] = cc_tx_ack_handler,
	[NCT_EVT_DC_RX_DATA] = dc_rx_data_handler,
	[NCT_EVT_DC_TX_DATA_ACK] = dc_tx_ack_handler,
	[NCT_EVT_CC_DISCONNECTED] = cc_disconnection_handler,
	[NCT_EVT_DC_DISCONNECTED] = dc_disconnection_handler,
	[NCT_EVT_DISCONNECTED] = disconnection_handler,
};

static const fsm_transition *state_event_handlers[] = {
	[STATE_IDLE] = idle_fsm_transition,
	[STATE_INITIALIZED] = initialized_fsm_transition,
	[STATE_CONNECTED] = connected_fsm_transition,
	[STATE_CC_CONNECTING] = cc_connecting_fsm_transition,
	[STATE_CC_CONNECTED] = cc_connected_fsm_transition,
	[STATE_CLOUD_STATE_REQUESTED] = cloud_requested_fsm_transition,
	[STATE_UA_PIN_WAIT] = cloud_requested_fsm_transition,
	[STATE_UA_PIN_COMPLETE] = ua_complete_fsm_transition,
	[STATE_DC_CONNECTING] = dc_connecting_fsm_transition,
	[STATE_DC_CONNECTED] = dc_connected_fsm_transition,
};
BUILD_ASSERT(ARRAY_SIZE(state_event_handlers) == STATE_TOTAL);

static bool persistent_session;
static bool jitp_association_wait;

/* Flag to track if the c2d topic was modified; if so, the desired section
 * in the shadow needs to be updated to prevent delta events.
 */
static bool c2d_topic_modified;

/* Flag to indicate if the shadow info sections should be been sent.
 * This only needs to be done once per boot.
 * The user application is responsible for keeping dynamic data up to date.
 */
static bool add_shadow_info = IS_ENABLED(CONFIG_NRF_CLOUD_SEND_SHADOW_INFO);

#if defined(CONFIG_NRF_CLOUD_LOCATION) && defined(CONFIG_NRF_CLOUD_MQTT)
#if defined(CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST)
static char anchor_list_buf[CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST_BUFFER_SIZE];
#endif
static nrf_cloud_location_response_t location_cb;
void nfsm_set_location_response_cb(nrf_cloud_location_response_t cb)
{
	location_cb = cb;
}
#endif

int nfsm_init(void)
{
	persistent_session = false;
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
		.opcode = NCT_CC_OPCODE_UPDATE_ACCEPTED,
		.message_id = NCT_MSG_ID_STATE_REPORT,
	};

	/* Prevent duplicate application notifications of user association request
	 * events. They are unnecessary and caused by various background shadow
	 * updates.
	 */
	if (!jitp_association_wait) {
		jitp_association_wait = true;
		add_shadow_info = IS_ENABLED(CONFIG_NRF_CLOUD_SEND_SHADOW_INFO);
		(void)nct_save_session_state(false);
		(void)nct_set_keepalive(CONFIG_NRF_CLOUD_MQTT_UA_WAIT_KEEPALIVE);
	} else {
		LOG_DBG("Ignoring duplicate STATE_UA_PIN_WAIT");
		return 0;
	}

	/* Update the shadow for the current state: STATE_UA_PIN_WAIT */
	err = nrf_cloud_state_encode(STATE_UA_PIN_WAIT, false, false, &msg.data);
	if (err < 0) {
		LOG_ERR("nrf_cloud_state_encode failed %d", err);
		return err;
	} else if (err == 0) {
		err = nct_cc_send(&msg);

		nrf_cloud_free((void *)msg.data.ptr);

		if (err) {
			LOG_ERR("nct_cc_send failed %d", err);
			return err;
		}
	}

	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST,
	};

	nfsm_set_current_state_and_notify(STATE_UA_PIN_WAIT, &evt);

	return 0;
}

static int state_ua_pin_complete(void)
{
	int err;
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_ACCEPTED,
		.message_id = NCT_MSG_ID_PAIR_STATUS_REPORT,
	};

	err = nrf_cloud_state_encode(STATE_UA_PIN_COMPLETE, c2d_topic_modified,
				     add_shadow_info, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_state_encode failed %d", err);
		return err;
	}

	add_shadow_info = false;
	c2d_topic_modified = false;
	if (jitp_association_wait) {
		jitp_association_wait = false;
		(void)nct_set_keepalive(CONFIG_NRF_CLOUD_MQTT_KEEPALIVE);
	}

	err = nct_cc_send(&msg);

	nrf_cloud_free((void *)msg.data.ptr);

	if (err) {
		LOG_ERR("nct_cc_send failed %d", err);
		return err;
	}

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
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_TRANSPORT_CONNECTED,
		.status = nct_evt->status
	};

	/* Notify the application of the connection event.
	 * State transitions according to the event result.
	 */
	if (nct_evt->status != NRF_CLOUD_ERR_STATUS_NONE) {
		evt.type = NRF_CLOUD_EVT_ERROR;
		evt.status = nct_evt->status;
		nfsm_set_current_state_and_notify(nfsm_get_current_state(),
						  &evt);
		return 0;
	}

	/* For a connected event, status indicates if a persistent session is present */
	evt.status = nct_evt->param.flag;
	nfsm_set_current_state_and_notify(STATE_CONNECTED, &evt);

	/* Connect the control channel now. */
	persistent_session = nct_evt->param.flag;
	if (!persistent_session) {
		err = nct_cc_connect();
		if (err) {
			return err;
		}
		nfsm_set_current_state_and_notify(STATE_CC_CONNECTING, NULL);
	} else {
		struct nct_evt nevt = { .type = NCT_EVT_CC_CONNECTED,
					.status = 0 };

		LOG_DBG("Previous session valid; skipping nct_cc_connect()");
		nfsm_handle_incoming_event(&nevt, STATE_CC_CONNECTING);
	}


	return 0;
}

static int disconnection_handler(const struct nct_evt *nct_evt)
{
	/* Set the state to INITIALIZED and notify the application of
	 * disconnection.
	 */
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED,
		.status = NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE,
	};

	if (nfsm_get_disconnect_requested()) {
		evt.status = NRF_CLOUD_DISCONNECT_USER_REQUEST;
	}

	nfsm_set_current_state_and_notify(STATE_INITIALIZED, &evt);

	return 0;
}

static int cc_connection_handler(const struct nct_evt *nct_evt)
{
	/* Set the state according to the status of the event.
	 * If status the connection, request state synchronization.
	 */
	const struct nct_cc_data get_request = {
		.opcode = NCT_CC_OPCODE_GET_REQ,
		.message_id = NCT_MSG_ID_STATE_REQUEST,
	};
	int err;
	const struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_ERROR,
		.status = nct_evt->status
	};

	if (nct_evt->status != NRF_CLOUD_ERR_STATUS_NONE) {
		/* Send error event and initiate disconnect */
		nfsm_set_current_state_and_notify(nfsm_get_current_state(), &evt);
		(void)nct_dc_disconnect();
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

static int set_endpoint_data(const struct nrf_cloud_obj_shadow_data *const input)
{
	int err;
	struct nct_dc_endpoints eps;
	struct nrf_cloud_obj *desired_obj = NULL;

	if (input->type == NRF_CLOUD_OBJ_SHADOW_TYPE_ACCEPTED) {
		desired_obj = &input->accepted->desired;
	} else if (input->type == NRF_CLOUD_OBJ_SHADOW_TYPE_DELTA) {
		desired_obj = &input->delta->state;
	} else {
		return -ENOTSUP;
	}

	err = nrf_cloud_obj_endpoint_decode(desired_obj, &eps);
	if (err) {
		LOG_ERR("nrf_cloud_obj_endpoint_decode failed %d", err);
		return err;
	}

	/* Update to use wildcard topic if necessary */
	c2d_topic_modified = nrf_cloud_set_wildcard_c2d_topic((char *)eps.e[DC_RX].utf8,
							      eps.e[DC_RX].size);

	/* Set the endpoint information. */
	nct_dc_endpoint_set(&eps);

	return 0;
}

static void shadow_control_process(struct nrf_cloud_obj_shadow_data *const input)
{
	if (input->type == NRF_CLOUD_OBJ_SHADOW_TYPE_TF) {
		return;
	}

	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_ACCEPTED,
		.message_id = NCT_MSG_ID_STATE_REPORT
	};
	int err = nrf_cloud_shadow_control_process(input, &msg.data);

	if (err == -ENODATA) {
		return;
	} else if (err == -ENOMSG) {
		LOG_DBG("No reply needed for device control shadow update");
		return;
	} else if (err) {
		LOG_ERR("Failed to process device control shadow update, error: err");
		return;
	}

	LOG_DBG("Confirming device control in shadow: %s", (const char *)msg.data.ptr);
	err = nct_cc_send(&msg);
	nrf_cloud_free((void *)msg.data.ptr);
	if (err) {
		LOG_ERR("nct_cc_send failed %d", err);
	}
}

static void accept_associated_or_wait_state(enum nfsm_state cur_state, enum nfsm_state new_state,
					    bool *const accept_state, bool *const do_discon)
{
	if (!accept_state || !do_discon) {
		return;
	}

	switch (cur_state) {
	case STATE_CC_CONNECTED:
	case STATE_CLOUD_STATE_REQUESTED:
	case STATE_UA_PIN_WAIT:
	case STATE_UA_PIN_COMPLETE:
		if (new_state == STATE_UA_PIN_COMPLETE) {
			*accept_state = true;
			return;
		} else if (new_state == STATE_UA_PIN_WAIT) {
			*accept_state = true;
			return;
		}
		break;
	case STATE_DC_CONNECTING:
	case STATE_DC_CONNECTED:
		if (new_state == STATE_UA_PIN_WAIT) {
			*accept_state = true;
			*do_discon = true;
		}
		break;
	default:
		break;
	}

	*accept_state = false;
	*do_discon = false;
}

static int cc_rx_data_handler(const struct nct_evt *nct_evt)
{
	__ASSERT_NO_MSG(nct_evt != NULL);
	__ASSERT_NO_MSG(nct_evt->param.cc != NULL);

	int err = 0;
	bool accept = false;
	bool discon = false;
	enum nfsm_state new_state;
	const enum nfsm_state current_state = nfsm_get_current_state();
	struct nrf_cloud_obj_shadow_accepted shadow_accepted = {0};
	struct nrf_cloud_obj_shadow_delta shadow_delta = {0};
	struct nrf_cloud_obj_shadow_transform shadow_tf = {0};
	struct nrf_cloud_obj_shadow_data shadow_data = {0};

	NRF_CLOUD_OBJ_JSON_DEFINE(shadow_obj);

	LOG_DBG("CC RX on topic [%d] %.*s: %s",
		nct_evt->param.cc->opcode,
		nct_evt->param.cc->topic.len,
		(const char *)nct_evt->param.cc->topic.ptr,
		(const char *)nct_evt->param.cc->data.ptr);

	switch (nct_evt->param.cc->opcode) {
	case NCT_CC_OPCODE_UPDATE_ACCEPTED:
	case NCT_CC_OPCODE_UPDATE_DELTA:
#if defined(CONFIG_NRF_CLOUD_MQTT_SHADOW_TRANSFORMS)
	case NCT_CC_OPCODE_TRANSFORM:
#endif
		break;
	case NCT_CC_OPCODE_UPDATE_REJECTED:
		LOG_DBG("Rejected shadow: %s", (char *)nct_evt->param.cc->data.ptr);
		__fallthrough;
	default:
		/* No need to process */
		return 0;
	}

	/* Decode input data */
	err = nrf_cloud_obj_input_decode(&shadow_obj, &nct_evt->param.cc->data);
	if (err) {
		LOG_ERR("Error decoding shadow data, error: %d", err);
		return -ENOMSG;
	}

	/* Decode data based on the topic */
	if (nct_evt->param.cc->opcode == NCT_CC_OPCODE_UPDATE_ACCEPTED) {
		err = nrf_cloud_obj_shadow_accepted_decode(&shadow_obj, &shadow_accepted);
		if (!err) {
			shadow_data.type = NRF_CLOUD_OBJ_SHADOW_TYPE_ACCEPTED;
			shadow_data.accepted = &shadow_accepted;
			LOG_DBG("Accepted shadow decoded");
		}
	} else if (nct_evt->param.cc->opcode == NCT_CC_OPCODE_UPDATE_DELTA) {
		err = nrf_cloud_obj_shadow_delta_decode(&shadow_obj, &shadow_delta);
		if (!err) {
			shadow_data.type = NRF_CLOUD_OBJ_SHADOW_TYPE_DELTA;
			shadow_data.delta = &shadow_delta;
			LOG_DBG("Delta shadow decoded");
		}
#if defined(CONFIG_NRF_CLOUD_MQTT_SHADOW_TRANSFORMS)
	} else if (nct_evt->param.cc->opcode == NCT_CC_OPCODE_TRANSFORM) {
		err = nrf_cloud_obj_shadow_transform_decode(&shadow_obj, &shadow_tf);
		if (!err) {
			shadow_data.type = NRF_CLOUD_OBJ_SHADOW_TYPE_TF;
			shadow_data.transform = &shadow_tf;
			LOG_DBG("Transform result received");
		}
#endif
	}

	nrf_cloud_obj_free(&shadow_obj);

	if (err) {
		return 0;
	}

	/* Check for (device/cloud association) state data */
	err = nrf_cloud_shadow_data_state_decode(&shadow_data, &new_state);
	if (!err) {
		accept_associated_or_wait_state(current_state, new_state, &accept, &discon);
	}

	/* If accepting the association complete state, set the endpoint data */
	if (accept && (new_state == STATE_UA_PIN_COMPLETE)) {
		/* Get and set endpoint data */
		(void)set_endpoint_data(&shadow_data);
	}

	/* Process control data */
	(void)shadow_control_process(&shadow_data);

	/* Check if data should be sent to the application */
	if (nrf_cloud_shadow_app_send_check(&shadow_data)) {
		/* Include the decoded shadow_data along with the original data */
		struct nrf_cloud_evt cloud_evt = {
			.type = NRF_CLOUD_EVT_RX_DATA_SHADOW,
			.data = nct_evt->param.cc->data,
			.topic = nct_evt->param.cc->topic,
			.shadow = &shadow_data
		};

		nfsm_set_current_state_and_notify(current_state, &cloud_evt);
	}

	/* Free the shadow objects */
	nrf_cloud_obj_shadow_accepted_free(&shadow_accepted);
	nrf_cloud_obj_shadow_delta_free(&shadow_delta);
	nrf_cloud_obj_shadow_transform_free(&shadow_tf);

	if (!accept) {
		/* The new state is not accepted */
		return 0;
	}

	LOG_DBG("New state: %d", new_state);

	if (new_state == STATE_UA_PIN_COMPLETE) {
		return state_ua_pin_complete();
	} else if (new_state == STATE_UA_PIN_WAIT) {
		if (discon) {
			/* Device was just removed from its nRF Cloud account. */
			(void)nct_dc_disconnect();
		}
		return state_ua_pin_wait();
	}

	return 0;
}

static int cc_tx_ack_handler(const struct nct_evt *nct_evt)
{
	int err;

	if (nct_evt->param.message_id == NCT_MSG_ID_STATE_REQUEST) {
		nfsm_set_current_state_and_notify(STATE_CLOUD_STATE_REQUESTED,
						  NULL);
		return 0;
	} else if (nct_evt->param.message_id == NCT_MSG_ID_PAIR_STATUS_REPORT) {
		if (!persistent_session) {
			err = nct_dc_connect();
			if (err) {
				return err;
			}

			nfsm_set_current_state_and_notify(STATE_DC_CONNECTING,
							  NULL);
		} else {
			struct nct_evt nevt = { .type = NCT_EVT_DC_CONNECTED,
						.status = 0 };

			LOG_DBG("Previous session valid; skipping nct_dc_connect()");
			nfsm_handle_incoming_event(&nevt, STATE_DC_CONNECTING);
		}
	} else if (nct_evt->type == NCT_EVT_PINGRESP) {
		struct nrf_cloud_evt evt = {
			.type = NRF_CLOUD_EVT_PINGRESP,
		};

		nfsm_set_current_state_and_notify(nfsm_get_current_state(), &evt);
	} else if (IS_VALID_USER_TAG(nct_evt->param.message_id)) {
		struct nrf_cloud_evt evt = {
			.type = NRF_CLOUD_EVT_SENSOR_DATA_ACK,
			.data.len = sizeof(nct_evt->param.message_id),
			.data.ptr = &nct_evt->param.message_id
		};

		LOG_DBG("Data ACK for user tag: %u", nct_evt->param.message_id);
		nfsm_set_current_state_and_notify(nfsm_get_current_state(), &evt);
	}

	return 0;
}

static int cc_tx_ack_in_state_requested_handler(const struct nct_evt *nct_evt)
{
	if (nct_evt->param.message_id == NCT_MSG_ID_STATE_REQUEST) {
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
	struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_READY,
		.status = nct_evt->status
	};

	if (nct_evt->status != NRF_CLOUD_ERR_STATUS_NONE) {
		evt.type = NRF_CLOUD_EVT_ERROR;
		nfsm_set_current_state_and_notify(nfsm_get_current_state(), &evt);
	} else {
		nfsm_set_current_state_and_notify(STATE_DC_CONNECTED, &evt);
	}

	(void)nrf_cloud_print_cloud_details();
	return 0;
}

static void agnss_process(const char * const buf, const size_t buf_len)
{
#if defined(CONFIG_NRF_CLOUD_AGNSS)
	int ret = nrf_cloud_agnss_process(buf, buf_len);

	if (ret) {
		struct nrf_cloud_evt evt = {
			.type = NRF_CLOUD_EVT_ERROR,
			.status = NRF_CLOUD_ERR_STATUS_AGNSS_PROC
		};

		LOG_ERR("Error processing A-GNSS data: %d", ret);
		nfsm_set_current_state_and_notify(nfsm_get_current_state(), &evt);
	} else {
		LOG_DBG("A-GNSS data processed");
	}

#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* If both A-GNSS and P-GPS are enabled, everything but ephemerides and almanacs
	 * are handled by A-GNSS.
	 * In this configuration it is important to check, after receiving A-GNSS data,
	 * whether any further assistance is needed by the modem for ephemerides,
	 * which would come from P-GPS (usually, in stored predictions in flash).
	 */
	if (ret == 0) {
		ret = nrf_cloud_pgps_notify_prediction();
		if (ret) {
			LOG_ERR("Error requesting P-GPS notification: %d", ret);
		}
	}
#endif
#endif
}

static void pgps_process(const char * const buf, const size_t buf_len)
{
#if defined(CONFIG_NRF_CLOUD_PGPS)
	int ret = nrf_cloud_pgps_process(buf, buf_len);

	if (ret) {
		struct nrf_cloud_evt evt = {
			.type = NRF_CLOUD_EVT_ERROR,
			.status = NRF_CLOUD_ERR_STATUS_PGPS_PROC
		};

		LOG_ERR("Error processing P-GPS data: %d", ret);
		nfsm_set_current_state_and_notify(nfsm_get_current_state(), &evt);
	} else {
		LOG_DBG("P-GPS data processed");
	}
#endif
}

static int location_process(const char * const buf)
{
#if defined(CONFIG_NRF_CLOUD_LOCATION) && defined(CONFIG_NRF_CLOUD_MQTT)
	if (location_cb) {

		int ret;
		struct nrf_cloud_location_result res = {0};

#if defined(CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST)
		res.anchor_buf = anchor_list_buf;
		res.anchor_buf_sz = sizeof(anchor_list_buf);
#endif

		ret = nrf_cloud_location_process(buf, &res);
		if (ret <= 0) {
			/* A location response was received, send to callback */
			location_cb(&res);

			LOG_DBG("Location data sent to provided callback");

			/* Clear the callback after use */
			nfsm_set_location_response_cb(NULL);
			return 0;
		}
		/* ret == 1 indicates that no location data was found */
	}
#endif
	return -ENOSYS;
}

static int dc_rx_data_handler(const struct nct_evt *nct_evt)
{
	__ASSERT_NO_MSG(nct_evt != NULL);
	__ASSERT_NO_MSG(nct_evt->param.dc != NULL);

	bool discon_req = false;

	struct nrf_cloud_evt cloud_evt = {
		.data = nct_evt->param.dc->data,
		.topic = nct_evt->param.dc->topic,
	};

	switch (nrf_cloud_dc_rx_topic_decode(cloud_evt.topic.ptr)) {
	case NRF_CLOUD_RCV_TOPIC_AGNSS:
		agnss_process(cloud_evt.data.ptr, cloud_evt.data.len);
		return 0;
	case NRF_CLOUD_RCV_TOPIC_PGPS:
		pgps_process(cloud_evt.data.ptr, cloud_evt.data.len);
		return 0;
	case NRF_CLOUD_RCV_TOPIC_LOCATION:
		if (location_process(cloud_evt.data.ptr) == 0) {
			/* Data was sent to cb, do not send to application */
			return 0;
		}
		cloud_evt.type = NRF_CLOUD_EVT_RX_DATA_LOCATION;
		break;
	case NRF_CLOUD_RCV_TOPIC_UNKNOWN:
		LOG_DBG("Received data on unknown topic: %s",
			(char *)(cloud_evt.topic.ptr ? cloud_evt.topic.ptr : "NULL"));
		/* Intentional fall-through */
	case NRF_CLOUD_RCV_TOPIC_GENERAL:
	default:
		discon_req = nrf_cloud_disconnection_request_decode(cloud_evt.data.ptr);
		if (discon_req) {
			cloud_evt.type = NRF_CLOUD_EVT_RX_DATA_DISCON;
		} else {
			cloud_evt.type = NRF_CLOUD_EVT_RX_DATA_GENERAL;
		}
		break;
	}

	nfsm_set_current_state_and_notify(nfsm_get_current_state(), &cloud_evt);

	if (discon_req) {
		LOG_INF("Device deleted from nRF Cloud");
		int err = nrf_cloud_disconnect();

		if (err < 0) {
			LOG_ERR("nRF Cloud disconnection-on-delete failure, error: %d", err);
		}
	}

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

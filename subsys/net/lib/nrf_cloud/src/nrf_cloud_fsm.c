/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_fsm.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_mem.h"
#include <zephyr/kernel.h>
#include <net/nrf_cloud_alert.h>
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
static int handle_pin_complete(const struct nct_evt *nct_evt);

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

/* Flag to track if the c2d topic was modified; if so, the desired section
 * in the shadow needs to be updated to prevent delta events.
 */
static bool c2d_topic_modified;

/* Flag to indicate if the device status has been added to the shadow.
 * This only needs to be done once per boot.
 * The user application is responsible for keeping dynamic data up to date.
 */
static bool add_dev_status = IS_ENABLED(CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS);

#if defined(CONFIG_NRF_CLOUD_LOCATION) && defined(CONFIG_NRF_CLOUD_MQTT)
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
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.message_id = NCT_MSG_ID_STATE_REPORT,
	};

	/* Publish report to the cloud on current status. */
	err = nrf_cloud_state_encode(STATE_UA_PIN_WAIT, false, add_dev_status, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_state_encode failed %d", err);
		return err;
	}

	add_dev_status = false;

	err = nct_cc_send(&msg);

	nrf_cloud_free((void *)msg.data.ptr);

	if (err) {
		LOG_ERR("nct_cc_send failed %d", err);
		return err;
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
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.message_id = NCT_MSG_ID_PAIR_STATUS_REPORT,
	};

	err = nrf_cloud_state_encode(STATE_UA_PIN_COMPLETE, c2d_topic_modified,
				     add_dev_status, &msg.data);
	if (err) {
		LOG_ERR("nrf_cloud_state_encode failed %d", err);
		return err;
	}

	add_dev_status = false;

	c2d_topic_modified = false;

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

static int handle_pin_complete(const struct nct_evt *nct_evt)
{
	int err;
	const struct nrf_cloud_data *payload = &nct_evt->param.cc->data;
	struct nrf_cloud_data rx;
	struct nrf_cloud_data tx;
	struct nrf_cloud_data bulk;
	struct nrf_cloud_data bin;
	struct nrf_cloud_data endpoint;

	err = nrf_cloud_data_endpoint_decode(payload, &tx, &rx, &bulk, &bin, &endpoint);
	if (err) {
		LOG_ERR("nrf_cloud_data_endpoint_decode failed %d", err);
		return err;
	}

	/* Update to use wildcard topic if necessary */
	c2d_topic_modified = nrf_cloud_set_wildcard_c2d_topic((char *)rx.ptr, rx.len);

	/* Set the endpoint information. */
	nct_dc_endpoint_set(&tx, &rx, &bulk, &bin, &endpoint);

	return state_ua_pin_complete();
}

static int cc_rx_data_handler(const struct nct_evt *nct_evt)
{
	__ASSERT_NO_MSG(nct_evt != NULL);
	__ASSERT_NO_MSG(nct_evt->param.cc != NULL);
	int err;
	enum nfsm_state new_state;
	const struct nrf_cloud_data *payload = &nct_evt->param.cc->data;
	const enum nfsm_state current_state = nfsm_get_current_state();
	struct nct_cc_data msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.message_id = NCT_MSG_ID_STATE_REPORT
	};
	enum nrf_cloud_ctrl_status status;

	LOG_DBG("CC RX on topic %*s: %s",
		nct_evt->param.cc->topic.len,
		(const char *)nct_evt->param.cc->topic.ptr,
		(const char *)nct_evt->param.cc->data.ptr);

	err = nrf_cloud_device_control_update(&nct_evt->param.cc->data, &msg.data,
					      &status);
	if (err) {
		LOG_ERR("Error updating device control: %d", err);
	} else if (msg.data.ptr) {
		LOG_DBG("Confirming shadow: %s", (const char *)msg.data.ptr);
		err = nct_cc_send(&msg);
		nrf_cloud_free((void *)msg.data.ptr);
		if (err) {
			LOG_ERR("nct_cc_send failed %d", err);
		}
	}

	struct nrf_cloud_evt cloud_evt = {
		.type = NRF_CLOUD_EVT_RX_DATA_SHADOW,
		.data = nct_evt->param.cc->data,
		.topic = nct_evt->param.cc->topic
	};

	/* Give application a chance to see the change to the shadow. */
	nfsm_set_current_state_and_notify(current_state, &cloud_evt);

	err = nrf_cloud_requested_state_decode(payload, &new_state);
	if (err) {
		LOG_DBG("Error %d when decoding requested state; current:%d",
			err, current_state);
		return 0;
	}

	LOG_DBG("New state: %d", new_state);

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
			/* Device was just removed from its nRF Cloud account. */
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
		struct nrf_cloud_location_result res;
		int ret = nrf_cloud_location_process(buf, &res);

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
	case NRF_CLOUD_RCV_TOPIC_AGPS:
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
		cloud_evt.type = NRF_CLOUD_EVT_RX_DATA_GENERAL;
		discon_req = nrf_cloud_disconnection_request_decode(cloud_evt.data.ptr);
		break;
	}

	nfsm_set_current_state_and_notify(nfsm_get_current_state(), &cloud_evt);

	if (discon_req) {
		LOG_DBG("Device deleted from nRF Cloud");
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

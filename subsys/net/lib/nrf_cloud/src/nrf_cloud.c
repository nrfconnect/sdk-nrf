/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <nrf_cloud.h>
#include "nrf_cloud_codec.h"
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_mem.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud, CONFIG_NRF_CLOUD_LOG_LEVEL);

/* Validates if the API was requested in the right state. */
#define NOT_VALID_STATE(EXPECTED) ((EXPECTED) < m_current_state)

/* Identifier for user association message sent to the cloud.
 * A unique, unsigned 16 bit magic number. Can't be zero.
 */
#define CC_UA_DATA_ID 9547

/* Maintains the state with respect to the cloud. */
static volatile enum nfsm_state m_current_state = STATE_IDLE;

/* Handler registered by the application with the module to receive
 * asynchronous events.
 */
static nrf_cloud_event_handler_t m_event_handler;


enum nfsm_state nfsm_get_current_state(void)
{
	return m_current_state;
}

void nfsm_set_current_state_and_notify(enum nfsm_state state,
				       const struct nrf_cloud_evt *evt)
{
	LOG_DBG("state: %d", state);

	m_current_state = state;
	if ((m_event_handler != NULL) && (evt != NULL)) {
		m_event_handler(evt);
	}
}

int nrf_cloud_init(const struct nrf_cloud_init_param *param)
{
	int err;

	if (m_current_state != STATE_IDLE) {
		return -EACCES;
	}

	if (param->event_handler == NULL) {
		return -EINVAL;
	}

	/* Initialize the state machine. */
	err = nfsm_init();
	if (err) {
		return err;
	}
	/* Initialize the encoder, decoder unit. */
	err = nrf_codec_init();
	if (err) {
		return err;
	}

	/* Initialize the transport. */
	err = nct_init();
	if (err) {
		return err;
	}

	m_event_handler = param->event_handler;
	m_current_state = STATE_INITIALIZED;

	return 0;
}

int nrf_cloud_connect(const struct nrf_cloud_connect_param *param)
{
	if (NOT_VALID_STATE(STATE_INITIALIZED)) {
		return -EACCES;
	}
	return nct_connect();
}

int nrf_cloud_disconnect(void)
{
	if (NOT_VALID_STATE(STATE_CONNECTED)) {
		return -EACCES;
	}
	return nct_disconnect();
}

int nrf_cloud_user_associate(const struct nrf_cloud_ua_param *param)
{
	int err;

	if (param == NULL) {
		return -EINVAL;
	}

	if (NOT_VALID_STATE(STATE_UA_INPUT_WAIT)) {
		return -EACCES;
	}

	struct nct_cc_data ua_msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = CC_UA_DATA_ID
	};

	err = nrf_cloud_encode_ua(param, &ua_msg.data);
	if (err) {
		return err;
	}

	err = nct_cc_send(&ua_msg);
	nrf_cloud_free((void *)ua_msg.data.ptr);

	return err;
}

int nrf_cloud_sensor_attach(const struct nrf_cloud_sa_param *param)
{
	if (NOT_VALID_STATE(STATE_DC_CONNECTED)) {
		return -EACCES;
	}

	const struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_SENSOR_ATTACHED
	};

	nfsm_set_current_state_and_notify(STATE_DC_CONNECTED, &evt);

	return 0;
}

int nrf_cloud_sensor_data_send(const struct nrf_cloud_sensor_data *param)
{
	int err;
	struct nct_dc_data sensor_data;

	if (NOT_VALID_STATE(STATE_DC_CONNECTED)) {
		return -EACCES;
	}

	if (param == NULL) {
		return -EINVAL;
	}

	err = nrf_cloud_encode_sensor_data(param, &sensor_data.data);
	if (err) {
		return err;
	}

	sensor_data.id = param->tag;
	err = nct_dc_send(&sensor_data);
	nrf_cloud_free((void *)sensor_data.data.ptr);

	return err;
}

int nrf_cloud_sensor_data_stream(const struct nrf_cloud_sensor_data *param)
{
	int err;
	struct nct_dc_data sensor_data;

	if (NOT_VALID_STATE(STATE_DC_CONNECTED)) {
		return -EACCES;
	}

	if (param == NULL) {
		return -EINVAL;
	}

	err = nrf_cloud_encode_sensor_data(param, &sensor_data.data);
	if (err) {
		return err;
	}

	sensor_data.id = param->tag;
	err = nct_dc_stream(&sensor_data);
	nrf_cloud_free((void *)sensor_data.data.ptr);

	return err;
}

int nct_input(const struct nct_evt *evt)
{
	return nfsm_handle_incoming_event(evt, m_current_state);
}

void nrf_cloud_process(void)
{
	nct_process();
}

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_TRANSPORT_H__
#define NRF_CLOUD_TRANSPORT_H__

#include <stddef.h>
#include <net/nrf_cloud.h>

#ifdef __cplusplus
extern "C" {
#endif

enum nct_evt_type {
	NCT_EVT_CONNECTED,
	NCT_EVT_CC_CONNECTED,
	NCT_EVT_DC_CONNECTED,
	NCT_EVT_CC_RX_DATA,
	NCT_EVT_CC_TX_DATA_ACK,
	NCT_EVT_PINGRESP,
	NCT_EVT_DC_RX_DATA,
	NCT_EVT_DC_TX_DATA_ACK,
	NCT_EVT_CC_DISCONNECTED,
	NCT_EVT_DC_DISCONNECTED,
	NCT_EVT_DISCONNECTED,
	NCT_EVT_TOTAL,
};

enum nct_cc_opcode {
	NCT_CC_OPCODE_GET_REQ,
	NCT_CC_OPCODE_UPDATE_REQ,
	NCT_CC_OPCODE_UPDATE_REJECT_RSP,
	NCT_CC_OPCODE_UPDATE_ACCEPT_RSP,
};

struct nct_dc_data {
	struct nrf_cloud_data data;
	struct nrf_cloud_topic topic;
	uint16_t message_id;
};

struct nct_cc_data {
	struct nrf_cloud_data data;
	struct nrf_cloud_topic topic;
	uint16_t message_id;
	enum nct_cc_opcode opcode;
};

struct nct_evt {
	int32_t status;
	union {
		struct nct_cc_data *cc;
		struct nct_dc_data *dc;
		uint16_t message_id;
		uint8_t flag;
	} param;
	enum nct_evt_type type;
};

int nct_socket_get(void);

/**@brief Initialization routine for the transport. */
int nct_init(const char * const client_id);

/**@brief Unintialize the transport; reset state and free allocated memory */
void nct_uninit(void);

/**@brief Establishes the transport connection. */
int nct_connect(void);

/**@brief Establishes the logical control channel on the transport connection.
 */
int nct_cc_connect(void);

/**@brief Establishes the logical data channel on the transport connection. */
int nct_dc_connect(void);

/**@brief Sends data on the control channel. */
int nct_cc_send(const struct nct_cc_data *cc);

/**@brief Sends data on the data channel. Reliable, should expect a @ref
 * NCT_EVT_DC_TX_DATA_ACK event.
 */
int nct_dc_send(const struct nct_dc_data *dc);

/**@brief Stream data on the data channel. Unreliable, no @ref
 * NCT_EVT_DC_TX_DATA_ACK event is generated.
 */
int nct_dc_stream(const struct nct_dc_data *dc);

/** @brief Publish data on the bulk endpoint topic.
 *
 *  @param[in] dc_data Pointer to structure containing the data to be published.
 *  @param[in] qos MQTT Quality of Service level of the publication.
 *
 *  @return 0 If successful. Otherwise, a negative error code is returned.
 *  @retval -EINVAL if one or several of the passed in arguments are invalid.
 */
int nct_dc_bulk_send(const struct nct_dc_data *dc_data, enum mqtt_qos qos);

/**@brief Disconnects the logical control channel. */
int nct_cc_disconnect(void);

/**@brief Disconnects the logical data channel. */
int nct_dc_disconnect(void);

/**@brief Disconnects the transport. */
int nct_disconnect(void);

/**
 * @brief Set the endpoint information.
 *
 * @note This routine must be called before @ref nrf_dc_connect.
 */
void nct_dc_endpoint_set(const struct nrf_cloud_data *tx_endpoint,
			 const struct nrf_cloud_data *rx_endpoint,
			 const struct nrf_cloud_data *bulk_endpoint,
			 const struct nrf_cloud_data *m_endpoint);

/**
 * @brief Get the endpoint information.
 */
void nct_dc_endpoint_get(struct nrf_cloud_data *tx_endpoint,
			 struct nrf_cloud_data *rx_endpoint,
			 struct nrf_cloud_data *bulk_endpoint,
			 struct nrf_cloud_data *m_endpoint);

/**@brief Needed for keep alive. */
int nct_process(void);

/**
 * @brief Helper function to determine when next keep alive message should be
 *        sent. Can be used for instance as a source for `poll` timeout.
 *
 * @return Time in milliseconds until next keep alive message is expected to
 *         be sent.
 * @return -1 if keep alive messages are not enabled.
 */
int nct_keepalive_time_left(void);

/**@brief Input from the cloud module. */
int nct_input(const struct nct_evt *evt);

/**@brief Retrieve the device id. */
int nct_client_id_get(char *id, size_t id_len);

/**@brief Send cloud event to the application. */
void nct_send_event(const struct nrf_cloud_evt * const evt);

/**@brief Troubleshooting function to overrule the persistent session setting. */
int nct_save_session_state(const int session_valid);

/**@brief Troubleshooting function to determine if persistent sessions are on. */
int nct_get_session_state(void);

/**@brief Set the topic prefix for the rx and tx topics. String passed
 * must be in the form "<stage>/<tenantId>/". The topic strings
 * will be constructed.
 */
void nct_set_topic_prefix(const char *topic_prefix);

/**@brief Troubleshooting function to retrieve the current nrfcloud stage. */
int nct_stage_get(char *cur_stage, const int cur_stage_len);

/**@brief Troubleshooting function to query the current nrfcloud tenant ID. */
int nct_tenant_id_get(char *cur_tenant, const int cur_tenant_len);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_TRANSPORT_H__ */

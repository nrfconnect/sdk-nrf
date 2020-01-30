/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NRF_CLOUD_TRANSPORT_H__
#define NRF_CLOUD_TRANSPORT_H__

#include <nrf_cloud.h>

#ifdef __cplusplus
extern "C" {
#endif

enum nct_evt_type {
	NCT_EVT_CONNECTED,
	NCT_EVT_CC_CONNECTED,
	NCT_EVT_DC_CONNECTED,
	NCT_EVT_CC_RX_DATA,
	NCT_EVT_CC_TX_DATA_ACK,
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
	u32_t id;
};

struct nct_cc_data {
	struct nrf_cloud_data data;
	u32_t id;
	enum nct_cc_opcode opcode;
};

struct nct_evt {
	u32_t status;
	union {
		struct nct_cc_data *cc;
		struct nct_dc_data *dc;
		u32_t data_id;
	} param;
	enum nct_evt_type type;
};

int nct_socket_get(void);

/**@brief Initialization routine for the transport. */
int nct_init(void);

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
			 const struct nrf_cloud_data *m_endpoint);

/**
 * @brief Get the endpoint information.
 */
void nct_dc_endpoint_get(struct nrf_cloud_data *tx_endpoint,
			 struct nrf_cloud_data *rx_endpoint,
			 struct nrf_cloud_data *m_endpoint);

/**@brief Needed for keep alive. */
void nct_process(void);

/**
 * @brief Helper function to determine when next keep alive message should be
 *        sent. Can be used for instance as a source for `poll` timeout.
 */
int nct_keepalive_time_left(void);

/**@brief Input from the cloud module. */
int nct_input(const struct nct_evt *evt);

/**@brief Signal to apply FOTA update. */
void nct_apply_update(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_TRANSPORT_H__ */

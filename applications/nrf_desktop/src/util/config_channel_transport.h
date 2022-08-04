/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CONFIG_CHANNEL_TRANSPORT_H_
#define _CONFIG_CHANNEL_TRANSPORT_H_

/**
 * @file
 * @defgroup config_channel_transport Configuration channel transport
 * @{
 * @brief API for the configuration channel transport.
 */

#include "config_event.h"

/**
 * @brief Parse the configuration channel report.
 *
 * @param buffer Pointer to the report buffer to be parsed.
 * @param length Length of the buffer.
 * @param event  Pointer to the event used to store the parsed values.
 *
 * @return Number of parsed bytes if the operation was successful. Otherwise,
 *         a negative error code is returned.
 */
int config_channel_report_parse(const uint8_t *buffer, size_t length,
				struct config_event *event);

/**
 * @brief Fill the configuration channel report with
 *        values from a provided event.
 *
 * @param buffer Pointer to the report buffer to be filled.
 * @param length Length of the buffer.
 * @param event  Pointer to an event with values to be copied to the buffer.
 *
 * @return Number of written bytes if the operation was successful. Otherwise,
 *         a negative error code is returned.
 */
int config_channel_report_fill(uint8_t *buffer, const size_t length,
			       const struct config_event *event);

/** @brief Config channel transport states. */
enum config_channel_transport_state {
	CONFIG_CHANNEL_TRANSPORT_DISABLED,
	CONFIG_CHANNEL_TRANSPORT_IDLE,
	CONFIG_CHANNEL_TRANSPORT_WAIT_RSP,
	CONFIG_CHANNEL_TRANSPORT_RSP_READY
};

/** @brief Configuration channel transport. */
struct config_channel_transport {
	struct k_work_delayable timeout;
	size_t data_len;
	uint16_t transport_id;
	uint8_t data[REPORT_SIZE_USER_CONFIG];

	enum config_channel_transport_state state;
};

/** @brief Initialize the configuration channel transport instance.
 *
 *  @param transport Pointer to the configuration channel transport instance.
 */
void config_channel_transport_init(struct config_channel_transport *transport);

/**
 * @brief Handle a get operation on the configuration channel.
 *
 * @param transport Pointer to the configuration channel transport instance.
 * @param buffer    Pointer to the buffer to be filled when handling the get
 *                  request.
 * @param length    Length of the data to be filled.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int config_channel_transport_get(struct config_channel_transport *transport,
				 uint8_t *buffer, size_t length);

/**
 * @brief Handle a set operation on the configuration channel.
 *
 * @param transport Pointer to the configuration channel transport instance.
 * @param buffer    Pointer to the report buffer to be parsed to handle
 *                  the set request.
 * @param length    Length of the incoming data.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int config_channel_transport_set(struct config_channel_transport *transport,
				 const uint8_t *buffer, size_t length);

/**
 * @brief Handle a get operation and inform that the transport is disabled.
 *
 * Fills the buffer with response informing that the transport is disabled and cannot be used to
 * configure device. If a given transport is disabled, set operation should be ignored and get
 * operation should use this function to prepare response for host.
 *
 * The disabled transport returns disconnected status.
 *
 * @param buffer    Pointer to the buffer to be filled.
 * @param length    Length of the data to be filled.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int config_channel_transport_get_disabled(uint8_t *buffer, size_t length);

/**
 * @brief Handle the response received from higher layer.
 *
 * @param transport Pointer to the configuration channel transport instance.
 * @param event     Pointer to the event carrying the received response.
 *
 * @return true if response was designated for this transport. Otherwise returns
 *         false.
 */
bool config_channel_transport_rsp_receive(struct config_channel_transport *transport,
					  struct config_event *event);

/**
 * @brief Handle the configuration channel transport disconnection
 *
 * @param transport Pointer to the configuration channel transport instance
 */
void config_channel_transport_disconnect(struct config_channel_transport *transport);

/**
 * @}
 */

#endif /*_CONFIG_CHANNEL_TRANSPORT_H_ */

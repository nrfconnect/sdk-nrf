/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _UNICAST_SERVER_H_
#define _UNICAST_SERVER_H_

#include "bt_le_audio_tx.h"
#include "le_audio.h"

#include <audio_defines.h>

/**
 * @brief	Get configuration for audio stream.
 *
 * @param[in]	conn			Pointer to the connection to get the configuration for.
 * @param[in]	dir			Direction to get the configuration from.
 * @param[out]	bitrate			Pointer to the bit rate used; can be NULL.
 * @param[out]	sampling_rate_hz	Pointer to the sampling rate used; can be NULL.
 * @param[out]	pres_delay_us		Pointer to the presentation delay used; can be NULL. Only
 *					valid for the sink direction.
 *
 * @retval	0		Operation successful.
 * @retval	-ENXIO		The feature is disabled.
 */
int unicast_server_config_get(struct bt_conn *conn, enum bt_audio_dir dir, uint32_t *bitrate,
			      uint32_t *sampling_rate_hz, uint32_t *pres_delay_us);

/**
 * @brief	Put the UUIDs from this module into the buffer.
 *
 * @note	This partial data is used to build a complete extended advertising packet.
 *
 * @param[out]	uuid_buf	Buffer being populated with UUIDs.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_server_uuid_populate(struct net_buf_simple *uuid_buf);

/**
 * @brief	Put the advertising data from this module into the buffer.
 *
 * @note	This partial data is used to build a complete extended advertising packet.
 *
 * @param[out]	adv_buf		Buffer being populated with ext adv elements.
 * @param[in]	adv_buf_vacant	Number of vacant elements in @p adv_buf.
 *
 * @return	Negative values for errors or number of elements added to @p adv_buf.
 */
int unicast_server_adv_populate(struct bt_data *adv_buf, uint8_t adv_buf_vacant);

/**
 * @brief	Send data from the LE Audio unicast (CIS) server, if configured as a source.
 *
 * @param[in]	enc_audio	Encoded audio struct.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_server_send(struct le_audio_encoded_audio enc_audio);

/**
 * @brief	Disable the Bluetooth LE Audio unicast (CIS) server.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_server_disable(void);

/**
 * @brief	Enable the Bluetooth LE Audio unicast (CIS) server.
 *
 * @param[in]	rx_cb		Callback for handling received data.
 * @param[in]	location	Location of the unicast_server to be enabled.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_server_enable(le_audio_receive_cb rx_cb, enum bt_audio_location location);

#endif /* _UNICAST_SERVER_H_ */

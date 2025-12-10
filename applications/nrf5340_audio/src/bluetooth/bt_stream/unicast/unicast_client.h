/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup audio_app_bt_stream
 * @{
 * @defgroup unicast_client Functions for unicast client functionality.
 * @{
 * @brief Helper functions to manage unicast client (gateway side) functionality.
 */

#ifndef _UNICAST_CLIENT_H_
#define _UNICAST_CLIENT_H_

#include "bt_le_audio_tx.h"

#include <zephyr/bluetooth/audio/audio.h>
#include <audio_defines.h>

/**
 * @brief Unicast discovery direction enumeration.
 */
enum unicast_discover_dir {
	UNICAST_SERVER_SINK = BT_AUDIO_DIR_SINK,
	UNICAST_SERVER_SOURCE = BT_AUDIO_DIR_SOURCE,
	UNICAST_SERVER_BIDIR = (BT_AUDIO_DIR_SINK | BT_AUDIO_DIR_SOURCE)
};

#if CONFIG_BT_BAP_UNICAST_CONFIGURABLE
#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK                                               \
	BT_BAP_LC3_PRESET_CONFIGURABLE(BT_AUDIO_LOCATION_FRONT_LEFT,                               \
				       BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED,                          \
				       CONFIG_BT_AUDIO_BITRATE_UNICAST_SINK)

#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE                                             \
	BT_BAP_LC3_PRESET_CONFIGURABLE(BT_AUDIO_LOCATION_FRONT_LEFT,                               \
				       BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED,                          \
				       CONFIG_BT_AUDIO_BITRATE_UNICAST_SRC)

#elif CONFIG_BT_BAP_UNICAST_16_2_1
#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK                                               \
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                             \
					 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)

#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE                                             \
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                             \
					 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)

#elif CONFIG_BT_BAP_UNICAST_24_2_1
#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK                                               \
	BT_BAP_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                             \
					 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)

#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE                                             \
	BT_BAP_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                             \
					 BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)
#elif CONFIG_BT_BAP_UNICAST_48_4_1
#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK                                               \
	BT_BAP_LC3_UNICAST_PRESET_48_4_1(BT_AUDIO_LOCATION_ANY, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)

#define BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE                                             \
	BT_BAP_LC3_UNICAST_PRESET_48_4_1(BT_AUDIO_LOCATION_ANY, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED)
#else
#error Unsupported LC3 codec preset for unicast
#endif /* CONFIG_BT_BAP_UNICAST_CONFIGURABLE */

/**
 * @brief	Get configuration for the audio stream.
 *
 * @param[in]	stream			Pointer to the stream to get the configuration for.
 * @param[out]	bitrate			Pointer to the bit rate used; can be NULL.
 * @param[out]	sampling_rate_hz	Pointer to the sampling rate used; can be NULL.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_client_config_get(struct bt_bap_stream *stream, uint32_t *bitrate,
			      uint32_t *sampling_rate_hz);

/**
 * @brief	Get the current audio locations used by the unicast client.
 *
 * @param[out]	locations	Pointer to store the audio locations bitmask.
 * @param[in]	dir		Direction of the streams in question.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_client_locations_get(uint32_t *locations, enum bt_audio_dir dir);

/**
 * @brief	Start service discovery for a Bluetooth LE Audio unicast (CIS) server.
 *
 * @param[in]	conn	Pointer to the connection.
 * @param[in]	dir	Direction of the stream.
 *
 * @retval	-EALREADY	Device has already been discovered.
 * @retval	-ENOSPC		No more room in headset list.
 * @retval	0		Success.
 */
int unicast_client_discover(struct bt_conn *conn, enum unicast_discover_dir dir);

/**
 * @brief	Handle a disconnected Bluetooth LE Audio unicast (CIS) server.
 *
 * @param[in]	conn	Pointer to the connection.
 */
void unicast_client_conn_disconnected(struct bt_conn *conn);

/**
 * @brief	Start the Bluetooth LE Audio unicast (CIS) client.
 *
 * @note	Will start both sink and source if present.
 *
 * @param[in]	cig_index	Index of the Connected Isochronous Group (CIG) to start.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_client_start(uint8_t cig_index);

/**
 * @brief	Stop the Bluetooth LE Audio unicast (CIS) client.
 *
 * @note	Will stop both sink and source if present.
 *
  @param[in]	cig_index	Index of the Connected Isochronous Group (CIG) to stop.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_client_stop(uint8_t cig_index);

/**
 * @brief	Send encoded audio using the Bluetooth LE Audio unicast.
 *
 * @param[in]	audio_frame	Pointer to the audio to send.
 * @param[in]	cig_index	Index of the Connected Isochronous Group (CIG) to send to.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_client_send(struct net_buf const *const audio_frame, uint8_t cig_index);

/**
 * @brief       Disable the Bluetooth LE Audio unicast (CIS) client.
 *
 * @param[in]	cig_index	Index of the Connected Isochronous Group (CIG) to disable.
 *
 * @return      0 for success, error otherwise.
 */
int unicast_client_disable(uint8_t cig_index);

/**
 * @brief	Enable the Bluetooth LE Audio unicast (CIS) client.
 *
 * @param[in]	cig_index	Index of the Connected Isochronous Group (CIG) to enable.
 * @param[in]   recv_cb		Callback for handling received data.
 *
 * @return	0 for success, error otherwise.
 */
int unicast_client_enable(uint8_t cig_index, le_audio_receive_cb recv_cb);

/**
 * @}
 * @}
 */

#endif /* _UNICAST_CLIENT_H_ */

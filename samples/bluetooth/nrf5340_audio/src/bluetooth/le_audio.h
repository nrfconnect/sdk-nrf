/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_H_
#define _LE_AUDIO_H_

#include <zephyr.h>

#define DEVICE_NAME_PEER CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_PEER_LEN (sizeof(DEVICE_NAME_PEER) - 1)

#define LE_AUDIO_SDU_SIZE_OCTETS(bitrate) (bitrate / (1000000 / CONFIG_AUDIO_FRAME_DURATION_US) / 8)
#define LE_AUDIO_PRES_DELAY_US 10000u

enum le_audio_evt_type {
	LE_AUDIO_EVT_CONFIG_RECEIVED,
	LE_AUDIO_EVT_STREAMING,
	LE_AUDIO_EVT_NOT_STREAMING,
	LE_AUDIO_EVT_NUM_EVTS
};

struct le_audio_evt {
	enum le_audio_evt_type le_audio_evt_type;
};

/**
 * @brief Callback for receiving Bluetooth LE Audio data
 *
 * @param data		Pointer to received data
 * @param size		Size of received data
 * @param bad_frame	Indicating if the frame is a bad frame or not
 * @param sdu_ref	ISO timestamp
 */
typedef void (*le_audio_receive_cb)(const uint8_t *const data, size_t size, bool bad_frame,
				    uint32_t sdu_ref);

/**
 * @brief Get configuration for audio stream
 *
 * @param bitrate	Pointer to bitrate used
 * @param sampling_rate	Pointer to sampling rate used
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate);

/**
 * @brief	Increase volume by one step
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_volume_up(void);

/**
 * @brief	Decrease volume by one step
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_volume_down(void);

/**
 * @brief	Mute volume
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_volume_mute(void);

/**
 * @brief	Resume playing Bluetooth LE Audio stream
 *
 * @return	0 for success, error otherwise
 */
int le_audio_play(void);

/**
 * @brief	Pause Bluetooth LE Audio stream
 *
 * @return	0 for success, error otherwise
 */
int le_audio_pause(void);

/**
 * @brief Send Bluetooth LE Audio data
 *
 * @param data	Data to send
 * @param size	Size of data to send
 *
 * @return	0 for success,
 *		-ENXIO if the feature is disabled,
 *		error otherwise
 */
int le_audio_send(uint8_t const *const data, size_t size);

/**
 * @brief Enable Bluetooth LE Audio
 *
 * @param recv_cb	Callback for receiving Bluetooth LE Audio data
 *
 * @return		0 for success, error otherwise
 */
int le_audio_enable(le_audio_receive_cb recv_cb);

/**
 * @brief Disable Bluetooth LE Audio
 *
 * @return	0 for success, error otherwise
 */
int le_audio_disable(void);

#endif /* _LE_AUDIO_H_ */

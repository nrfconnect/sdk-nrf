/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BROADCAST_SINK_H_
#define _BROADCAST_SINK_H_

#include "le_audio.h"

/**
 * @brief	Change active audio stream if the BIG contains more than one BIS.
 *
 * @note	The active stream will iterate every time this function is called.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_sink_change_active_audio_stream(void);

/**
 * @brief	Get configuration for audio stream.
 *
 * @param[out]	bitrate		Pointer to the bitrate used; can be NULL.
 * @param[out]	sampling_rate	Pointer to the sampling rate used; can be NULL.
 * @param[out]	pres_delay	Pointer to the presentation delay used; can be NULL.
 *
 * @retval	0		Operation successful.
 * @retval	-ENXIO		The feature is disabled.
 */
int broadcast_sink_config_get(uint32_t *bitrate, uint32_t *sampling_rate, uint32_t *pres_delay);

/**
 * @brief	Set periodic advertising sync.
 *
 * @param[in]	pa_sync		Pointer to the periodic advertising sync.
 * @param[in]	broadcast_id	Broadcast ID of the periodic advertising.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_sink_pa_sync_set(struct bt_le_per_adv_sync *pa_sync, uint32_t broadcast_id);

/**
 * @brief	Start the Bluetooth LE Audio broadcast sink.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_sink_start(void);

/**
 * @brief	Stop the Bluetooth LE Audio broadcast sink.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_sink_stop(void);

/**
 * @brief	Enable the LE Audio broadcast sink.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_sink_enable(le_audio_receive_cb recv_cb);

/**
 * @brief	Disable the LE Audio broadcast sink.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_sink_disable(void);

#endif /* _BROADCAST_SINK_H_ */

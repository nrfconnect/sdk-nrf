/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_TX_H_
#define _LE_AUDIO_TX_H_

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>

#include "le_audio.h"

struct le_audio_tx_info {
	struct stream_index idx;
	struct bt_cap_stream *cap_stream;
	uint8_t audio_channel;
};

/**
 * @brief	Allocates buffers and sends data to the controller.
 *
 * @note	Send all available channels in a single call.
 *		Do not call this for each channel.
 *
 * @param[in]	tx		Pointer to an array of le_audio_tx_info elements.
 * @param[in]	num_tx		Number of elements in @p tx.
 * @param[in]	enc_audio	Encoded audio data.
 *
 * @return	0 if successful, error otherwise.
 */
int bt_le_audio_tx_send(struct le_audio_tx_info *tx, uint8_t num_tx,
			struct le_audio_encoded_audio enc_audio);

/**
 * @brief	Initializes a stream. Must be called when a TX stream is started.
 *
 * @param[in]	stream_idx	Stream index.
 *
 * @retval	-EACCES		The module has not been initialized.
 * @retval	0		Success.
 */
int bt_le_audio_tx_stream_started(struct stream_index stream_idx);

/**
 * @brief	Frees a TX buffer. Must be called when a TX stream has been sent.
 *
 * @param[in]	stream_idx	Stream index.
 *
 * @retval	-EACCES		The module has not been initialized.
 * @retval	0		Success.
 */
int bt_le_audio_tx_stream_sent(struct stream_index stream_idx);

/**
 * @brief	Initializes the TX path for ISO transmission.
 */
void bt_le_audio_tx_init(void);

#endif /* _LE_AUDIO_TX_H_ */

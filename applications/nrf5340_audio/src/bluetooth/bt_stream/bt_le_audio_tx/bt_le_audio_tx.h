/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_TX_H_
#define _LE_AUDIO_TX_H_

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>

#include "le_audio.h"

/**
 * @brief	Allocates buffers and sends data to the controller.
 *
 * @note	Send all available channels in a single call.
 *		Do not call this for each channel.
 *
 * @param	bap_streams		Pointer to an array of BAP streams.
 * @param	audio_mapping_mask	Pointer to an array of channel masks.
 * @param	enc_audio		Encoded audio data.
 * @param	streams_to_tx		Number of streams to send.
 *
 * @return 0 if successful, error otherwise.
 */
int bt_le_audio_tx_send(struct bt_bap_stream **bap_streams, uint8_t *audio_mapping_mask,
			struct le_audio_encoded_audio enc_audio, uint8_t streams_to_tx);

/**
 * @brief Resets TX buffers. Must be called when a TX stream is stopped.
 *
 * @param stream_idx	Stream index
 *
 * @retval  -EACCES  The module has not been initialized.
 * @retval  0 Success.
 */
int bt_le_audio_tx_stream_stopped(uint8_t stream_idx);

/**
 * @brief Initializes a stream. Must be called when a TX stream is started.
 *
 * @param stream_idx	Stream index.
 *
 * @retval  -EACCES  The module has not been initialized.
 * @retval  0 Success.
 */
int bt_le_audio_tx_stream_started(uint8_t stream_idx);

/**
 * @brief Frees a TX buffer. Must be called when a TX stream has been sent.
 *
 * @param stream_idx	Stream index.
 *
 * @retval  -EACCES  The module has not been initialized.
 * @retval  0 Success.
 */
int bt_le_audio_tx_stream_sent(uint8_t stream_idx);

/**
 * @brief Initializes the TX path for ISO transmission.
 *
 * @retval  -EALREADY  The module has already been initialized.
 * @retval  0 Success.
 */
int bt_le_audio_tx_init(void);

#endif /* _LE_AUDIO_TX_H_ */

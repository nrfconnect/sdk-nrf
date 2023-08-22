/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_RX_H_
#define _LE_AUDIO_RX_H_

/**
 * @brief Data handler when ISO data has been received.
 *
 * @param[in] p_data		Pointer to the received data.
 * @param[in] data_size		Size of the received data.
 * @param[in] bad_frame		Bad frame flag. (I.e. set for missed ISO data).
 * @param[in] sdu_ref		SDU reference timestamp.
 * @param[in] channel_index	Which channel is received.
 * @param[in] desired_data_size	The expected data size.
 *
 * @return 0 if successful, error otherwise.
 */
void le_audio_rx_data_handler(uint8_t const *const p_data, size_t data_size, bool bad_frame,
			      uint32_t sdu_ref, enum audio_channel channel_index,
			      size_t desired_data_size);

/**
 * @brief Initialize the receive audio path.
 *
 * @return 0 if successful, error otherwise.
 */
int le_audio_rx_init(void);

#endif /* _LE_AUDIO_RX_H_ */

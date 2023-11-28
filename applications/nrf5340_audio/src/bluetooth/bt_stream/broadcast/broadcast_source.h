/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BROADCAST_SOURCE_H_
#define _BROADCAST_SOURCE_H_

#include "bt_le_audio_tx.h"

/**
 * @brief	Get the data to advertise.
 *
 * @param[out]	ext_adv		Pointer to the pointer of bt_data used for extended advertising.
 * @param[out]	ext_adv_size	Pointer to size of @p ext_adv.
 * @param[out]	per_adv		Pointer to the pointer of bt_data used for periodic advertising.
 * @param[out]	per_adv_size	Pointer to size of @p per_adv.
 */
void broadcast_source_adv_get(const struct bt_data **ext_adv, size_t *ext_adv_size,
			      const struct bt_data **per_adv, size_t *per_adv_size);

/**
 * @brief	Start the Bluetooth LE Audio broadcast (BIS) source.
 *
 * @param[in]	ext_adv		Pointer to the extended advertising set; can be NULL if a stream
 *				is restarted.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_start(struct bt_le_ext_adv *ext_adv);

/**
 * @brief	Stop the Bluetooth LE Audio broadcast (BIS) source.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_stop(void);

/**
 * @brief	Broadcast the Bluetooth LE Audio data.
 *
 * @param[in]	enc_audio	Encoded audio struct.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_send(struct le_audio_encoded_audio enc_audio);

/**
 * @brief	Disable the LE Audio broadcast (BIS) source.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_disable(void);

/**
 * @brief	Enable the LE Audio broadcast (BIS) source.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_enable(void);

#endif /* _BROADCAST_SOURCE_H_ */

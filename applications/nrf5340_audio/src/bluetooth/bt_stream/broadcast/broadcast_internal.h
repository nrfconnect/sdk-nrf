/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BROADCAST_INTERNAL_H_
#define _BROADCAST_INTERNAL_H_

#include "le_audio.h"

#if CONFIG_BT_AUDIO_BROADCAST_CONFIGURABLE
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_PRESET_CONFIGURABLE(                                                            \
		BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,                      \
		BT_AUDIO_CONTEXT_TYPE_MEDIA, CONFIG_BT_AUDIO_BITRATE_BROADCAST_SRC)

#elif CONFIG_BT_BAP_BROADCAST_16_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_24_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_16_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_24_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_24_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#else
#error Unsupported LC3 codec preset for broadcast
#endif /* CONFIG_BT_AUDIO_BROADCAST_CONFIGURABLE */

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
 * @brief	Start the Bluetooth LE Audio broadcast source.
 *
 * @param[in]	ext_adv		Pointer to the extended advertising set, can be NULL if a stream
 *				is restarted.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_start(struct bt_le_ext_adv *ext_adv);

/**
 * @brief	Stop the Bluetooth LE Audio broadcast source.
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
int broadcast_source_send(struct encoded_audio enc_audio);

/**
 * @brief	Enable the LE Audio broadcast source.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_enable(void);

/**
 * @brief	Disable the LE Audio broadcast source.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_disable(void);

#endif /* _BROADCAST_INTERNAL_H_ */

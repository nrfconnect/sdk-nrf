/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup audio_app_bt_stream
 * @{
 * @defgroup broadcast_source Functions for broadcast source functionality.
 * @{
 * @brief Helper functions to manage broadcast source functionality.
 */

#ifndef _BROADCAST_SOURCE_H_
#define _BROADCAST_SOURCE_H_

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include "bt_le_audio_tx.h"

#if CONFIG_BT_AUDIO_BROADCAST_CONFIGURABLE
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_PRESET_CONFIGURABLE(                                                            \
		CONFIG_BT_AUDIO_BROADCAST_SAMPLE_RATE_VALUE,                                       \
		BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,                      \
		BT_AUDIO_CONTEXT_TYPE_MEDIA, CONFIG_BT_AUDIO_BITRATE_BROADCAST_SRC)

#elif CONFIG_BT_BAP_BROADCAST_16_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_16_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_24_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_24_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_24_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_48_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_48_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)
#elif CONFIG_BT_BAP_BROADCAST_48_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_48_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_48_4_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_48_4_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_48_4_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_48_4_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_48_6_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_48_6_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_48_6_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_48_6_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)
#else
#error Unsupported LC3 codec preset for broadcast
#endif /* CONFIG_BT_AUDIO_BROADCAST_CONFIGURABLE */

/* Size of the Public Broadcast Announcement header, 2-octet Service UUID followed by
 * an octet for the features and an octet for the length of the meta data field.
 */
#define BROADCAST_SOURCE_PBA_HEADER_SIZE (BT_UUID_SIZE_16 + (sizeof(uint8_t) * 2))

#define BROADCAST_SOURCE_ADV_NAME_MAX (32)
#define BROADCAST_SOURCE_ADV_ID_START (BT_UUID_SIZE_16)

struct subgroup_config {
	enum bt_audio_location *location;
	uint8_t num_bises;
	enum bt_audio_context context;
	struct bt_bap_lc3_preset group_lc3_preset;
	char *preset_name;
};

struct broadcast_source_big {
	struct subgroup_config *subgroups;
	uint8_t num_subgroups;
	uint8_t packing;
	bool encryption;
	uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];
	char broadcast_name[BROADCAST_SOURCE_ADV_NAME_MAX + 1];
	char adv_name[CONFIG_BT_DEVICE_NAME_MAX + 1];
	bool fixed_id;
	uint32_t broadcast_id;
};

/**
 * @brief  Advertising data for broadcast source.
 */
struct broadcast_source_ext_adv_data {
	/* Broadcast Audio Streaming UUIDs. */
	struct net_buf_simple *uuid_buf;

	/* Broadcast Audio Streaming Endpoint advertising data. */
	uint8_t brdcst_id_buf[BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE];

	/* Buffer for Appearance. */
	uint8_t brdcst_appearance_buf[(sizeof(uint8_t) * 2)];

	/* Broadcast name, must be between 4 and 32 UTF-8 encoded characters in length. */
	uint8_t brdcst_name_buf[BROADCAST_SOURCE_ADV_NAME_MAX];

#if (CONFIG_AURACAST)
	/* Number of free metadata items */
	uint8_t pba_metadata_vacant_cnt;

	/* Public Broadcast Announcement buffer. */
	uint8_t *pba_buf;
#endif /* (CONFIG_AURACAST) */
};

/**
 * @brief	Periodic advertising data for broadcast source.
 */
struct broadcast_source_per_adv_data {
	/* Buffer for periodic advertising data */
	struct net_buf_simple *base_buf;
};

/**
 * @brief	Populate the extended advertising data buffer.
 *
 * @param[in]	big_index		Index of the Broadcast Isochronous Group (BIG) to get
 *					advertising data for.
 * @param[in]	fixed_id		Flag to indicate if the broadcast ID will be random or not.
 * @param[in]	broadcast_id		Broadcast ID to be used in the advertising data if
 *					@p fixed_id is set to true. The broadcast ID is three octets
 *					long.
 * @param[in]	ext_adv_data		Pointer to the extended advertising buffers.
 * @param[out]	ext_adv_buf		Pointer to the bt_data used for extended advertising.
 * @param[out]	ext_adv_buf_vacant	Pointer to unused size of @p ext_adv_buf.
 *
 * @return	Negative values for errors or number of elements added to @p ext_adv_buf.
 */
int broadcast_source_ext_adv_populate(uint8_t big_index, bool fixed_id, uint32_t broadcast_id,
				      struct broadcast_source_ext_adv_data *ext_adv_data,
				      struct bt_data *ext_adv_buf, size_t ext_adv_buf_vacant);

/**
 * @brief	Populate the periodic advertising data buffer.
 *
 * @param[in]	big_index		Index of the Broadcast Isochronous Group (BIG) to get
 *					advertising data for.
 * @param[in]	per_adv_data		Pointer to a structure of periodic advertising buffers.
 * @param[out]	per_adv_buf		Pointer to the bt_data used for periodic advertising.
 * @param[out]	per_adv_buf_vacant	Pointer to unused size of @p per_adv_buf.
 *
 * @return	Negative values for errors or number of elements added to @p per_adv_buf.
 */
int broadcast_source_per_adv_populate(uint8_t big_index,
				      struct broadcast_source_per_adv_data *per_adv_data,
				      struct bt_data *per_adv_buf, size_t per_adv_buf_vacant);

/**
 * @brief	Check if the broadcast source is streaming.
 *
 * @param[in]	big_index	Index of the Broadcast Isochronous Group (BIG) to check.
 *
 * @retval	True	The broadcast source is streaming.
 * @retval	False	The broadcast source is not streaming.
 */

bool broadcast_source_is_streaming(uint8_t big_index);

/**
 * @brief	Start the Bluetooth LE Audio broadcast (BIS) source.
 *
 * @param[in]	big_index	Index of the Broadcast Isochronous Group (BIG) to start.
 * @param[in]	ext_adv		Pointer to the extended advertising set; can be NULL if a stream
 *				is restarted.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_start(uint8_t big_index, struct bt_le_ext_adv *ext_adv);

/**
 * @brief	Stop the Bluetooth LE Audio broadcast (BIS) source.
 *
 * @param[in]	big_index	Index of the Broadcast Isochronous Group (BIG) to stop.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_stop(uint8_t big_index);

/**
 * @brief	Get the broadcast ID for the given Broadcast Isochronous Group (BIG).
 *
 * @note	The broadcast ID is used to identify the broadcast. Its value is three octets long.
 *		This function should only be called after the BIG has been created.
 *
 * @param[in]	big_index	Index of the Broadcast Isochronous Group (BIG) to get the broadcast
 *				ID for.
 * @param[out]	broadcast_id	Pointer to the broadcast ID.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_id_get(uint8_t big_index, uint32_t *broadcast_id);

/**
 * @brief	Broadcast the Bluetooth LE Audio data.
 *
 * @param[in]	audio_frame	Pointer to the audio buffer.
 * @param[in]	big_index	Index of the Broadcast Isochronous Group (BIG) to broadcast.
 * @param[in]	subgroup_index	Index of the subgroup to broadcast.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_send(struct net_buf const *const audio_frame, uint8_t big_index,
			  uint8_t subgroup_index);

/**
 * @brief	Disable the LE Audio broadcast (BIS) source.
 *
 * @param[in]	big_index  Index of the Broadcast Isochronous Group (BIG) to disable.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_disable(uint8_t big_index);

/**
 * @brief	Create a set up for a default broadcaster.
 *
 * @note	This will create the parameters for a simple broadcaster with 1 Broadcast
 *		Isochronous Group (BIG), 1 subgroup, and 2 BISes.
 *		The BISes will be front_left and front_right and language will be set to 'eng'.
 *
 * @param[out]	broadcast_param	Pointer to populate with parameters for setting up the broadcaster.
 */
void broadcast_source_default_create(struct broadcast_source_big *broadcast_param);

/**
 * @brief	Enable the LE Audio broadcast (BIS) source.
 *
 * @param[in]	broadcast_param	Array of create parameters for creating a Broadcast Isochronous
 *				Group (BIG).
 * @param[in]	big_index	Index of the BIG to enable.
 *
 * @return	0 for success, error otherwise.
 */
int broadcast_source_enable(struct broadcast_source_big const *const broadcast_param,
			    uint8_t big_index);

/**
 * @}
 * @}
 */

#endif /* _BROADCAST_SOURCE_H_ */

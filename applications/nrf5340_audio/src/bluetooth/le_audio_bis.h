/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_BIS_H_
#define _LE_AUDIO_BIS_H_

#include <zephyr/kernel.h>
#include <bluetooth/audio/audio.h>
#include "le_audio.h"

/* These give the row as given in Table 6.4 of BAP
   starting form 1 with a user defined default at 0 */
#define BT_AUDIO_BROADCAST_LC3_DEFAULT 0
#define BT_AUDIO_LC3_BROADCAST_LATENCY_MANDATORY_1 4
#define BT_AUDIO_LC3_BROADCAST_LATENCY_MANDATORY_2 6
#define BT_AUDIO_LC3_BROADCAST_RELIABILITY_MANDATORY_1 20
#define BT_AUDIO_LC3_BROADCAST_RELIABILITY_MANDATORY_2 22

#define BT_AUDIO_LC3_BROADCAST_PRESET_DEFAUT(_loc, _stream_context)                                \
	BT_AUDIO_LC3_PRESET(                                                                       \
		BT_CODEC_LC3_CONFIG(                                                               \
			BT_CODEC_CONFIG_LC3_FREQ_48KHZ, BT_CODEC_CONFIG_LC3_DURATION_10, _loc,     \
			LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 1, _stream_context),         \
		BT_CODEC_LC3_QOS_10_UNFRAMED(LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 4u,     \
					     20u, LE_AUDIO_PRES_DELAY_US))

#if (BT_AUDIO_BROADCAST_LC3_CONFIGURATION == BT_AUDIO_BROADCAST_LC3_DEFAULT)
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_DEFAUT(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)
#elif (CONFIG_AUDIO_SOURCE_I2S &&                                                                  \
       (BT_AUDIO_BROADCAST_LC3_CONFIGURATION == BT_AUDIO_LC3_BROADCAST_LATENCY_MANDATORY_1))
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)
#elif (CONFIG_AUDIO_SOURCE_I2S &&                                                                  \
       (BT_AUDIO_BROADCAST_LC3_CONFIGURATION == BT_AUDIO_LC3_BROADCAST_LATENCY_MANDATORY_2))
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)
#elif (CONFIG_AUDIO_SOURCE_I2S &&                                                                  \
       (BT_AUDIO_BROADCAST_LC3_CONFIGURATION == BT_AUDIO_LC3_BROADCAST_RELIABILITY_MANDATORY_1))
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)
#elif (CONFIG_AUDIO_SOURCE_I2) &&                                                                  \
	(BT_AUDIO_BROADCAST_LC3_CONFIGURATION == BT_AUDIO_LC3_BROADCAST_RELIABILITY_MANDATORY_2))
#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_BROADCAST_PRESET_24_2_2(BT_AUDIO_LOCATION_FRONT_LEFT,                         \
					     BT_AUDIO_CONTEXT_TYPE_MEDIA)
#else
BUILD_ASSERT(0, "Unsupported LC3 codec preset for broadcast");
#endif

#endif /* _LE_AUDIO_BIS_H_ */
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LE_AUDIO_CIS_H_
#define _LE_AUDIO_CIS_H_

#include <zephyr/kernel.h>
#include <bluetooth/audio/audio.h>
#include "le_audio.h"

/* These give the row as given in Table 5.2 of BAP (starting form 0) */
#define BT_AUDIO_UNICAST_LC3_DEFAULT 11
#define BT_AUDIO_LC3_UNICAST_MANDATORY_1 3
#define BT_AUDIO_LC3_UNICAST_MANDATORY_2 5

#define BT_AUDIO_LC3_UNICAST_PRESET_DEFAULT                                                        \
	BT_AUDIO_LC3_PRESET(                                                                       \
		BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ,                                \
				    BT_CODEC_CONFIG_LC3_DURATION_10, BT_AUDIO_LOCATION_FRONT_LEFT, \
				    LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 1,               \
				    BT_AUDIO_CONTEXT_TYPE_MEDIA),                                  \
		BT_CODEC_LC3_QOS_10_UNFRAMED(LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 2u,     \
					     20u, LE_AUDIO_PRES_DELAY_US))

#if (CONFIG_AUDIO_SOURCE_USB)
#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO BT_AUDIO_LC3_UNICAST_PRESET_DEFAULT
#else
#if (CONFIG_BT_AUDIO_LC3_CONFIGURATION == BT_AUDIO_UNICAST_LC3_DEFAULT)
#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO BT_AUDIO_LC3_UNICAST_PRESET_DEFAULT
#elif (CONFIG_BT_AUDIO_LC3_CONFIGURATION == BT_AUDIO_LC3_UNICAST_MANDATORY_1)
#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO BT_AUDIO_LC3_UNICAST_PRESET_16_2_1
#elif ((CONFIG_AUDIO_DEV == HEADSET) &&                                                            \
       (CONFIG_BT_AUDIO_LC3_CONFIGURATION == BT_AUDIO_LC3_UNICAST_MANDATORY_2))
#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO BT_AUDIO_LC3_UNICAST_PRESET_24_2_1
#else
BUILD_ASSERT(0, "Unsupported LC3 codec preset for unicast");
#endif
#endif

#endif /* _LE_AUDIO_CIS_H_ */
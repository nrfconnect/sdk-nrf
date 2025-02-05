/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NAC_TEST_
#define _NAC_TEST_

/**
 * @defgroup auraconfig_tester nRF Auraconfig tester
 * @brief nRF Auraconfig tester
 *
 * @{
 */

#include <zephyr/bluetooth/audio/cap.h>

#define ADV_NAME_MAX (28)

struct nac_test_values_subgroup {
	uint32_t sampling_rate_hz;
	uint32_t frame_duration_us;
	uint8_t octets_per_frame;
	char language[4]; /* Allow space for '\0' */
	bool immediate_rend_flag;
	enum bt_audio_context context;
	char program_info[255];
	uint8_t num_bis;
	enum bt_audio_location locations[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
};

struct nac_test_values_big {
	char adv_name[ADV_NAME_MAX + 1];
	char broadcast_name[ADV_NAME_MAX + 1];
	uint32_t broadcast_id;
	uint8_t phy;
	uint32_t pd_us;
	bool encryption;
	uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE + 1];
	uint8_t num_subgroups;
	bool high_quality;
	bool std_quality;
	struct nac_test_values_subgroup *subgroups;
};

void nac_test_loop(void);

int nac_test_main(void);

/** @} */

#endif /* _NAC_TEST_ */

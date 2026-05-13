/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "lc3_file_fake_data.h"

#include <zephyr/sys/util.h>

const uint8_t fake_dataset1_valid[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				       0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
const size_t fake_dataset1_valid_size = ARRAY_SIZE(fake_dataset1_valid);

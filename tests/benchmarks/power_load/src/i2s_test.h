/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2s.h>

#define I2S_THREAD_STACKSIZE (4096)
#define I2S_THREAD_PRIORITY  (1)
#define I2S_THREAD_SLEEP     (100)

#define SAMPLE_WIDTH	   16
#define WORDS_COUNT	   16
#define SLAB_ALIGN	   32
#define NUMBER_OF_BLOCKS   1
#define NUMBER_OF_CHANNELS 1
#define TIMEOUT_MS	   2000
#define FRAME_CLK_FREQ_HZ  8000

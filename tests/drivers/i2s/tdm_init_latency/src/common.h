/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <dk_buttons_and_leds.h>

#define WORDS_COUNT		       512
#define TIMEOUT_MS		       2000
#define NUMBER_OF_BLOCKS	       1
#define NUMBER_OF_CHANNELS	       2
#define TEST_TIMER_COUNT_TIME_LIMIT_MS 500
#define SAMPLE_RATE		       48000UL

void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms);

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

extern void hf_audio_clock_test_prepare(void *fixture);

ZTEST_SUITE(suite_hf_audio_clock, NULL, NULL, hf_audio_clock_test_prepare, NULL, NULL);

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <audio_defines.h>

#define TEST_BITS_8  8
#define TEST_BITS_16 16
#define TEST_BITS_24 24
#define TEST_BITS_32 32

#define CONTIN_TEST_CONTIN_LOC_MAX   (BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT)
#define CONTIN_TEST_CONTIN_DATA_SIZE 300
#define CONTIN_TEST_POOL_CONTIN_NUM  1
#define CONTIN_TEST_POOL_FINITE_NUM  1
#define CONTIN_TEST_POOL_CONTIN_SIZE                                                               \
	(CONTIN_TEST_CONTIN_DATA_SIZE *                                                            \
	 ((CONTIN_TEST_CONTIN_LOC_MAX == BT_AUDIO_LOCATION_MONO_AUDIO                              \
		   ? 1                                                                             \
		   : POPCOUNT(CONTIN_TEST_CONTIN_LOC_MAX))))
#define CONTIN_TEST_SAMPLE_SIZE	 (16)
#define CONTIN_TEST_CARRIER_SIZE (16)
#define CONTIN_TEST_BYTE	 0x1f

#define CONTIN_TEST_INTERLEAVED	  true
#define CONTIN_TEST_DEINTERLEAVED false
#define CONTIN_TEST_FILLED	  true
#define CONTIN_TEST_NOT_FILLED	  false

extern uint8_t const test_arr[256];

void validate_contin_array(struct net_buf const *const pcm_contin, uint8_t const *const pcm_finite,
			   uint32_t locs_out, uint16_t finite_start, uint16_t finite_end,
			   bool interleaved, bool filled);

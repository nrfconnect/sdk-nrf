/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <audio_defines.h>

enum test_locations {
	TEST_CHANS_MONO = 0,
	TEST_CHANS_1 = 1,
	TEST_CHANS_2 = 2,
	TEST_CHANS_3 = 3
};

#define CONTIN_TEST_FINITE_DATA_SIZE 256
#define CONTIN_TEST_CONTIN_DATA_SIZE 300
#define CONTIN_TEST_POOL_CONTIN_NUM  1
#define CONTIN_TEST_POOL_FINITE_NUM  1
#define CONTIN_TEST_POOL_CONTIN_SIZE (CONTIN_TEST_CONTIN_DATA_SIZE * TEST_CHANS_2)
#define CONTIN_TEST_POOL_FINITE_SIZE CONTIN_TEST_FINITE_DATA_SIZE
#define CONTIN_TEST_SAMPLE_SIZE	     (16)
#define CONTIN_TEST_CARRIER_SIZE     (16)
#define CONTIN_TEST_CONTIN_BYTE	     0x1f

#define TEST_BITS_8  8
#define TEST_BITS_16 16
#define TEST_BITS_24 24
#define TEST_BITS_32 32

extern const uint8_t test_arr[CONTIN_TEST_FINITE_DATA_SIZE];

void validate_contin_single_array(struct net_buf *pcm_contin, uint8_t *pcm_finite, uint8_t channels,
				  uint8_t channel, uint16_t finite_start, uint16_t finite_end,
				  bool interleaved);

void validate_contin_chans_array(struct net_buf *pcm_contin, uint8_t *pcm_finite, uint8_t channels,
				 uint16_t finite_start, uint16_t finite_pos, bool interleaved);

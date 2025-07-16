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
#include <audio_defines.h>

enum test_locations {
	TEST_LOC_MONO = 0x00000000,
	TEST_LOC_L = 0x00000001,
	TEST_LOC_R = 0x00000002,
	TEST_LOC_C = 0x00000003,
	TEST_LOC_L_R = 0x00000003,
	TEST_LOC_L_R_C = 0x00000007
};

#define TEST_BITS_8  8
#define TEST_BITS_16 16
#define TEST_BITS_24 24
#define TEST_BITS_32 32

#define CONTIN_TEST_CONTIN_LOC_MAX   TEST_LOC_L_R
#define CONTIN_TEST_FINITE_DATA_SIZE 256
#define CONTIN_TEST_CONTIN_DATA_SIZE 300
#define CONTIN_TEST_POOL_CONTIN_NUM  1
#define CONTIN_TEST_POOL_FINITE_NUM  1
#define CONTIN_TEST_POOL_CONTIN_SIZE                                                               \
	(CONTIN_TEST_CONTIN_DATA_SIZE *                                                            \
	 ((CONTIN_TEST_CONTIN_LOC_MAX == TEST_LOC_MONO ? 1                                         \
						       : POPCOUNT(CONTIN_TEST_CONTIN_LOC_MAX))))
#define CONTIN_TEST_POOL_FINITE_SIZE CONTIN_TEST_FINITE_DATA_SIZE
#define CONTIN_TEST_SAMPLE_SIZE	     (16)
#define CONTIN_TEST_CARRIER_SIZE     (16)
#define CONTIN_TEST_BYTE	     0x1f

extern const uint8_t test_arr[CONTIN_TEST_FINITE_DATA_SIZE];

void validate_contin_array(struct net_buf *pcm_contin, uint8_t *pcm_finite, uint32_t locs_out,
			   uint16_t finite_start, uint16_t finite_end, bool interleaved,
			   bool filled);

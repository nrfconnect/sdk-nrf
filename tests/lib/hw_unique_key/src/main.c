/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <hw_unique_key.h>
#include "../../../../lib/hw_unique_key/hw_unique_key_internal.h"
#include <nrf_cc3xx_platform_kmu.h>
#include <pm_config.h>
#include <nrfx.h>

#define STATE_TEST_WRITE 0x54834352
#define STATE_TEST_LOAD 0x17029357
#define STATE_TEST_ERROR 0x72490698
#define STATE_TEST_INVALID 0x98296028

/* This test involves multiple resets, so this variable is __noinit to keep
 * state between resets.
 */
static __noinit volatile uint32_t state;

#if defined(CONFIG_HAS_HW_NRF_CC312)
__ALIGNED(4)
static uint8_t test_key[32] = {
	0xc5, 0xa8, 0x08, 0xeb, 0xe3, 0x1e, 0xa5, 0xb4,
	0xe9, 0x44, 0x1a, 0x76, 0x45, 0x58, 0xd8, 0x8b,
	0x40, 0x26, 0x33, 0xa8, 0xcd, 0x2d, 0x51, 0x67,
	0x8d, 0xef, 0x00, 0x24, 0x30, 0x52, 0xd7, 0x3d
};

__ALIGNED(4)
static uint8_t test_label[16] = {
	0xe2, 0x75, 0xab, 0x25, 0x00, 0x3b, 0x15, 0xe1,
	0xa1, 0x98, 0x78, 0x5b, 0x4e, 0x57, 0x43, 0x9a
};

__ALIGNED(4)
static uint8_t test_context[16] = {
	0x69, 0xa7, 0x99, 0xd5, 0x32, 0x29, 0xb7, 0xc7,
	0x1b, 0xf1, 0x33, 0x56, 0x2d, 0x31, 0x1c, 0xf3
};

__ALIGNED(4)
static uint8_t expected_key[32] = {
	0xc9, 0xd2, 0xb7, 0xd5, 0x81, 0x41, 0xad, 0xcb,
	0xd6, 0x4b, 0xac, 0xa0, 0xd1, 0xdb, 0x5e, 0x36,
	0x08, 0x60, 0x55, 0x33, 0x18, 0x6e, 0x32, 0x3b,
	0x73, 0xa8, 0x77, 0xe3, 0xa4, 0x3e, 0x83, 0x8d
};

#else
__ALIGNED(4)
static uint8_t test_key[16] = {
	0x19, 0x9a, 0xe3, 0xc7, 0x9d, 0xd0, 0x16, 0x8c,
	0x3e, 0xee, 0xa8, 0x46, 0xea, 0x4e, 0xdc, 0x6e
};


__ALIGNED(4)
static uint8_t test_label[19] = {
	0x29, 0xd6, 0xa6, 0x8d, 0x3f, 0xfb, 0x39, 0x1f,
	0x03, 0x6d, 0xd5, 0x3b, 0xe3, 0x54, 0xe4, 0x02,
	0x9a, 0x69, 0x5a
};

__ALIGNED(4)
static uint8_t test_context[19] = {
	0x58, 0xfd, 0xf5, 0xa0, 0x8f, 0x8e, 0xfc, 0x42,
	0x10, 0x55, 0xae, 0xe5, 0xf4, 0x6d, 0x25, 0x74,
	0x40, 0xd4, 0x85
};

__ALIGNED(4)
static uint8_t expected_key[16] = {
	0x65, 0x29, 0x5e, 0x2e, 0xcb, 0x6e, 0x6c, 0x40,
	0x3a, 0x38, 0x76, 0xc9, 0x9f, 0xf7, 0xfc, 0x71
};
#endif

static uint32_t expected_fatal;
static uint32_t actual_fatal;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	actual_fatal++;
}


static void do_key_test(void)
{
	int err;
	uint8_t out_key[HUK_SIZE_BYTES];

	switch (state) {
	case STATE_TEST_WRITE:
		state = STATE_TEST_ERROR;

		for (int i = 0; i < ARRAY_SIZE(huk_slots); i++) {
			uint32_t keyslot = huk_slots[i];

			zassert_false(hw_unique_key_is_written(keyslot), NULL);
			err = hw_unique_key_write(keyslot, test_key);
			zassert_equal(HW_UNIQUE_KEY_SUCCESS, err, "unexpected error: %d\n", err);
			zassert_true(hw_unique_key_is_written(keyslot), NULL);
		}

		state = STATE_TEST_LOAD;
		NVIC_SystemReset();
		break;
	case STATE_TEST_LOAD:
		state = STATE_TEST_ERROR;

		for (int i = 0; i < ARRAY_SIZE(huk_slots); i++) {
			zassert_true(hw_unique_key_is_written(huk_slots[i]), NULL);

#ifndef HUK_HAS_KMU
			if (huk_slots[i] == HUK_KEYSLOT_KDR) {
				err = hw_unique_key_load_kdr();
				zassert_equal(HW_UNIQUE_KEY_SUCCESS, err, "unexpected error: %d\n",
					      err);
			}
#endif

			err = nrf_cc3xx_platform_kmu_shadow_key_derive(huk_slots[i],
				HUK_SIZE_BYTES * 8, test_label, sizeof(test_label), test_context,
				sizeof(test_context), out_key, sizeof(out_key));

			zassert_equal(0, err, "unexpected error: %d\n", err);
			zassert_mem_equal(expected_key, out_key, sizeof(expected_key), NULL);

#ifdef PM_HW_UNIQUE_KEY_PARTITION_ADDRESS
			state = STATE_TEST_INVALID;
			expected_fatal++;
			/* The following causes an exception */
			zassert_equal(0, *(uint32_t *)PM_HW_UNIQUE_KEY_PARTITION_ADDRESS, NULL);
#else
			NRF_KMU->SELECTKEYSLOT = KMU_SELECT_SLOT(huk_slots[i]);
			zassert_equal(0xDEADDEAD,
				NRF_UICR_S->KEYSLOT.KEY[KMU_SELECT_SLOT(huk_slots[i])].VALUE[0],
				"\n%d\n",
				NRF_UICR_S->KEYSLOT.KEY[KMU_SELECT_SLOT(huk_slots[i])].VALUE[0]);
			NRF_KMU->SELECTKEYSLOT = 0;
			state = STATE_TEST_INVALID;
#endif
		}
		zassert_equal(STATE_TEST_INVALID, state, "State hasn't changed.\n");
		break;
	case STATE_TEST_ERROR:
		state = STATE_TEST_INVALID;

		zassert_unreachable("An error occurred.\n");
		break;
	default:
		state = STATE_TEST_ERROR;

		for (int i = 0; i < ARRAY_SIZE(huk_slots); i++) {
			uint32_t keyslot = huk_slots[i];

			if (hw_unique_key_is_written(keyslot)) {
				printk("This test requires a full chip erase to be rerun.\n");
				NVIC_SystemReset();
			}
		}

		state = STATE_TEST_WRITE;
		NVIC_SystemReset();
	}
}

ZTEST(test_hw_unique_key, test_hw_unique_key1)
{
	do_key_test();
}

void check_fatal(void *f)
{
	zassert_equal(expected_fatal, actual_fatal,
			"An unexpected fatal error has occurred (%d != %d).\n",
			expected_fatal, actual_fatal);
}

ZTEST_SUITE(test_hw_unique_key, NULL, NULL, NULL, check_fatal, NULL);

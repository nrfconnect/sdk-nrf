/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_LIB_HW_UNIQUE_KEY_INTERNAL_H__
#define NRF_LIB_HW_UNIQUE_KEY_INTERNAL_H__

#include <hw_unique_key.h>

/* Define print and panic macros. This library can be compiled for different OSes. */
#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#elif defined(__NRF_TFM__)
#include "utilities.h"
#endif

/* The available slots as an array that can be iterated over. */
static const enum hw_unique_key_slot huk_slots[] = {
#ifndef HUK_HAS_KMU
	HUK_KEYSLOT_KDR,
#else
	HUK_KEYSLOT_MKEK,
	HUK_KEYSLOT_MEXT,
#endif
};

#endif /* NRF_LIB_HW_UNIQUE_KEY_INTERNAL_H__ */

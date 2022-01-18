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
#include <sys/printk.h>
#include <kernel.h>
#define HUK_PRINT(msg) printk(msg)
#define HUK_PRINT_VAL(msg, val) printk(msg " %d", val)
#define HUK_PANIC k_panic

#elif defined(__NRF_TFM__)
#include "tfm_spm_log.h"
#include "utilities.h"
#define HUK_PRINT(msg) SPMLOG_DBGMSG(msg)
#define HUK_PRINT_VAL(msg, val) SPMLOG_DBGMSGVAL(msg, val)
#define HUK_PANIC tfm_core_panic

#else
static void panic(void)
{
	while (1)
		;
}
#define HUK_PRINT(...)
#define HUK_PANIC panic

#endif /* __ZEPHYR__ */

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

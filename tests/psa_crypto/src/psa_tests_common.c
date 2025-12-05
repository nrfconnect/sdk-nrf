/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <psa/crypto_extra.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include "psa_tests_common.h"
#include <hw_unique_key.h>

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#include <tfm_builtin_key_ids.h>
#include "cracen_psa_kmu.h"
#include <tfm_crypto_defs.h>
#else
#include <cracen_psa.h>
#endif

LOG_MODULE_REGISTER(ikg, LOG_LEVEL_INF);

ZTEST_SUITE(test_suite_ikg, NULL, NULL, NULL, NULL, NULL);

static psa_key_id_t key_id;

int crypto_init(void)
{
	psa_status_t status;

#if !defined(CONFIG_BUILD_WITH_TFM)
	if (!hw_unique_key_are_any_written()) {
		status = hw_unique_key_write_random();
		if (status != HW_UNIQUE_KEY_SUCCESS) {
			return status;
		}
	}
#endif
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return status;
	}
	return APP_SUCCESS;
}

int crypto_finish(void)
{
	psa_status_t status;

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return APP_SUCCESS;
}

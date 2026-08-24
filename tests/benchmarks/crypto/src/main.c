/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "crypto_benchmarks.h"

#include <zephyr/timing/timing.h>

LOG_MODULE_REGISTER(crypto_benchmarks, LOG_LEVEL_DBG);

int main(void)
{
	psa_status_t status;

	timing_init();

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed! (Error: %d)", status);
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	for (size_t i = 0; i < suite_group_cnt; i++) {
		const struct suite_group *group = &suite_groups[i];

		for (size_t j = 0; j < group->suite_cnt; j++) {
			run_suite(group, group->suites[j]);
		}
	}

	report_summary();

	if (failure_count() != 0) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

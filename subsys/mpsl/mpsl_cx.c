/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "mpsl_cx_internal.h"

#include <mpsl/mpsl_cx_config_thread.h>
#include <sys/util.h>

#if IS_ENABLED(CONFIG_MPSL_CX_THREAD)
static int cx_thread_configure(void)
{
	struct mpsl_cx_thread_interface_config cfg = {
		.request_pin  = CONFIG_MPSL_CX_THREAD_PIN_REQUEST,
		.priority_pin = CONFIG_MPSL_CX_THREAD_PIN_PRIORITY,
		.granted_pin  = CONFIG_MPSL_CX_THREAD_PIN_GRANT,
	};

	return mpsl_cx_thread_interface_config_set(&cfg);
}
#endif

int mpsl_cx_configure(void)
{
	int err = 0;

#if IS_ENABLED(CONFIG_MPSL_CX_THREAD)
	err = cx_thread_configure();
#else
#error Incomplete CONFIG_MPSL_CX configuration. No supported coexistence protocol found.
#endif

	return err;
}

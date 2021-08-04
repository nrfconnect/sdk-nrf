/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <device.h>
#include <sys/util.h>

#if IS_ENABLED(CONFIG_MPSL_CX_THREAD)
#include <mpsl/mpsl_cx_config_thread.h>

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

static int mpsl_cx_configure(void)
{
	int err = 0;

#if IS_ENABLED(CONFIG_MPSL_CX_THREAD)
	err = cx_thread_configure();
#else
#error Incomplete CONFIG_MPSL_CX configuration. No supported coexistence protocol found.
#endif

	return err;
}

static int mpsl_cx_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if IS_ENABLED(CONFIG_MPSL_CX)
	return mpsl_cx_configure();
#else
	return 0;
#endif
}

SYS_INIT(mpsl_cx_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

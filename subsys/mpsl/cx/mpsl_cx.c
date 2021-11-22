/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <device.h>
#include <sys/util.h>

#if IS_ENABLED(CONFIG_MPSL_CX_THREAD)
#include <mpsl/mpsl_cx_config_thread.h>

#if DT_NODE_HAS_STATUS(DT_PHANDLE(DT_NODELABEL(radio), coex), okay)
#define CX_NODE DT_PHANDLE(DT_NODELABEL(radio), coex)
#else
#define CX_NODE DT_INVALID_NODE
#error No enabled coex nodes registered in DTS.
#endif

static int cx_thread_configure(void)
{
	struct mpsl_cx_thread_interface_config cfg = {
		.request_pin  = NRF_DT_GPIOS_TO_PSEL(CX_NODE, req_gpios),
		.priority_pin = NRF_DT_GPIOS_TO_PSEL(CX_NODE, pri_dir_gpios),
		.granted_pin  = NRF_DT_GPIOS_TO_PSEL(CX_NODE, grant_gpios),
	};

	return mpsl_cx_thread_interface_config_set(&cfg);
}
#elif IS_ENABLED(CONFIG_MPSL_CX_BT_1WIRE)
#include <mpsl/mpsl_cx_config_bluetooth.h>

static inline int cx_bluetooth_1wire_configure(void)
{
	return mpsl_cx_bt_interface_1wire_config_set();
}
#elif IS_ENABLED(CONFIG_MPSL_CX_BT_3WIRE)
#include <mpsl/mpsl_cx_config_bluetooth.h>

static inline int cx_bluetooth_3wire_configure(void)
{
	return mpsl_cx_bt_interface_3wire_config_set();
}
#endif

static int mpsl_cx_configure(void)
{
	int err = 0;

#if IS_ENABLED(CONFIG_MPSL_CX_THREAD)
	err = cx_thread_configure();
#elif IS_ENABLED(CONFIG_MPSL_CX_BT_1WIRE)
	err = cx_bluetooth_1wire_configure();
#elif IS_ENABLED(CONFIG_MPSL_CX_BT_3WIRE)
	err = cx_bluetooth_3wire_configure();
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

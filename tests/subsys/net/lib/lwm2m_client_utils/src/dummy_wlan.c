/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/device.h>

/* Define a dummy driver just that the linker finds it. Otherwise we get a complaint like:
 *     undefined reference to `__device_dts_ord_12'
 */
#define DT_DRV_COMPAT nordic_wlan0
DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);

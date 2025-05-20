/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_POWER_REBOOT_H__
#define NRF_POWER_REBOOT_H__

/* In april 2021 the header reboot.h was moved from power/reboot.h to
 * sys/reboot.h. This breaks compatibility with any code that includes
 * <power/reboot.h>, so we create this file (which is located at
 * power/reboot.h) to give users time to port their code to #include
 * <sys/reboot.h>.
 */
#warning "<power/reboot.h> is deprecated, use <sys/reboot.h> instead"

#include <zephyr/sys/reboot.h>

#endif

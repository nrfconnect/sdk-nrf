/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SAMPLE_REBOOT_H_
#define _SAMPLE_REBOOT_H_

#include <zephyr/kernel.h>

/**
 * @brief Safely reboot sample.
 *
 * @param error - Is the reboot due to an error?
 */
void sample_reboot(const bool error);

static inline void sample_reboot_error(void)
{
	sample_reboot(true);
}

static inline void sample_reboot_normal(void)
{
	sample_reboot(false);
}

#endif /* _SAMPLE_REBOOT_H_ */

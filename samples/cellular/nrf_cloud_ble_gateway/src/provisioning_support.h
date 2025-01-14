/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SAMPLE_REBOOT_H_
#define _SAMPLE_REBOOT_H_

#include <zephyr/kernel.h>

/**
 * @brief Sleep until the provisioning library is idle.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if the provisioning library became idle in time.
 * @return false if timed out.
 */
#if defined(CONFIG_NRF_PROVISIONING)

bool await_provisioning_idle(k_timeout_t timeout);

#else /*CONFIG_NRF_PROVISIONING*/

static inline bool await_provisioning_idle(k_timeout_t timeout)
{
	return true;
}

#endif /*CONFIG_NRF_PROVISIONING*/

#endif /* _SAMPLE_REBOOT_H_ */

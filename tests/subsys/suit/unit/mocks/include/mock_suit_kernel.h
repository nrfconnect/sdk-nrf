/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_KERNEL_H__
#define MOCK_SUIT_KERNEL_H__

#include <zephyr/kernel.h>

FAKE_VALUE_FUNC(int, k_mutex_lock, struct k_mutex *, k_timeout_t);
FAKE_VALUE_FUNC(int, k_mutex_unlock, struct k_mutex *);

static inline void mock_suit_kernel_reset(void)
{
	RESET_FAKE(k_mutex_lock);
	RESET_FAKE(k_mutex_unlock);
}

#endif /* MOCK_SUIT_KERNEL_H__ */

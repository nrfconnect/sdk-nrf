/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#define ZEPHYR_INCLUDE_KERNEL_H_

/* CMock chokes on zephyr/kernel.h, so we define the minimum required to generate mocks
 * from this file instead.
 *
 * NOTE: This file is not included in the test source code, it is used only for mock generation.
 */

#include <stdint.h>

typedef struct k_timeout {
	uint64_t value;
} k_timeout_t;

typedef struct {
	/* We don't need full type definition for mockups */
} _wait_q_t;

struct k_sem {
	_wait_q_t wait_q;
	unsigned int count;
	unsigned int limit;
};

int z_impl_k_sem_init(struct k_sem *sem, unsigned int initial_count, unsigned int limit);

int z_impl_k_sem_take(struct k_sem *sem, k_timeout_t timeout);

void z_impl_k_sem_give(struct k_sem *sem);

#endif /* ZEPHYR_INCLUDE_KERNEL_H_ */

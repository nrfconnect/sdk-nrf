/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#define ZEPHYR_INCLUDE_KERNEL_H_
/* CMock chokes on zephyr/kernel.h, so we define the minimum required to build
 * mpsl_pm and tests, then let CMock generate mocks from this file instead.
 */
#include <stdint.h>
struct k_work {
};
typedef struct k_timeout {
	uint64_t value;
} k_timeout_t;
struct k_work_delayable {
	void *handler;
};
#define Z_WORK_DELAYABLE_INITIALIZER(work_handler) {work_handler}
#define K_WORK_DELAYABLE_DEFINE(work, work_handler)                                                \
	struct k_work_delayable work = Z_WORK_DELAYABLE_INITIALIZER(work_handler)
k_timeout_t K_USEC(uint64_t d);
int64_t k_uptime_get(void);
#endif

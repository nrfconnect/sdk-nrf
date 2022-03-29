/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* TEST_MULTICONTEXT */

#include <zephyr.h>

#define THREAD1_PRIORITY K_PRIO_COOP(1)
#define THREAD2_PRIORITY K_PRIO_COOP(2)

enum source_id {
	SOURCE_T1,
	SOURCE_T2,
	SOURCE_ISR,

	SOURCE_CNT
};

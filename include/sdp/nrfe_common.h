/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRFE_COMMON_H__
#define NRFE_COMMON_H__

#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __packed {
	atomic_bool locked;
	uint32_t data_size;
} nrfe_shared_data_lock_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFE_COMMON_H__ */

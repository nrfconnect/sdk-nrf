/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HPF_HPF_COMMON_H__
#define HPF_HPF_COMMON_H__

#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

enum data_lock_state {
	DATA_LOCK_STATE_OFF = 0,
	DATA_LOCK_STATE_BUSY,
	DATA_LOCK_STATE_WITH_DATA,
	DATA_LOCK_STATE_READY,
};

struct hpf_shared_data_lock {
	uint32_t data_size;
	atomic_t locked;
};

#ifdef __cplusplus
}
#endif

#endif /* HPF_HPF_COMMON_H__ */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SM_IPT_OS_H_
#define SM_IPT_OS_H_

#include <zephyr.h>

/**
 * @defgroup sm_ipt_os_zephyr SM IPT OS abstraction for Zephyr.
 * @{
 * @brief SM IPT OS abstraction for Zephyr.
 *
 * API is compatible with sm_ipt_os API. For API documentation
 * @see sm_ipt_os_tmpl.h
 */

#ifdef __cplusplus
extern "C" {
#endif


struct sm_ipt_os_ctx{
	void *out_shmem_ptr;
	void *in_shmem_ptr;
	uint32_t out_total_size;
	uint32_t in_total_size;
	void (*signal_handler)(struct sm_ipt_os_ctx *);
	const struct device *ipm_tx_handle;
};

#define SM_IPT_ASSERT(_expr) __ASSERT(_expr, "SM IPT assertion failed")

#define SM_IPT_OS_MEMORY_BARRIER() __DSB()

#define SM_IPT_OS_GET_CONTAINTER(ptr, type, field) CONTAINER_OF(ptr, type, field)

void sm_ipt_os_signal(struct sm_ipt_os_ctx *os_ctx);
void sm_ipt_os_signal_handler(struct sm_ipt_os_ctx *os_ctx, void (*handler)(struct sm_ipt_os_ctx *));
int sm_ipt_os_init(struct sm_ipt_os_ctx *os_ctx);

typedef atomic_t sm_ipt_os_atomic_t;
#define sm_ipt_os_atomic_or atomic_or
#define sm_ipt_os_atomic_and atomic_and
#define sm_ipt_os_atomic_get atomic_get

typedef struct k_mutex sm_ipt_os_mutex_t;
#define sm_ipt_os_mutex_init k_mutex_init
#define sm_ipt_os_unlock k_mutex_unlock

static inline void sm_ipt_os_lock(sm_ipt_os_mutex_t *mutex) {
	k_mutex_lock(mutex, K_FOREVER);
}

typedef struct k_sem sm_ipt_os_sem_t;
#define sm_ipt_os_give k_sem_give

static inline void sm_ipt_os_sem_init(sm_ipt_os_sem_t *sem) {
	k_sem_init(sem, 0, 1);
}
static inline void sm_ipt_os_take(sm_ipt_os_sem_t *sem) {
	k_sem_take(sem, K_FOREVER);
}

#define sm_ipt_os_yield k_yield
#define sm_ipt_os_fatal k_oops
#define sm_ipt_os_clz64 __builtin_clzll
#define sm_ipt_os_clz32 __builtin_clz


#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* SM_IPT_OS_H */

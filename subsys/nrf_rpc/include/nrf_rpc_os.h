/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_OS_H_
#define NRF_RPC_OS_H_

#include <zephyr.h>

/**
 * @defgroup nrf_rpc_os_zephyr nRF PRC OS abstraction for Zephyr.
 * @{
 * @brief nRF PRC OS abstraction for Zephyr.
 *
 * API is compatible with nrf_rpc_os API. For API documentation
 * @see nrf_rpc_os_tmpl.h
 */

#ifdef __cplusplus
extern "C" {
#endif

struct nrf_rpc_os_event {
	struct k_sem sem;
};

struct nrf_rpc_os_msg {
	struct k_sem sem;
	const uint8_t *data;
	size_t len;
};

typedef void (*nrf_rpc_os_work_t)(const uint8_t *data, size_t len);

int nrf_rpc_os_init(nrf_rpc_os_work_t callback);

void nrf_rpc_os_thread_pool_send(const uint8_t *data, size_t len);

static inline int nrf_rpc_os_event_init(struct nrf_rpc_os_event *event)
{
	return k_sem_init(&event->sem, 0, 1);
}

static inline void nrf_rpc_os_event_set(struct nrf_rpc_os_event *event)
{
	k_sem_give(&event->sem);
}

static inline void nrf_rpc_os_event_wait(struct nrf_rpc_os_event *event)
{
	k_sem_take(&event->sem, K_FOREVER);
}

static inline int nrf_rpc_os_msg_init(struct nrf_rpc_os_msg *msg)
{
	return k_sem_init(&msg->sem, 0, 1);
}

void nrf_rpc_os_msg_set(struct nrf_rpc_os_msg *msg, const uint8_t *data,
			size_t len);

void nrf_rpc_os_msg_get(struct nrf_rpc_os_msg *msg, const uint8_t **data,
			size_t *len);

static inline void *nrf_rpc_os_tls_get(void)
{
	return k_thread_custom_data_get();
}

static inline void nrf_rpc_os_tls_set(void *data)
{
	k_thread_custom_data_set(data);
}

uint32_t nrf_rpc_os_ctx_pool_reserve(void);
void nrf_rpc_os_ctx_pool_release(uint32_t number);

void nrf_rpc_os_remote_count(int count);

static inline void nrf_rpc_os_remote_reserve(void)
{
	extern struct k_sem _nrf_rpc_os_remote_counter;

	k_sem_take(&_nrf_rpc_os_remote_counter, K_FOREVER);
}

static inline void nrf_rpc_os_remote_release(void)
{
	extern struct k_sem _nrf_rpc_os_remote_counter;

	k_sem_give(&_nrf_rpc_os_remote_counter);
}

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* NRF_RPC_OS_H_ */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_OS_H_
#define NRF_RPC_OS_H_

#include <zephyr/kernel.h>
#include <nrf_rpc_errno.h>

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

#define NRF_RPC_OS_WAIT_FOREVER -1
#define NRF_RPC_OS_NO_WAIT 0

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

static inline int nrf_rpc_os_event_wait(struct nrf_rpc_os_event *event, int timeout)
{
	int err;

	err = k_sem_take(&event->sem, (timeout == NRF_RPC_OS_WAIT_FOREVER) ?
							K_FOREVER : K_MSEC(timeout));
	if (err == -EAGAIN) {
		return -NRF_EAGAIN;
	}

	return 0;
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

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* NRF_RPC_OS_H_ */

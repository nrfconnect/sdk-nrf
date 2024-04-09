/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_CLIENT_OS_H__
#define SSF_CLIENT_OS_H__

#include <sdfw/sdfw_services/ssf_errno.h>

#if defined(__ZEPHYR__)
#include "ssf_client_zephyr.h"
#else
#error "Unsupported ssf os"

/**
 * @brief       Logging wrappers to be implemented for each OS.
 */
#define SSF_CLIENT_LOG_REGISTER(module, log_lvl_cfg)
#define SSF_CLIENT_LOG_DECLARE(module, log_lvl_cfg)
#define SSF_CLIENT_LOG_ERR(...)
#define SSF_CLIENT_LOG_WRN(...)
#define SSF_CLIENT_LOG_INF(...)
#define SSF_CLIENT_LOG_DBG(...)

#endif

#define SSF_CLIENT_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define SSF_CLIENT_SEM_WAIT_FOREVER -1
#define SSF_CLIENT_SEM_NO_WAIT 0

/**
 * @brief       Wrapper for an OS specific binary semaphore context.
 */
struct ssf_client_sem;

/**
 * @brief       Initialize binary semaphore prior to first use.
 *
 * @param[in]   sem  A pointer to the semaphore.
 *
 * @return      0 on success, otherwise a negative SSF errno.
 */
int ssf_client_sem_init(struct ssf_client_sem *sem);

/**
 * @brief       Take semaphore.
 *
 * @param[in]   sem  A pointer to the semaphore.
 * @param[in]   timeout  Waiting period to take the semaphore, or one of the special
 *                       values SSF_CLIENT_SEM_WAIT_FOREVER and SSF_CLIENT_SEM_NO_WAIT.
 *
 * @return      0 on success, otherwise a negative SSF errno.
 */
int ssf_client_sem_take(struct ssf_client_sem *sem, int timeout);

/**
 * @brief       Give semaphore.
 *
 * @param[in]   sem  A pointer to the semaphore.
 */
void ssf_client_sem_give(struct ssf_client_sem *sem);

#endif /* SSF_CLIENT_OS_H__ */

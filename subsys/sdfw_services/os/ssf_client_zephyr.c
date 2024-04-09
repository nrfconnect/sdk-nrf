/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ssf_client_zephyr.h"

#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_errno.h>
#include "ssf_client_os.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

int ssf_client_sem_init(struct ssf_client_sem *sem)
{
	int err;

	err = k_sem_init(&sem->sem, 1, 1);
	if (err != 0) {
		return -SSF_EINVAL;
	}

	return 0;
}

int ssf_client_sem_take(struct ssf_client_sem *sem, int timeout)
{
	int err;

	err = k_sem_take(&sem->sem,
			 (timeout == SSF_CLIENT_SEM_WAIT_FOREVER) ? K_FOREVER : K_MSEC(timeout));
	if (err != 0) {
		return (err == -EAGAIN) ? -SSF_EAGAIN : SSF_EBUSY;
	}

	return 0;
}

void ssf_client_sem_give(struct ssf_client_sem *sem)
{
	k_sem_give(&sem->sem);
}

#if CONFIG_SSF_CLIENT_SYS_INIT
static int client_init(void)
{
	return ssf_client_init();
}

SYS_INIT(client_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

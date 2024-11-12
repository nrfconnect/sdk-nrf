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

#ifdef CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY
BUILD_ASSERT(CONFIG_SSF_CLIENT_SYS_INIT_PRIORITY > CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY,
	     "SSF_CLIENT_SYS_INIT_PRIORITY must be higher than IPC_SERVICE_REG_BACKEND_PRIORITY");
#endif

#ifdef CONFIG_NRF_802154_SER_RADIO_INIT_PRIO
BUILD_ASSERT(CONFIG_SSF_CLIENT_SYS_INIT_PRIORITY < CONFIG_NRF_802154_SER_RADIO_INIT_PRIO,
	     "SSF_CLIENT_SYS_INIT_PRIORITY must be lower than NRF_802154_SER_RADIO_INIT_PRIO");
#endif

BUILD_ASSERT(
	CONFIG_SSF_CLIENT_SYS_INIT_PRIORITY > CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	"SSF_CLIENT_SYS_INIT_PRIORITY must be higher than the IPC ICMSG initialization priority");

static int client_init(void)
{
	return ssf_client_init();
}

SYS_INIT(client_init, POST_KERNEL, CONFIG_SSF_CLIENT_SYS_INIT_PRIORITY);
#endif

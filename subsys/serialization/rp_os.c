/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <rp_os.h>

rp_err_t rp_os_signal_init(struct rp_ser *rp)
{
	int err;
	struct k_sem *sem = RP_OS_SIGNAL_GET(struct k_sem, rp);

	err = k_sem_init(sem, 0, 1);
	if (err) {
		return RP_ERROR_OS_ERROR;
	}

	return RP_SUCCESS;
}

rp_err_t rp_os_response_wait(struct rp_ser *rp)
{
	int err;
	struct k_sem *sem = RP_OS_SIGNAL_GET(struct k_sem, rp);

	err = k_sem_take(sem, CONFIG_RP_OS_RSP_WAIT_TIME);
	if (err) {
		return RP_ERROR_OS_ERROR;
	}

	return RP_SUCCESS;
}

rp_err_t rp_os_response_signal(struct rp_ser *rp)
{
	struct k_sem *sem = RP_OS_SIGNAL_GET(struct k_sem, rp);

	k_sem_give(sem);

	return RP_SUCCESS;
}

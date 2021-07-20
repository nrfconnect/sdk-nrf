/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sm_ipt_errno.h>
#include <sm_ipt_os.h>

#define SM_IPT_LOG_MODULE SM_IPT_OS
#include <sm_ipt_log.h>

#include <drivers/ipm.h>

#define SHM_NODE            DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR      DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE            DT_REG_SIZE(SHM_NODE)

void sm_ipt_os_signal(struct sm_ipt_os_ctx *os_ctx)
{
	int err = ipm_send(os_ctx->ipm_tx_handle, 0, 0, NULL, 0);
	if (err != 0) {
		LOG_ERR("Failed to notify: %d", err);
	}
}

void sm_ipt_os_signal_handler(struct sm_ipt_os_ctx *os_ctx, void (*handler)(struct sm_ipt_os_ctx *))
{
	os_ctx->signal_handler = handler;
}

static void ipm_callback(const struct device *ipmdev, void *user_data, uint32_t id,
			 volatile void *data)
{
	struct sm_ipt_os_ctx *os_ctx = (struct sm_ipt_os_ctx *)user_data;

	if (os_ctx->signal_handler)
		os_ctx->signal_handler(os_ctx);
}

int sm_ipt_os_init(struct sm_ipt_os_ctx *os_ctx)
{
	const struct device *ipm_rx_handle;
	uint32_t size = WB_DN(SHM_SIZE / 2);

	os_ctx->out_total_size = size;
	os_ctx->in_total_size = size;

	uint32_t addr1 = SHM_START_ADDR;
	uint32_t addr2 = SHM_START_ADDR + size;

	if (IS_ENABLED(CONFIG_SM_IPT_PRIMARY)) {
		os_ctx->out_shmem_ptr = (void*)addr1;
		os_ctx->in_shmem_ptr = (void*)addr2;
	} else {
		os_ctx->out_shmem_ptr = (void*)addr2;
		os_ctx->in_shmem_ptr = (void*)addr1;
	}

	/* IPM setup. */
	os_ctx->ipm_tx_handle = device_get_binding(IS_ENABLED(CONFIG_SM_IPT_PRIMARY) ?
					   "IPM_1" : "IPM_0");
	if (!os_ctx->ipm_tx_handle) {
		LOG_ERR("Could not get TX IPM device handle");
		return -NRF_ENODEV;
	}

	ipm_rx_handle = device_get_binding(IS_ENABLED(CONFIG_SM_IPT_PRIMARY) ?
					   "IPM_0" : "IPM_1");
	if (!ipm_rx_handle) {
		LOG_ERR("Could not get RX IPM device handle");
		return -NRF_ENODEV;
	}

	ipm_register_callback(ipm_rx_handle, ipm_callback, os_ctx);

	return 0;
}

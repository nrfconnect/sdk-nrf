/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#if defined(CONFIG_NRF_RPC)
#include <nrf_rpc_tr.h>
#include <nrf_rpc/nrf_rpc_uart.h>
#include <nrf_rpc_errno.h>
#include <nrf_rpc_cbor.h>
#endif
#include <sys/errno.h>

LOG_MODULE_REGISTER(nrf_rpc_host, CONFIG_NRF_RPC_HOST_LOG_LEVEL);

#if defined(CONFIG_NRF_RPC)
static void err_handler(const struct nrf_rpc_err_report *report)
{
	printk("nRF RPC error %d ocurred. See nRF RPC logs for more details.\r\n",
	       report->code);
	//k_oops();
}

static int serialization_init(void)
{

	int err;

	printk("Init begin\n");

	err = nrf_rpc_init(err_handler);
	if (err) {
		LOG_ERR("Init failed %d\n", err);
		return -NRF_EINVAL;
	}

	printk("Init done\n");

	return 0;
}
#endif

int main(void)
{
#if defined(CONFIG_NRF_RPC)
	int ret;

	k_sleep(K_MSEC(3000));
	ret = serialization_init();

	if (ret != 0) {
		LOG_ERR("Init RPC Failed.");
	}
#endif

	printk("Welcome to RPC host\r\n");

	return 0;
}

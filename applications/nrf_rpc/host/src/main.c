/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <nrf_rpc_tr.h>
#include <nrf_rpc/nrf_rpc_uart.h>
#include <nrf_rpc_errno.h>
#include <nrf_rpc_cbor.h>
#include <sys/errno.h>


LOG_MODULE_REGISTER(nrf_rpc_host, CONFIG_NRF_RPC_HOST_LOG_LEVEL);

//NRF_RPC_UART_TRANSPORT(uart_tr, DEVICE_DT_GET(DT_NODELABEL(uart0)));
//NRF_RPC_GROUP_DEFINE(nrf_rpc_group, "nrf_uart_rpc", &uart_tr, NULL, NULL, NULL);
/*
struct remote_inc_result {
        int err;
        int output;
};

static void remote_inc_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
                           void *handler_data)
{
        struct remote_inc_result *result =
                (struct remote_inc_result *)handler_data;

        if (zcbor_int32_decode(ctx->zs, &result->output)) {
                result->err = 0;
        } else {
                result->err = -NRF_EINVAL;
        }
}

int remote_inc(int input, int *output)
{
        struct remote_inc_result result;
        struct nrf_rpc_cbor_ctx ctx;

        NRF_RPC_CBOR_ALLOC(&nrf_rpc_group, ctx, MAX_ENCODED_LEN);

        if (zcbor_int32_put(ctx.zs, input)) {
                if (!nrf_rpc_cbor_cmd(&nrf_rpc_group, RPC_COMMAND_INC, &ctx,
                                remote_inc_rsp, &result)) {
                        *output = result.output;
                        return result.err;
                } else {
                        return -NRF_EINVAL;
                }
        }
        return -NRF_EINVAL;
}
*/
static void err_handler(const struct nrf_rpc_err_report *report)
{
	printk("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
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

int main(void)
{
	int ret = usb_enable(NULL);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}
	k_sleep(K_MSEC(5000));
	ret = serialization_init();

	if (ret != 0) {
		LOG_ERR("Init RPC Failed.");
	}

	printk("Welcome to RPC host\r\n");
	/*while (true) {
		k_sleep(K_MSEC(5000));
		int input = 0;
		int output = 0;
		//remote_inc(input, &output);

		printk("input: %d output: %d\r\n", input, output);
	}*/
	return 0;
}

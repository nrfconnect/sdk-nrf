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

LOG_MODULE_REGISTER(nrf_rpc_remote, CONFIG_NRF_RPC_REMOTE_LOG_LEVEL);

//NRF_RPC_UART_TRANSPORT(uart_tr, DEVICE_DT_GET(DT_NODELABEL(uart0)));
//NRF_RPC_GROUP_DEFINE(nrf_rpc_sample_group, "nrf_uart_rpc", &uart_tr, NULL, NULL, NULL);
/*
static void remote_inc_handler(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx, void* handler_data)
{
        int err;
        int input = 0;
        int output;
        struct nrf_rpc_cbor_ctx nctx;


        zcbor_int32_decode(ctx->zs, &input);

        nrf_rpc_cbor_decoding_done(group, ctx);


        output = input + 1;


        NRF_RPC_CBOR_ALLOC(group, nctx, MAX_ENCODED_LEN);

        zcbor_int32_put(nctx.zs, output);

        err = nrf_rpc_cbor_rsp(group, &nctx);

        if (err < 0) {
            LOG_ERR("Sending response failed\n");
        }
}

NRF_RPC_CBOR_CMD_DECODER(nrf_rpc_sample_group, remote_inc_handler,
                         RPC_COMMAND_INC, remote_inc_handler, NULL);
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

extern volatile int num_interrupts;

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

	printk("Welcome to RPC remote\r\n");

	/*while(1) {
		k_sleep(K_MSEC(1000));
		printk("interrupts_received %d\r\n", num_interrupts);
	}*/

	return 0;
}

//SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

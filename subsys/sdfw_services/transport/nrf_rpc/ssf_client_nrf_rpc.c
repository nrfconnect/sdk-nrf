/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>

#include "ssf_client_transport.h"

#include <sdfw/sdfw_services/ssf_errno.h>
#include "ssf_client_os.h"
#include "ssf_nrf_rpc_common.h"
#include <nrf_rpc/nrf_rpc_ipc.h>
#include <zephyr/devicetree.h>

SSF_CLIENT_LOG_DECLARE(ssf_client, CONFIG_SSF_CLIENT_LOG_LEVEL);

#if CONFIG_SSF_CLIENT_DOMAIN_ID == 2
NRF_RPC_IPC_TRANSPORT(ssf_transport, DEVICE_DT_GET(DT_NODELABEL(cpusec_cpuapp_ipc)), "ssf_ept");
#elif CONFIG_SSF_CLIENT_DOMAIN_ID == 3
NRF_RPC_IPC_TRANSPORT(ssf_transport, DEVICE_DT_GET(DT_NODELABEL(cpusec_cpurad_ipc)), "ssf_ept");
#else
#error Unsupported domain
#endif
NRF_RPC_GROUP_DEFINE(ssf_group, "ssf_" CONFIG_SSF_CLIENT_NRF_RPC_GROUP_NAME, &ssf_transport, NULL,
		     NULL, NULL);

static bool transport_initialized;
static ssf_client_transport_notif_handler notif_handler;

static void ssf_notification_handler(const struct nrf_rpc_group *group, const uint8_t *packet,
				     size_t len, void *handler_data)
{
	ARG_UNUSED(group);
	ARG_UNUSED(handler_data);

	if (notif_handler != NULL && packet != NULL) {
		notif_handler(packet, len);
	}
}

NRF_RPC_EVT_DECODER(ssf_group, ssf_notif_decoder, CONFIG_SSF_NRF_RPC_NOTIF_ID,
		    ssf_notification_handler, NULL);

static void err_handler(const struct nrf_rpc_err_report *report)
{
	SSF_CLIENT_LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
			   report->code);
}

int ssf_client_transport_init(ssf_client_transport_notif_handler handler)
{
	int err;

	if (handler == NULL) {
		return -SSF_EINVAL;
	}
	notif_handler = handler;

	transport_initialized = false;

	err = nrf_rpc_init(err_handler);
	if (err != 0) {
		return -SSF_EINVAL;
	}

	transport_initialized = true;

	return 0;
}

int ssf_client_transport_alloc_tx_buf(uint8_t **buf, size_t len)
{
	if (buf == NULL) {
		return -SSF_EINVAL;
	}

	if (!transport_initialized) {
		return -SSF_EBUSY;
	}

	nrf_rpc_alloc_tx_buf(&ssf_group, buf, len);
	if (*buf == NULL) {
		return -SSF_ENOMEM;
	}

	return 0;
}

void ssf_client_transport_free_tx_buf(uint8_t *buf)
{
	if (!transport_initialized) {
		return;
	}

	nrf_rpc_free_tx_buf(&ssf_group, buf);
}

void ssf_client_transport_decoding_done(const uint8_t *packet)
{
	if (!transport_initialized) {
		return;
	}

	nrf_rpc_decoding_done(&ssf_group, packet);
}

int ssf_client_transport_request_send(uint8_t *pkt, size_t pkt_len, const uint8_t **rsp_pkt,
				      size_t *rsp_pkt_len)
{
	int err;

	if (pkt == NULL) {
		return -SSF_EINVAL;
	}

	if (rsp_pkt == NULL || rsp_pkt_len == NULL) {
		err = -SSF_EINVAL;
		goto free_request_and_exit;
	}

	if (!transport_initialized) {
		err = -SSF_EBUSY;
		goto free_request_and_exit;
	}

	err = ssf_translate_error(nrf_rpc_cmd_rsp(&ssf_group, CONFIG_SSF_NRF_RPC_CMD_ID, pkt,
						  pkt_len, rsp_pkt, rsp_pkt_len));
	if (err == -SSF_EFAULT) {
		goto free_request_and_exit;
	}

	return err;

free_request_and_exit:
	nrf_rpc_free_tx_buf(&ssf_group, pkt);
	return err;
}

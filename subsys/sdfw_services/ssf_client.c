/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sdfw/sdfw_services/ssf_client.h>

#include <sdfw/sdfw_services/ssf_client_notif.h>
#include <sdfw/sdfw_services/ssf_errno.h>
#include "ssf_client_os.h"
#include "ssf_client_transport.h"

#include <zcbor_decode.h>
#include <zcbor_encode.h>

SSF_CLIENT_LOG_REGISTER(ssf_client, CONFIG_SSF_CLIENT_LOG_LEVEL);

int ssf_client_notif_init(void);
void ssf_client_notif_handler(const uint8_t *pkt, size_t pkt_len);

struct service_context {
	const struct ssf_client_srvc *srvc;
	size_t hdr_len;
	size_t pl_len;
	uint8_t *req_pkt;
	size_t req_pkt_len;
	const uint8_t *rsp_pkt;
	size_t rsp_pkt_len;
	int32_t ssf_err;
};

static int raw_payload_copy(uint8_t *dest, size_t dest_len, const uint8_t *src, size_t src_len,
			    size_t *bytes_written)
{
	if (dest_len < src_len) {
		return -SSF_ENOMEM;
	}

	if (dest == NULL || src == NULL) {
		return -SSF_EFAULT;
	}

	memcpy(dest, src, src_len);
	if (bytes_written != NULL) {
		*bytes_written = src_len;
	}

	return 0;
}

static int encode_header(struct service_context *ctx)
{
	const struct ssf_client_srvc *srvc = ctx->srvc;

	ZCBOR_STATE_E(state, CONFIG_SSF_CLIENT_ZCBOR_MAX_BACKUPS, ctx->req_pkt, srvc->req_buf_size,
		      1);

	/* Header proceeding all ssf service requests:
	 *     - remote ID of requesting remote
	 *     - service ID of the requested service
	 *     - service version of the requested service
	 */
	bool success = zcbor_list_start_encode(state, 3) &&
		       ((zcbor_uint32_put(state, CONFIG_SSF_CLIENT_DOMAIN_ID) &&
			 zcbor_uint32_put(state, (uint32_t)srvc->id) &&
			 zcbor_uint32_put(state, (uint32_t)srvc->version)) ||
			(zcbor_list_map_end_force_encode(state), false)) &&
		       zcbor_list_end_encode(state, 3);

	if (!success) {
		return -SSF_EPROTO;
	}

	ctx->hdr_len = state->payload - ctx->req_pkt;

	return 0;
}

static int encode_payload(struct service_context *ctx, void *req, bool is_raw_data)
{
	int err;

	if (is_raw_data) {
		struct ssf_client_raw_data *raw = (struct ssf_client_raw_data *)req;

		err = raw_payload_copy(&ctx->req_pkt[ctx->hdr_len],
				       ctx->srvc->req_buf_size - ctx->hdr_len, raw->data, raw->len,
				       &ctx->pl_len);
	} else {
		err = ctx->srvc->req_encode(&ctx->req_pkt[ctx->hdr_len],
					    ctx->srvc->req_buf_size - ctx->hdr_len, req,
					    &ctx->pl_len);
	}

	return err;
}

static int send_request(struct service_context *ctx)
{
	int err;

	ctx->req_pkt_len = ctx->hdr_len + ctx->pl_len;

	SSF_CLIENT_LOG_DBG("Sending request: id 0x%x, remote %d, pkt-len %d", ctx->srvc->id,
			   CONFIG_SSF_CLIENT_DOMAIN_ID, ctx->req_pkt_len);

	err = ssf_client_transport_request_send(ctx->req_pkt, ctx->req_pkt_len, &ctx->rsp_pkt,
						&ctx->rsp_pkt_len);
	if (err != 0) {
		if (err == -SSF_EMSGSIZE) {
			return err;
		}
		return -SSF_EIO;
	}

	return err;
}

static int decode_header(struct service_context *ctx)
{
	ZCBOR_STATE_D(state, CONFIG_SSF_CLIENT_ZCBOR_MAX_BACKUPS, ctx->rsp_pkt, ctx->rsp_pkt_len,
		      1, 0);

	/* Header proceeding all ssf service responses
	 *     - ssf error: zero on success, otherwise an errno
	 *                  indicating an ssf error on the server side.
	 */
	bool success = zcbor_list_start_decode(state) &&
		       (zcbor_int32_decode(state, &ctx->ssf_err) ||
			(zcbor_list_map_end_force_decode(state), false)) &&
		       zcbor_list_end_decode(state);

	if (!success) {
		return -SSF_EPROTO;
	}

	/* Previously used for request header length. Reuse for response header length. */
	ctx->hdr_len = state->payload - ctx->rsp_pkt;

	return 0;
}

static int decode_payload(struct service_context *ctx, void *rsp, bool is_raw_data)
{
	int err;

	if (is_raw_data) {
		struct ssf_client_raw_data *raw = (struct ssf_client_raw_data *)rsp;

		err = raw_payload_copy(raw->data, raw->len, &ctx->rsp_pkt[ctx->hdr_len],
				       ctx->rsp_pkt_len - ctx->hdr_len, &raw->len);
	} else {
		err = ctx->srvc->rsp_decode(&ctx->rsp_pkt[ctx->hdr_len],
					    ctx->rsp_pkt_len - ctx->hdr_len, rsp, NULL);
	}

	if (err != 0) {
		return -SSF_EPROTO;
	}

	SSF_CLIENT_LOG_DBG("Received response: id 0x%x, remote %d, pkt-len 0x%x", ctx->srvc->id,
			   CONFIG_SSF_CLIENT_DOMAIN_ID, ctx->rsp_pkt_len);

	return err;
}

static int process_request(const struct ssf_client_srvc *srvc, void *req, void *rsp,
			   const uint8_t **rsp_pkt, bool is_raw_data)
{
	int err;
	struct service_context ctx;

	ctx.srvc = srvc;

	err = ssf_client_transport_alloc_tx_buf(&ctx.req_pkt, srvc->req_buf_size);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Failed to allocate request buffer, err %d", err);
		return err;
	}

	err = encode_header(&ctx);
	if (err != 0) {
		ssf_client_transport_free_tx_buf(ctx.req_pkt);
		SSF_CLIENT_LOG_ERR("Failed to encode request header, zcbor err %d", err);
		return -SSF_EPROTO;
	}

	err = encode_payload(&ctx, req, is_raw_data);
	if (err != 0) {
		ssf_client_transport_free_tx_buf(ctx.req_pkt);
		SSF_CLIENT_LOG_ERR("Failed to encode request payload, err %d", err);
		return -SSF_EPROTO;
	}

	err = send_request(&ctx);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Failed to send request with id 0x%x, err %d", srvc->id, err);
		return err;
	}

	err = decode_header(&ctx);
	if (err != 0) {
		ssf_client_transport_decoding_done(ctx.rsp_pkt);
		SSF_CLIENT_LOG_ERR("Failed decode header, zcbor err %d", err);
		return -SSF_EPROTO;
	}

	if (ctx.ssf_err != 0) {
		ssf_client_transport_decoding_done(ctx.rsp_pkt);
		SSF_CLIENT_LOG_ERR("Request with id 0x%x failed, rsp %d", srvc->id, ctx.ssf_err);
		return ctx.ssf_err;
	}

	err = decode_payload(&ctx, rsp, is_raw_data);
	if (err != 0) {
		ssf_client_transport_decoding_done(ctx.rsp_pkt);
		SSF_CLIENT_LOG_ERR("Failed to decode response payload, err %d", err);
		return err;
	}

	if (rsp_pkt == NULL) {
		ssf_client_transport_decoding_done(ctx.rsp_pkt);
	} else {
		*rsp_pkt = ctx.rsp_pkt;
	}

	return 0;
}

int ssf_client_init(void)
{
	int err;

	err = ssf_client_notif_init();
	if (err != 0) {
		return -SSF_EINVAL;
	}

	err = ssf_client_transport_init(ssf_client_notif_handler);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Failed to initialize transport");
		return -SSF_EINVAL;
	}

	return 0;
}

int ssf_client_send_request(const struct ssf_client_srvc *srvc, void *req, void *decoded_rsp,
			    const uint8_t **rsp_pkt)
{
	if (srvc == NULL || req == NULL || decoded_rsp == NULL) {
		return -SSF_EINVAL;
	}
	if (srvc->req_encode == NULL || srvc->rsp_decode == NULL) {
		return -SSF_EPROTO;
	}

	return process_request(srvc, req, decoded_rsp, rsp_pkt, false);
}

int ssf_client_send_raw_request(const struct ssf_client_srvc *srvc,
				struct ssf_client_raw_data raw_req,
				struct ssf_client_raw_data *raw_rsp)
{
	int err;

	if (srvc == NULL || raw_rsp == NULL) {
		return -SSF_EINVAL;
	}

	err = process_request(srvc, (void *)&raw_req, (void *)raw_rsp, NULL, true);

	return err;
}

void ssf_client_decode_done(const uint8_t *rsp_pkt)
{
	ssf_client_transport_decoding_done(rsp_pkt);
}

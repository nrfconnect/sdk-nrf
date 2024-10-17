/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/socket.h>
#include <net/download_client.h>
#include <net/download_client_transport.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include "download_client_internal.h"


LOG_MODULE_DECLARE(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define COAP_VER 1
#define FILENAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE
#define COAP_PATH_ELEM_DELIM "/"

struct transport_params_coap {
	/** Initialization status */
	bool initialized;
	/** CoAP block context. */
	struct coap_block_context block_ctx;
	/** CoAP pending object. */
	struct coap_pending pending;

	struct {
		/** Socket descriptor. */
		int fd;
		/** Protocol for current download. */
		int proto;
		/** Socket type */
		int type;
		/** Port */
		uint16_t port;
		/** Destination address storage */
		struct sockaddr remote_addr;
	} sock;

	/** Request new data */
	bool new_data_req;
	/** Request retransmission */
	bool retransmission_req;
};

BUILD_ASSERT(CONFIG_DOWNLOAD_CLIENT_TRANSPORT_PARAMS_SIZE >= sizeof(struct transport_params_coap));

/* declaration of strtok_r appears to be missing in some cases,
 * even though it's defined in the minimal libc, so we forward declare it
 */
extern char *strtok_r(char *str, const char *sep, char **state);

int url_parse_file(const char *url, char *file, size_t len);

static int coap_get_current_from_response_pkt(const struct coap_packet *cpkt)
{
	int block = 0;

	block = coap_get_option_int(cpkt, COAP_OPTION_BLOCK2);
	if (block < 0) {
		return block;
	}

	return GET_BLOCK_NUM(block) << (GET_BLOCK_SIZE(block) + 4);
}

static bool has_pending(struct download_client *dlc)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;
	return coap->pending.timeout > 0;
}

int coap_block_init(struct download_client *dlc, size_t from)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	if (coap->initialized) {
		return 0;
	}

	coap_block_transfer_init(&coap->block_ctx,
				 5, 0); //TODO 5 replaced by CONFIG_DOWNLOAD_CLIENT_COAP_BLOCK_SIZE
	coap->block_ctx.current = from;
	coap_pending_clear(&coap->pending);

	coap->initialized = true;
	return 0;
}

int coap_get_recv_timeout(struct download_client *dlc)
{
	int timeout;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	__ASSERT(has_pending(dlc), "Must have coap pending");

	/* Retransmission is cycled in case recv() times out. In case sending request
	 * blocks, the time that is used for sending request must be substracted next time
	 * recv() is called.
	 */
	timeout = coap->pending.t0 + coap->pending.timeout - k_uptime_get_32();
	if (timeout < 0) {
		/* All time is spent when sending request and time this
		 * method is called, there is no time left for receiving;
		 * skip over recv() and initiate retransmission on next
		 * cycle
		 */
		return 0;
	}

	return timeout;
}

int coap_initiate_retransmission(struct download_client *dlc)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	if (coap->pending.timeout == 0) {
		return -EINVAL;
	}

	if (!coap_pending_cycle(&coap->pending)) {
		LOG_ERR("CoAP max-retransmissions exceeded");
		return -1;
	}

	return 0;
}

static int coap_block_update(struct download_client *dlc, struct coap_packet *pkt,
			     size_t *blk_off, bool *more)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	int err, new_current;

	*blk_off = coap->block_ctx.current %
		   coap_block_size_to_bytes(coap->block_ctx.block_size);
	if (*blk_off) {
		LOG_DBG("%d bytes of current block already downloaded",
			*blk_off);
	}

	new_current = coap_get_current_from_response_pkt(pkt);
	if (new_current < 0) {
		LOG_ERR("Failed to get current from CoAP packet, err %d", new_current);
		return new_current;
	}

	if (new_current < coap->block_ctx.current) {
		LOG_WRN("Block out of order %d, expected %d", new_current,
			coap->block_ctx.current);
		return -1;
	} else if (new_current > coap->block_ctx.current) {
		LOG_WRN("Block out of order %d, expected %d", new_current,
			coap->block_ctx.current);
		return -1;
	}

	err = coap_update_from_block(pkt, &coap->block_ctx);
	if (err) {
		return err;
	}

	if (dlc->file_size == 0 && coap->block_ctx.total_size > 0) {
		LOG_DBG("Total size: %d", coap->block_ctx.total_size);
		dlc->file_size = coap->block_ctx.total_size;
	}

	*more = coap_next_block(pkt, &coap->block_ctx);
	if (!*more) {
		LOG_DBG("Last block received");
	}

	return 0;
}

static int coap_parse(struct download_client *dlc, size_t len)
{
	int err;
	size_t blk_off;
	uint8_t response_code;
	uint16_t payload_len;
	const uint8_t *payload;
	struct coap_packet response;
	bool more;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	/* TODO: currently we stop download on every error, but this is mostly not necessary
	 * and we can just request the same block again using retry mechanism
	 */

	err = coap_packet_parse(&response, dlc->config.buf, len, NULL, 0);
	if (err) {
		LOG_ERR("Failed to parse CoAP packet, err %d", err);
		return -EBADMSG;
	}


	if (coap_header_get_id(&response) != coap->pending.id) {
		LOG_ERR("Response is not pending");
		return -EBADMSG;
	}

	coap_pending_clear(&coap->pending);

	if (coap_header_get_type(&response) != COAP_TYPE_ACK) {
		LOG_ERR("Response must be of coap type ACK");
		return -EBADMSG;
	}

	response_code = coap_header_get_code(&response);
	if (response_code != COAP_RESPONSE_CODE_OK &&
	    response_code != COAP_RESPONSE_CODE_CONTENT) {
		LOG_ERR("Server responded with code 0x%x", response_code);
		return -EBADMSG;
	}

	err = coap_block_update(dlc, &response, &blk_off, &more);
	if (err) {
		return -EBADMSG;
	}

	payload = coap_packet_get_payload(&response, &payload_len);
	if (!payload) {
		LOG_WRN("No CoAP payload!");
		return -EBADMSG;
	}

	/* TODO: because our buffer is large enough for the whole datagram,
	 * we don't scrictly need to copy the bytes at the beginning
	 * of the buffer, we could simply send a fragment pointing to the
	 * payload directly.
	 */
	LOG_DBG("CoAP response: %d, copying %d bytes",
		coap_header_get_code(&response), payload_len - blk_off);
	memcpy(dlc->config.buf + dlc->buf_offset, payload + blk_off,
	       payload_len - blk_off);

	dlc->buf_offset += payload_len - blk_off;
	dlc->progress += payload_len - blk_off;

	if (!more) {
		/* Mark the end, in case we did not know the total size */
		dlc->file_size = dlc->progress;
	}

	coap->new_data_req = true;
	return 0;
}

static int coap_request_send(struct download_client *dlc)
{
	int err;
	uint16_t id;
	char file[FILENAME_SIZE];
	char *path_elem;
	char *path_elem_saveptr;
	struct coap_packet request;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	if (has_pending(dlc)) {
		id = coap->pending.id;
	} else {
		id = coap_next_id();
	}

	err = coap_packet_init(&request, dlc->config.buf, dlc->config.buf_size, COAP_VER,
			       COAP_TYPE_CON, 8, coap_next_token(), COAP_METHOD_GET, id);
	if (err) {
		LOG_ERR("Failed to init CoAP message, err %d", err);
		return err;
	}

	err = url_parse_file(dlc->file, file, sizeof(file));
	if (err) {
		LOG_ERR("Unable to parse url");
		return err;
	}

	path_elem = strtok_r(file, COAP_PATH_ELEM_DELIM, &path_elem_saveptr);
	do {
		err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
			path_elem, strlen(path_elem));
		if (err) {
			LOG_ERR("Unable add option to request");
			return err;
		}
	} while ((path_elem = strtok_r(NULL, COAP_PATH_ELEM_DELIM, &path_elem_saveptr)));

	err = coap_append_block2_option(&request, &coap->block_ctx);
	if (err) {
		LOG_ERR("Unable to add block2 option");
		return err;
	}

	err = coap_append_size2_option(&request, &coap->block_ctx);
	if (err) {
		LOG_ERR("Unable to add size2 option");
		return err;
	}

	if (!has_pending(dlc)) {
		struct coap_transmission_parameters params = coap_get_transmission_parameters();

		params.max_retransmission =
			CONFIG_DOWNLOAD_CLIENT_COAP_MAX_RETRANSMIT_REQUEST_COUNT;
		err = coap_pending_init(&coap->pending, &request, &coap->sock.remote_addr,
					&params);
		if (err < 0) {
			return -EINVAL;
		}

		coap_pending_cycle(&coap->pending);
	}

	LOG_DBG("CoAP next block: %d", coap->block_ctx.current);

	err = client_socket_send_timeout_set(coap->sock.fd,  coap->pending.timeout);
	if (err) {
		return err;
	}

	err = client_socket_send(coap->sock.fd, dlc->config.buf, request.offset);
	if (err) {
		LOG_ERR("Failed to send CoAP request, errno %d", errno);
		return err;
	}

	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(request.data, request.offset, "CoAP request");
	}

	return 0;
}

static bool dlc_coap_proto_supported(struct download_client *dlc, const char *uri)
{
	if (strncmp(uri, "coaps://", 8) == 0) {
		return true;
	} else if (strncmp(uri, "coap://", 7) == 0) {
		return true;
	}

	return false;
}

static int dlc_coap_init(struct download_client *dlc, struct download_client_host_cfg *host_cfg, const char *uri)
{
	int err;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	coap->sock.proto = IPPROTO_UDP;
	coap->sock.type = SOCK_DGRAM;

	if (strncmp(uri, "coaps://", 8) == 0) {
		coap->sock.proto = IPPROTO_DTLS_1_2;
		coap->sock.type = SOCK_DGRAM;

		if (host_cfg->sec_tag_list == NULL || host_cfg->sec_tag_count == 0) {
			LOG_WRN("No security tag provided for TLS/DTLS");
			return -EINVAL;
		}
	}

	err = url_parse_port(uri, &coap->sock.port);
	if (err) {
		switch (coap->sock.proto) {
		case IPPROTO_DTLS_1_2:
			coap->sock.port = 5684;
			break;
		case IPPROTO_UDP:
			coap->sock.port = 5683;
			break;
		}
		LOG_DBG("Port not specified, using default: %d", coap->sock.port);
	}

	if (host_cfg->set_native_tls) {
		LOG_DBG("Enabled native TLS");
		coap->sock.type |= SOCK_NATIVE_TLS;
	}

	return 0;
}

static int dlc_coap_deinit(struct download_client *dlc)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	if (coap->sock.fd != -1) {
		client_socket_close(&coap->sock.fd);
		dlc_transport_evt_disconnected(dlc);
	}

	return 0;
}

static int dlc_coap_connect(struct download_client *dlc)
{
	int err;
	int ns_err;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	err = -1;
	ns_err = -1;

	if (!coap->sock.remote_addr.sa_family) {
		err = client_socket_host_lookup(dlc->hostname, dlc->host_config.pdn_id, &http->sock.remote_addr);
		if (err) {
			return err;
		}
	}

	err = client_socket_configure_and_connect(
		&coap->sock.fd, coap->sock.proto, coap->sock.type, coap->sock.port,
		&coap->sock.remote_addr, dlc->hostname, &dlc->host_config);
	if (err) {
		goto cleanup;
	}

	coap_block_init(dlc, dlc->progress);

cleanup:
	if (err) {
		/* Unable to connect, close socket */
		client_socket_close(&coap->sock.fd);
	}

	return 0;
}

static int dlc_coap_close(struct download_client *dlc)
{
	int err;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	if (coap->sock.fd != -1) {
		err = client_socket_close(&coap->sock.fd);
		dlc_transport_evt_disconnected(dlc);
		return err;
	}

	return -EBADF;
}

static int dlc_coap_download(struct download_client *dlc)
{
	int ret, len, timeout;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dlc->transport_internal;

	//if (coap->connection_close) {
	//	return -ECONNRESET;
	//}

	if (coap->new_data_req) {
		/* Request next fragment */
		dlc->buf_offset = 0;
		ret = coap_request_send(dlc);
		if (ret) {
			LOG_DBG("data_req failed, err %d", ret);
			/** Attempt reconnection. */
			return -ECONNRESET;
		}

		coap->new_data_req = false;
	}

	if (coap->retransmission_req) {
		dlc->buf_offset = 0; //TODO should this be here?
		ret = coap_initiate_retransmission(dlc);
		if (ret) {
			LOG_DBG("retransmission_req failed, err %d", ret);
			/** Attempt reconnection. */
			return -ECONNRESET;
		}

		coap->retransmission_req = false;
	}

	__ASSERT(dlc->buf_offset < dlc->config.buf_size, "Buffer overflow");

	LOG_DBG("Receiving up to %d bytes at %p...",
		(dlc->config.buf_size - dlc->buf_offset),
		(void *)(dlc->config.buf + dlc->buf_offset));

	timeout = coap_get_recv_timeout(dlc);
	if (!timeout) {
		return -ETIMEDOUT;
	}

	ret = client_socket_recv_timeout_set(coap->sock.fd, timeout);
	if (ret) {
		return ret;
	}

	len = client_socket_recv(coap->sock.fd,
				 dlc->config.buf + dlc->buf_offset,
				 dlc->config.buf_size - dlc->buf_offset);
	if (len < 0) {
		if ((len == ETIMEDOUT) ||
		    (len == EWOULDBLOCK) ||
		    (len == EAGAIN)) {
			/* Request data again */
			coap->retransmission_req = true;
			return 0;
		}

		return len;
	}

	ret = coap_parse(dlc, len);
	if (ret < 0) {
		/* Request data again */
		coap->retransmission_req = true;
		return 0;
	}

	/* Accumulate buffer offset */
	dlc->progress += len;
	dlc->buf_offset += len;

	dlc_transport_evt_data(dlc, dlc->config.buf, len);
	if (dlc->progress == dlc->file_size) {
		dlc_transport_evt_download_complete(dlc);
	}

	return 0;
}

static struct dlc_transport dlc_transport_coap = {
	.proto_supported = dlc_coap_proto_supported,
	.init = dlc_coap_init,
	.deinit = dlc_coap_deinit,
	.connect = dlc_coap_connect,
	.close = dlc_coap_close,
	.download = dlc_coap_download,
};

DLC_TRANSPORT(coap, &dlc_transport_coap);
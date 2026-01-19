/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
/* Define _POSIX_C_SOURCE before including <string.h> in order to use `strtok_r`. */
#define _POSIX_C_SOURCE 200809L

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/socket.h>
#include <net/downloader.h>
#include <net/downloader_transport.h>
#include <net/downloader_transport_coap.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include "dl_socket.h"
#include "dl_parse.h"

LOG_MODULE_DECLARE(downloader, CONFIG_DOWNLOADER_LOG_LEVEL);

#define DEFAULT_PORT_DTLS 5684
#define DEFAULT_PORT_UDP 5683

#define COAP_VER	     1
#define FILENAME_SIZE	     CONFIG_DOWNLOADER_MAX_FILENAME_SIZE
#define COAP_PATH_ELEM_DELIM "/"

#define COAP "coap://"
#define COAPS "coaps://"

struct transport_params_coap {
	/** Flag whether config is set */
	bool cfg_set;
	/** Configuration options */
	struct downloader_transport_coap_cfg cfg;
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
		struct net_sockaddr remote_addr;
	} sock;

	/** Request new data */
	bool new_data_req;
	/** Request retransmission */
	bool retransmission_req;
	/* Proxy-URI option value */
	const char *proxy_uri;
	/* Client auth callback */
	int (*auth_cb)(int sock);
};

BUILD_ASSERT(CONFIG_DOWNLOADER_TRANSPORT_PARAMS_SIZE >= sizeof(struct transport_params_coap));

static int coap_request_send(struct downloader *dl);

static int coap_get_current_from_response_pkt(const struct coap_packet *cpkt)
{
	int block = 0;

	block = coap_get_option_int(cpkt, COAP_OPTION_BLOCK2);
	if (block < 0) {
		return block;
	}

	return GET_BLOCK_NUM(block) << (GET_BLOCK_SIZE(block) + 4);
}

static bool has_pending(struct downloader *dl)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;
	return coap->pending.timeout > 0;
}

int coap_block_init(struct downloader *dl, size_t from)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	coap_block_transfer_init(&coap->block_ctx, coap->cfg.block_size, 0);
	coap->block_ctx.current = from;
	coap_pending_clear(&coap->pending);

	coap->initialized = true;
	return 0;
}

static int coap_get_recv_timeout(struct downloader *dl, uint32_t *timeout)
{
	int timeo;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	__ASSERT(has_pending(dl), "Must have coap pending");

	/* Retransmission is cycled in case recv() times out. In case sending request
	 * blocks, the time that is used for sending request must be substracted next time
	 * recv() is called.
	 */
	timeo = coap->pending.t0 + coap->pending.timeout - k_uptime_get_32();
	if (timeo < 0) {
		/* All time is spent when sending request and time this
		 * method is called, there is no time left for receiving;
		 * skip over recv() and initiate retransmission on next
		 * cycle
		 */
		return -ETIMEDOUT;
	}

	*timeout = timeo;
	return 0;
}

int coap_initiate_retransmission(struct downloader *dl)
{
	struct transport_params_coap *coap;
	int ret;

	coap = (struct transport_params_coap *)dl->transport_internal;

	if (coap->pending.timeout == 0) {
		return -EINVAL;
	}

	if (!coap_pending_cycle(&coap->pending)) {
		LOG_ERR("CoAP max-retransmissions exceeded");
		return -1;
	}

	ret = coap_request_send(dl);
	if (ret) {
		LOG_DBG("coap_request_send failed, err %d", ret);
		return -ECONNRESET;
	}

	return 0;
}

static int coap_block_update(struct downloader *dl, struct coap_packet *pkt, size_t *blk_off,
			     bool *more)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	int err, new_current;

	*blk_off = coap->block_ctx.current % coap_block_size_to_bytes(coap->block_ctx.block_size);
	if (*blk_off) {
		LOG_DBG("%d bytes of current block already downloaded", *blk_off);
	}

	new_current = coap_get_current_from_response_pkt(pkt);
	if (new_current < 0) {
		LOG_ERR("Failed to get current from CoAP packet, err %d", new_current);
		return new_current;
	}

	if (new_current < coap->block_ctx.current) {
		LOG_WRN("Block out of order %d, expected %d", new_current, coap->block_ctx.current);
		return -1;
	} else if (new_current > coap->block_ctx.current) {
		LOG_WRN("Block out of order %d, expected %d", new_current, coap->block_ctx.current);
		return -1;
	}

	err = coap_update_from_block(pkt, &coap->block_ctx);
	if (err) {
		return err;
	}

	if (dl->file_size == 0 && coap->block_ctx.total_size > 0) {
		LOG_DBG("Total size: %d", coap->block_ctx.total_size);
		dl->file_size = coap->block_ctx.total_size;
	}

	*more = coap_next_block(pkt, &coap->block_ctx);
	if (!*more) {
		LOG_DBG("Last block received");
	}

	return 0;
}

static int coap_parse(struct downloader *dl, size_t len)
{
	int err;
	size_t blk_off;
	uint8_t response_code;
	uint16_t payload_len;
	const uint8_t *payload;
	struct coap_packet response;
	bool more;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	/* TODO: currently we stop download on every error, but this is mostly not necessary
	 * and we can just request the same block again using retry mechanism
	 */

	err = coap_packet_parse(&response, dl->cfg.buf, len, NULL, 0);
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
	if (response_code != COAP_RESPONSE_CODE_OK && response_code != COAP_RESPONSE_CODE_CONTENT) {
		LOG_ERR("Server responded with code 0x%x", response_code);
		return -EBADMSG;
	}

	err = coap_block_update(dl, &response, &blk_off, &more);
	if (err) {
		return -EBADMSG;
	}

	payload = coap_packet_get_payload(&response, &payload_len);
	if (!payload) {
		LOG_WRN("No CoAP payload!");
		return -EBADMSG;
	}

	/* Accumulate buffer offset */
	dl->progress += payload_len;
	dl->buf_offset = 0;

	dl_transport_evt_data(dl, (void *)payload, payload_len);

	if (!more) {
		/* Mark the end, in case we did not know the total size */
		dl->file_size = dl->progress;
	}

	coap->new_data_req = true;
	return 0;
}

static int coap_request_send(struct downloader *dl)
{
	int err;
	uint16_t id;
	char file[FILENAME_SIZE];
	char *path_elem;
	char *path_elem_saveptr;
	struct coap_packet request;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	if (has_pending(dl)) {
		id = coap->pending.id;
	} else {
		id = coap_next_id();
	}

	err = coap_packet_init(&request, dl->cfg.buf, dl->cfg.buf_size, COAP_VER,
			       COAP_TYPE_CON, 8, coap_next_token(), COAP_METHOD_GET, id);
	if (err) {
		LOG_ERR("Failed to init CoAP message, err %d", err);
		return err;
	}

	LOG_DBG("dl->file: %s", dl->file);
	strncpy(file, dl->file, sizeof(file) - 1);

	path_elem = strtok_r(file, COAP_PATH_ELEM_DELIM, &path_elem_saveptr);
	do {
		err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH, path_elem,
						strlen(path_elem));
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

	if (coap->proxy_uri != NULL) {
		err = coap_packet_append_option(&request, COAP_OPTION_PROXY_URI,
			coap->proxy_uri, strlen(coap->proxy_uri));
		if (err) {
			LOG_ERR("Unable to add Proxy-URI option");
			return err;
		}
	}

	if (!has_pending(dl)) {
		struct coap_transmission_parameters params = coap_get_transmission_parameters();

		params.max_retransmission = coap->cfg.max_retransmission;
		err = coap_pending_init(&coap->pending, &request, &coap->sock.remote_addr, &params);
		if (err < 0) {
			return -EINVAL;
		}

		coap_pending_cycle(&coap->pending);
	}

	LOG_DBG("CoAP next block: %d", coap->block_ctx.current);

	err = dl_socket_send_timeout_set(coap->sock.fd, coap->pending.timeout);
	if (err) {
		return err;
	}

	err = dl_socket_send(coap->sock.fd, dl->cfg.buf, request.offset);
	if (err) {
		LOG_ERR("Failed to send CoAP request, errno %d", errno);
		return err;
	}

	if (IS_ENABLED(CONFIG_DOWNLOADER_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(request.data, request.offset, "CoAP request");
	}

	return 0;
}

static bool dl_coap_proto_supported(struct downloader *dl, const char *url)
{
	if (strncmp(url, COAPS, (sizeof(COAPS) - 1)) == 0) {
		return true;
	} else if (strncmp(url, COAP, (sizeof(COAP) - 1)) == 0) {
		return true;
	}

	return false;
}

static int dl_coap_init(struct downloader *dl, struct downloader_host_cfg *dl_host_cfg,
			const char *url)
{
	int err;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	/* Reset coap internal struct except config. */
	struct downloader_transport_coap_cfg tmp_cfg = coap->cfg;
	bool cfg_set = coap->cfg_set;

	memset(coap, 0, sizeof(struct transport_params_coap));

	if (cfg_set) {
		coap->cfg = tmp_cfg;
		coap->cfg_set = cfg_set;
	} else {
		coap->cfg.block_size = COAP_BLOCK_1024;
		coap->cfg.max_retransmission = 4;
	}

	coap->sock.proto = NET_IPPROTO_UDP;
	coap->sock.type = NET_SOCK_DGRAM;

	if (strncmp(url, COAPS, (sizeof(COAPS) - 1)) == 0) {
		coap->sock.proto = NET_IPPROTO_DTLS_1_2;
		coap->sock.type = NET_SOCK_DGRAM;

		if (dl_host_cfg->sec_tag_list == NULL || dl_host_cfg->sec_tag_count == 0) {
			LOG_WRN("No security tag provided for TLS/DTLS");
			return -EINVAL;
		}
	}

	err = dl_parse_url_port(url, &coap->sock.port);
	if (err) {
		switch (coap->sock.proto) {
		case NET_IPPROTO_DTLS_1_2:
			coap->sock.port = DEFAULT_PORT_DTLS;
			break;
		case NET_IPPROTO_UDP:
			coap->sock.port = DEFAULT_PORT_UDP;
			break;
		}
		LOG_DBG("Port not specified, using default: %d", coap->sock.port);
	}

	if (dl_host_cfg->set_native_tls) {
		LOG_DBG("Enabled native TLS");
		coap->sock.type |= SOCK_NATIVE_TLS;
	}

	/* Copy proxy-uri and auth-cb to internal struct */
	coap->proxy_uri = dl_host_cfg->proxy_uri;
	coap->auth_cb = dl_host_cfg->auth_cb;

	return 0;
}

static int dl_coap_deinit(struct downloader *dl)
{
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	if (coap->sock.fd != -1) {
		dl_socket_close(&coap->sock.fd);
	}

	return 0;
}

static int dl_coap_connect(struct downloader *dl)
{
	int err;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	err = -1;

	err = dl_socket_configure_and_connect(&coap->sock.fd, coap->sock.proto, coap->sock.type,
					      coap->sock.port, &coap->sock.remote_addr,
					      dl->hostname, &dl->host_cfg);
	if (err) {
		goto cleanup;
	}

	coap_block_init(dl, dl->progress);

cleanup:
	if (err) {
		/* Unable to connect, close socket */
		dl_socket_close(&coap->sock.fd);
		return err;
	}

	coap->new_data_req = true;

	/* Run auth callback if set */
	if (coap->auth_cb != NULL) {
		return coap->auth_cb(coap->sock.fd);
	}

	return err;
}

static int dl_coap_close(struct downloader *dl)
{
	int err;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	if (coap->sock.fd != -1) {
		err = dl_socket_close(&coap->sock.fd);
		return err;
	}

	return -EBADF;
}

static int dl_coap_download(struct downloader *dl)
{
	int ret, len, timeout;
	struct transport_params_coap *coap;

	coap = (struct transport_params_coap *)dl->transport_internal;

	if (coap->new_data_req) {
		/* Request next fragment */
		dl->buf_offset = 0;
		ret = coap_request_send(dl);
		if (ret) {
			LOG_DBG("data_req failed, err %d", ret);
			/** Attempt reconnection. */
			return ret;
		}

		coap->new_data_req = false;
	}

	if (coap->retransmission_req) {
		dl->buf_offset = 0;
		ret = coap_initiate_retransmission(dl);
		if (ret) {
			LOG_DBG("retransmission_req failed, err %d", ret);
			/** Attempt reconnection. */
			return -ECONNRESET;
		}

		coap->retransmission_req = false;
	}

	__ASSERT(dl->buf_offset < dl->cfg.buf_size, "Buffer overflow");

	LOG_DBG("Receiving up to %d bytes at %p...", (dl->cfg.buf_size - dl->buf_offset),
		(void *)(dl->cfg.buf + dl->buf_offset));

	ret = coap_get_recv_timeout(dl, &timeout);
	if (ret) {
		LOG_DBG("CoAP timeout");
		return -ETIMEDOUT;
	}

	ret = dl_socket_recv_timeout_set(coap->sock.fd, timeout);
	if (ret) {
		LOG_DBG("Failed to set CoAP recv timeout, err %d", ret);
		return ret;
	}

	len = dl_socket_recv(coap->sock.fd, dl->cfg.buf + dl->buf_offset,
			     dl->cfg.buf_size - dl->buf_offset);
	if (len < 0) {
		if ((len == -ETIMEDOUT) || (len == -EWOULDBLOCK) || (len == -EAGAIN)) {
			/* Request data again */
			coap->retransmission_req = true;
			LOG_DBG("CoAP recv failed with error: %d, retransmission requested", len);
			return 0;
		}

		return len;
	}

	ret = coap_parse(dl, len);
	if (ret < 0) {
		/* Request data again */
		coap->retransmission_req = true;
		return 0;
	}

	if (dl->progress == dl->file_size) {
		dl->complete = true;
	}

	return 0;
}

static const struct dl_transport dl_transport_coap = {
	.proto_supported = dl_coap_proto_supported,
	.init = dl_coap_init,
	.deinit = dl_coap_deinit,
	.connect = dl_coap_connect,
	.close = dl_coap_close,
	.download = dl_coap_download,
};

DL_TRANSPORT(coap, &dl_transport_coap);

int downloader_transport_coap_set_config(struct downloader *dl,
					 struct downloader_transport_coap_cfg *cfg)
{
	struct transport_params_coap *coap;

	if (!dl || !cfg) {
		return -EINVAL;
	}

	coap = (struct transport_params_coap *)dl->transport_internal;
	coap->cfg = *cfg;

	coap->cfg_set = true;

	return 0;
}

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <net/download_client.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_DECLARE(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define COAP_VER 1
#define FILENAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE
#define COAP_PATH_ELEM_DELIM "/"

/* declaration of strtok_r appears to be missing in some cases,
 * even though it's defined in the minimal libc, so we forward declare it
 */
extern char *strtok_r(char *str, const char *sep, char **state);

int url_parse_file(const char *url, char *file, size_t len);
int socket_send(const struct download_client *client, size_t len, int timeout);

static int coap_get_current_from_response_pkt(const struct coap_packet *cpkt)
{
	int block = 0;

	block = coap_get_option_int(cpkt, COAP_OPTION_BLOCK2);
	if (block < 0) {
		return block;
	}

	return GET_BLOCK_NUM(block) << (GET_BLOCK_SIZE(block) + 4);
}

static bool has_pending(struct download_client *client)
{
	return client->coap.pending.timeout > 0;
}

int coap_block_init(struct download_client *client, size_t from)
{
	coap_block_transfer_init(&client->coap.block_ctx,
				 CONFIG_DOWNLOAD_CLIENT_COAP_BLOCK_SIZE, 0);
	client->coap.block_ctx.current = from;
	return 0;
}

int coap_get_recv_timeout(struct download_client *dl)
{
	int timeout;

	__ASSERT(has_pending(dl), "Must have coap pending");

	/* Retransmission is cycled in case recv() times out. In case sending request
	 * blocks, the time that is used for sending request must be substracted next time
	 * recv() is called.
	 */
	timeout = dl->coap.pending.t0 + dl->coap.pending.timeout - k_uptime_get_32();
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

int coap_initiate_retransmission(struct download_client *dl)
{
	if (dl->coap.pending.timeout == 0) {
		return -EINVAL;
	}

	if (!coap_pending_cycle(&dl->coap.pending)) {
		LOG_ERR("CoAP max-retransmissions exceeded");
		return -1;
	}

	return 0;
}

static int coap_block_update(struct download_client *client, struct coap_packet *pkt,
			     size_t *blk_off, bool *more)
{
	int err, new_current;

	*blk_off = client->coap.block_ctx.current %
		   coap_block_size_to_bytes(client->coap.block_ctx.block_size);
	if (*blk_off) {
		LOG_DBG("%d bytes of current block already downloaded",
			*blk_off);
	}

	new_current = coap_get_current_from_response_pkt(pkt);
	if (new_current < 0) {
		LOG_ERR("Failed to get current from CoAP packet, err %d", new_current);
		return new_current;
	}

	if (new_current < client->coap.block_ctx.current) {
		LOG_WRN("Block out of order %d, expected %d", new_current,
			client->coap.block_ctx.current);
		return -1;
	} else if (new_current > client->coap.block_ctx.current) {
		LOG_WRN("Block out of order %d, expected %d", new_current,
			client->coap.block_ctx.current);
		return -1;
	}

	err = coap_update_from_block(pkt, &client->coap.block_ctx);
	if (err) {
		return err;
	}

	if (client->file_size == 0 && client->coap.block_ctx.total_size > 0) {
		LOG_DBG("Total size: %d", client->coap.block_ctx.total_size);
		client->file_size = client->coap.block_ctx.total_size;
	}

	*more = coap_next_block(pkt, &client->coap.block_ctx);
	if (!*more) {
		LOG_DBG("Last block received");
	}

	return 0;
}

int coap_parse(struct download_client *client, size_t len)
{
	int err;
	size_t blk_off;
	uint8_t response_code;
	uint16_t payload_len;
	const uint8_t *payload;
	struct coap_packet response;
	bool more;

	/* TODO: currently we stop download on every error, but this is mostly not necessary
	 * and we can just request the same block again using retry mechanism
	 */

	err = coap_packet_parse(&response, client->buf, len, NULL, 0);
	if (err) {
		LOG_ERR("Failed to parse CoAP packet, err %d", err);
		return -1;
	}

	err = coap_block_update(client, &response, &blk_off, &more);
	if (err) {
		return err;
	}

	if (coap_header_get_id(&response) != client->coap.pending.id) {
		LOG_ERR("Response is not pending");
		return -1;
	}

	coap_pending_clear(&client->coap.pending);

	if (coap_header_get_type(&response) != COAP_TYPE_ACK) {
		LOG_ERR("Response must be of coap type ACK");
		return -1;
	}

	response_code = coap_header_get_code(&response);
	if (response_code != COAP_RESPONSE_CODE_OK &&
	    response_code != COAP_RESPONSE_CODE_CONTENT) {
		LOG_ERR("Server responded with code 0x%x", response_code);
		return -1;
	}

	payload = coap_packet_get_payload(&response, &payload_len);
	if (!payload) {
		LOG_WRN("No CoAP payload!");
		return -1;
	}

	/* TODO: because our buffer is large enough for the whole datagram,
	 * we don't scrictly need to copy the bytes at the beginning
	 * of the buffer, we could simply send a fragment pointing to the
	 * payload directly.
	 */
	LOG_DBG("CoAP response: %d, copying %d bytes",
		coap_header_get_code(&response), payload_len - blk_off);
	memcpy(client->buf + client->offset, payload + blk_off,
	       payload_len - blk_off);

	client->offset += payload_len - blk_off;
	client->progress += payload_len - blk_off;

	if (!more) {
		/* Mark the end, in case we did not know the total size */
		client->file_size = client->progress;
	}

	return 0;
}

int coap_request_send(struct download_client *client)
{
	int err;
	uint16_t id;
	char file[FILENAME_SIZE];
	char *path_elem;
	char *path_elem_saveptr;
	struct coap_packet request;

	if (has_pending(client)) {
		id = client->coap.pending.id;
	} else {
		id = coap_next_id();
	}

	err = coap_packet_init(&request, client->buf, CONFIG_DOWNLOAD_CLIENT_BUF_SIZE, COAP_VER,
			       COAP_TYPE_CON, 8, coap_next_token(), COAP_METHOD_GET, id);
	if (err) {
		LOG_ERR("Failed to init CoAP message, err %d", err);
		return err;
	}

	err = url_parse_file(client->file, file, sizeof(file));
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

	err = coap_append_block2_option(&request, &client->coap.block_ctx);
	if (err) {
		LOG_ERR("Unable to add block2 option");
		return err;
	}

	err = coap_append_size2_option(&request, &client->coap.block_ctx);
	if (err) {
		LOG_ERR("Unable to add size2 option");
		return err;
	}

	if (!has_pending(client)) {
		err = coap_pending_init(&client->coap.pending, &request, &client->remote_addr,
					CONFIG_DOWNLOAD_CLIENT_COAP_MAX_RETRANSMIT_REQUEST_COUNT);
		if (err < 0) {
			return -EINVAL;
		}

		coap_pending_cycle(&client->coap.pending);
	}

	LOG_DBG("CoAP next block: %d", client->coap.block_ctx.current);

	err = socket_send(client, request.offset, client->coap.pending.timeout);
	if (err) {
		LOG_ERR("Failed to send CoAP request, errno %d", errno);
		return err;
	}

	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(request.data, request.offset, "CoAP request");
	}

	return 0;
}

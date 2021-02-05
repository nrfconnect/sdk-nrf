/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/coap.h>
#include <net/download_client.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define COAP_VER 1
#define FILENAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE

int url_parse_file(const char *url, char *file, size_t len);
int socket_send(const struct download_client *client, size_t len);

int coap_block_init(struct download_client *client, size_t from)
{
	coap_block_transfer_init(&client->coap.block_ctx,
				 CONFIG_DOWNLOAD_CLIENT_COAP_BLOCK_SIZE, 0);
	client->coap.block_ctx.current = from;
	return 0;
}

int coap_block_update(struct download_client *client, struct coap_packet *pkt,
		      size_t *blk_off)
{
	int err;
	bool more;

	*blk_off = client->coap.block_ctx.current %
		   coap_block_size_to_bytes(client->coap.block_ctx.block_size);
	if (*blk_off) {
		LOG_DBG("%d bytes of current block already downloaded",
			*blk_off);
	}

	err = coap_update_from_block(pkt, &client->coap.block_ctx);
	if (err) {
		return err;
	}

	if (client->file_size == 0) {
		LOG_DBG("Total size: %d", client->coap.block_ctx.total_size);
		client->file_size = client->coap.block_ctx.total_size;
	}

	more = coap_next_block(pkt, &client->coap.block_ctx);
	if (!more) {
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

	err = coap_packet_parse(&response, client->buf, len, NULL, 0);
	if (err) {
		LOG_ERR("Failed to parse CoAP packet, err %d", err);
		return -1;
	}

	err = coap_block_update(client, &response, &blk_off);
	if (err) {
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

	return 0;
}

int coap_request_send(struct download_client *client)
{
	int err;
	char file[FILENAME_SIZE];
	struct coap_packet request;

	err = coap_packet_init(
		&request, client->buf, CONFIG_DOWNLOAD_CLIENT_BUF_SIZE,
		COAP_VER, COAP_TYPE_CON, 8, coap_next_token(),
		COAP_METHOD_GET, coap_next_id()
	);
	if (err) {
		LOG_ERR("Failed to init CoAP message, err %d", err);
		return err;
	}

	err = url_parse_file(client->file, file, sizeof(file));
	if (err) {
		return err;
	}

	err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					file, strlen(file));
	if (err) {
		LOG_ERR("Unable add option to request");
		return err;
	}

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

	LOG_DBG("CoAP next block: %d", client->coap.block_ctx.current);

	err = socket_send(client, request.offset);
	if (err) {
		LOG_ERR("Failed to send CoAP request, errno %d", errno);
		return err;
	}

	if (IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS)) {
		LOG_HEXDUMP_DBG(request.data, request.offset, "CoAP request");
	}

	return 0;
}

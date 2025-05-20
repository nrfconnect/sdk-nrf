/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include "parser.h"
#include "sms_deliver.h"

LOG_MODULE_DECLARE(sms, CONFIG_SMS_LOG_LEVEL);

int parser_create(struct parser *parser, struct parser_api *api)
{
	memset(parser, 0, sizeof(struct parser));

	parser->api = api;

	parser->data = k_calloc(1, api->data_size());
	if (parser->data == NULL) {
		LOG_ERR("Out of memory when creating parser");
		return -ENOMEM;
	}

	return 0;
}

int parser_delete(struct parser *parser)
{
	k_free(parser->data);
	parser->data = NULL;

	return 0;
}

/**
 * @brief Process given data with all sub parser of the given parser.
 *
 * @param[in] parser Parser instance which has information on sub parsers that are executed.
 *
 * @return Zero if successful, negative value in error cases.
 */
static int parser_process(struct parser *parser)
{
	int ofs_inc;

	parser_module *parsers = parser->api->get_parsers();

	parser->buf_pos = 0;

	for (int i = 0; i < parser->api->get_parser_count(); i++) {
		ofs_inc = parsers[i](parser, &parser->buf[parser->buf_pos]);

		if (ofs_inc < 0) {
			return ofs_inc;
		}

		parser->buf_pos += ofs_inc;

		/* If we have gone beyond the length of the given data, we need to return a failure.
		 * We don't have issues in accessing memory beyond parser->buf_size bytes of
		 * parser->buf as the buffer is overly long.
		 */
		if (parser->buf_pos > parser->buf_size) {
			return -EMSGSIZE;
		}
	}

	parser->payload_pos = parser->buf_pos;

	return 0;
}

int parser_process_str(struct parser *parser, const char *data)
{
	uint16_t length = strlen(data);
	uint16_t data_buf_size = length / 2;
	size_t bin_buf_len;

	if (length % 2 != 0) {
		LOG_ERR("Data length (%d) is not divisible by two: %s",
			length, data);
		return -EMSGSIZE;
	}

	if (data_buf_size > PARSER_BUF_SIZE) {
		LOG_ERR("Data length (%d) is bigger than the internal buffer size (%d)",
			data_buf_size,
			PARSER_BUF_SIZE);
		return -EMSGSIZE;
	}

	parser->buf_size = data_buf_size;
	bin_buf_len = hex2bin(data, length, parser->buf, PARSER_BUF_SIZE);
	if (bin_buf_len == 0) {
		LOG_ERR("Invalid hex character found in buffer: %s", data);
		return -EINVAL;
	}

	return parser_process(parser);
}

int parser_get_payload(struct parser *parser, char *buf, uint8_t buf_size)
{
	parser_module payload_decoder = parser->api->get_decoder();

	parser->payload = buf;
	parser->payload_buf_size = buf_size;
	return payload_decoder(parser, &parser->buf[parser->payload_pos]);
}

int parser_get_header(struct parser *parser, void *header)
{
	return parser->api->get_header(parser, header);
}

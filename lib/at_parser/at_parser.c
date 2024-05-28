/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <modem/at_parser.h>

#include "at_token.h"
#include "at_match.h"

/* Carriage Return. */
#define CR '\r'
/* Line Feed. */
#define LF '\n'
/* Null Terminator. */
#define NULL_TERMINATOR '\0'
/* Minus Sign. */
#define MINUS_SIGN '-'
/* Init Sentinel. */
#define INIT_SENTINEL 0xc0ffee

enum at_num_type {
	AT_NUM_TYPE_INT16,
	AT_NUM_TYPE_UINT16,
	AT_NUM_TYPE_INT32,
	AT_NUM_TYPE_UINT32,
	AT_NUM_TYPE_INT64,
	AT_NUM_TYPE_UINT64,
};

/* Trim expected CR, LF, or CRLF. */
static void trim_crlf(const char **str)
{
	while ((*str)[0] == CR || (*str)[0] == LF) {
		(*str)++;
	}
}

static bool is_subparam(const struct at_token *token)
{
	switch (token->type) {
	case AT_TOKEN_TYPE_INT:
	case AT_TOKEN_TYPE_QUOTED_STRING:
	case AT_TOKEN_TYPE_ARRAY:
	case AT_TOKEN_TYPE_EMPTY:
		return true;
	default:
		return false;
	}
}

/* Check if the remainder of the string contains a response. */
static bool is_resp(const char *str)
{
	/* All possible response variants. */
	static const char * const resp[] = {
		"OK\r\n",
		"ERROR\r\n",
		"+CME ERROR:",
		"+CMS ERROR:"
	};

	trim_crlf(&str);

	for (size_t i = 0; i < ARRAY_SIZE(resp); i++) {
		if (strncmp(str, resp[i], strlen(resp[i])) == 0) {
			return true;
		}
	}

	return false;
}

static bool is_index_ahead(struct at_parser *parser, size_t index)
{
	return index > parser->count - 1;
}

/* Lookahead if the string matches either CR or LF. */
static bool lookahead_cr_lf(const char *str)
{
	return  (str[0] == CR || str[0] == LF) && str[1] == NULL_TERMINATOR;
}

/* Lookahead if the string starts with either CR or LF and has more data. */
static bool lookahead_cr_lf_and_more(const char *str)
{
	return  (str[0] == CR || str[0] == LF) && str[1] != NULL_TERMINATOR;
}

/* Lookahead if the string matches either CR or LF, or starts with either CR or LF and has more
 * data.
 */
static bool lookahead_cr_lf_or_more(const char *str)
{
	return lookahead_cr_lf(str) || lookahead_cr_lf_and_more(str);
}

/* Lookahead if the string matches CRLF. */
static bool lookahead_crlf(const char *str)
{
	return str[0] == CR && str[1] == LF && str[2] == NULL_TERMINATOR;
}

/* Lookahead if the string starts with CRLF and has more data. */
static bool lookahead_crlf_and_more(const char *str)
{
	return str[0] == CR && str[1] == LF && str[2] != NULL_TERMINATOR;
}

/* Lookahead if the string matches CRLF or starts with CRLF and has more data. */
static bool lookahead_crlf_or_more(const char *str)
{
	return lookahead_crlf(str) || lookahead_crlf_and_more(str);
}

static void at_token_set_empty(struct at_token *token, const char *start)
{
	token->start = start;
	token->len = 0;
	token->type = AT_TOKEN_TYPE_EMPTY;
	token->var = AT_TOKEN_VAR_NO_COMMA;
}

static int at_parser_check(struct at_parser *parser)
{
	if (!parser) {
		return -EINVAL;
	}

	if (!parser->at || !parser->cursor || parser->init_sentinel != INIT_SENTINEL) {
		return -EPERM;
	}

	return 0;
}

/* Retrieve one token from the AT parser.  */
static int at_parser_tok(struct at_parser *parser, struct at_token *token)
{
	const char *remainder = NULL;

	/* The lexer cannot match empty strings, so intercept the special case where the empty
	 * subparameter is the one after the previously parsed token.
	 * This case is detected in the previous call to this function.
	 */
	if (parser->is_next_empty) {
		at_token_set_empty(token, parser->cursor);

		/* Set the remainder to the current cursor because an empty token has length
		 * zero.
		 */
		remainder = parser->cursor;

		/* Reset the flag. */
		parser->is_next_empty = false;

		goto finalize;
	}

	/* Trim leading CRLF. */
	if (parser->count == 0 && lookahead_crlf_and_more(parser->cursor)) {
		parser->cursor += 2;
	}

	/* Nothing left to tokenize. */
	if (parser->cursor[0] == NULL_TERMINATOR ||
	    lookahead_cr_lf(parser->cursor) ||
	    lookahead_crlf(parser->cursor)) {
		return -EIO;
	}

	/* Don't tokenize response. */
	if (is_resp(parser->cursor)) {
		return -EIO;
	}

	/* Stop if there is a new line to parse. */
	if (lookahead_cr_lf_and_more(parser->cursor) ||
	    lookahead_crlf_and_more(parser->cursor)) {
		return -EAGAIN;
	}

	if (parser->count == 0) {
		*token = at_match_cmd(parser->cursor, &remainder);
	} else {
		*token = at_match_subparam(parser->cursor, &remainder);
	}

	if (token->type == AT_TOKEN_TYPE_INVALID) {
		/* If the token is neither a valid command nor a valid subparameter, check if it's a
		 * non-quoted string.
		 */
		*token = at_match_str(parser->cursor, &remainder);
		if (token->type == AT_TOKEN_TYPE_INVALID) {
			return -EBADMSG;
		}
	}

	if (is_subparam(token)) {
		bool next_is_NULL_CR_LF_or_CRLF = (remainder[0] == NULL_TERMINATOR) ||
						  lookahead_cr_lf_or_more(remainder) ||
						  lookahead_crlf_or_more(remainder);

		if (token->var == AT_TOKEN_VAR_COMMA && next_is_NULL_CR_LF_or_CRLF) {
			/* The next token is of type empty if and only if the current token has a
			 * trailing comma and the remaining string either is empty or starts with
			 * CR, LF, or CRLF.
			 */
			parser->is_next_empty = true;
		} else if (token->var == AT_TOKEN_VAR_NO_COMMA && !next_is_NULL_CR_LF_or_CRLF) {
			/* If the token is a subparameter but does not have a trailing comma then it
			 * is the last subparameter of the line.
			 * If there is more to parse, then the token must be followed by CR, LF, or
			 * CRLF otherwise the string is malformed.
			 */
			return -EBADMSG;
		}
	}

finalize:
	parser->count++;
	parser->cursor = remainder;

	return 0;
}

/* Seek the AT parser cursor to the given index. */
static int at_parser_seek(struct at_parser *parser, size_t index, struct at_token *token)
{
	int err;

	if (!is_index_ahead(parser, index)) {
		/* Rewind parser. */
		parser->cursor = parser->at;
		parser->count = 0;
	}

	do {
		err = at_parser_tok(parser, token);
	} while (is_index_ahead(parser, index) && !err);

	return err;
}

int at_parser_init(struct at_parser *parser, const char *at)
{
	if (!parser || !at) {
		return -EINVAL;
	}

	memset(parser, 0, sizeof(struct at_parser));

	parser->at = at;
	parser->cursor = at;
	parser->init_sentinel = INIT_SENTINEL;

	return 0;
}

int at_parser_cmd_next(struct at_parser *parser)
{
	int err;
	struct at_token token = {0};

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	do {
		err = at_parser_tok(parser, &token);
	} while (!err);

	if (err != -EAGAIN) {
		return -EOPNOTSUPP;
	}

	trim_crlf(&parser->cursor);

	/* Reset count. */
	parser->count = 0;
	/* Set pointer of current AT command string to the current cursor, which points to the
	 * beginning of a new AT command line.
	 */
	parser->at = parser->cursor;

	return 0;
}

int at_parser_cmd_type_get(struct at_parser *parser, enum at_parser_cmd_type *type)
{
	int err;
	struct at_token token;

	if (!type) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	token = at_match_cmd(parser->at, NULL);
	switch (token.type) {
	case AT_TOKEN_TYPE_CMD_TEST:
		*type = AT_PARSER_CMD_TYPE_TEST;
		break;
	case AT_TOKEN_TYPE_CMD_READ:
		*type = AT_PARSER_CMD_TYPE_READ;
		break;
	case AT_TOKEN_TYPE_CMD_SET:
		*type = AT_PARSER_CMD_TYPE_SET;
		break;
	default:
		*type = AT_PARSER_CMD_TYPE_UNKNOWN;
		return -EOPNOTSUPP;
	}

	return 0;
}

int at_parser_cmd_count_get(struct at_parser *parser, size_t *count)
{
	int err;
	struct at_token token = {0};

	if (!count) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	do {
		err = at_parser_tok(parser, &token);
	} while (!err);

	*count = parser->count;

	return (err == -EIO || err == -EAGAIN) ? 0 : err;
}

static int at_parser_num_get_impl(struct at_parser *parser, size_t index, void *value,
				  enum at_num_type type)
{
	int err;
	struct at_token token = {0};

	if (!value) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	err = at_parser_seek(parser, index, &token);
	if (err) {
		return err;
	}

	if (token.type != AT_TOKEN_TYPE_INT) {
		return -EOPNOTSUPP;
	}

	/* Must be set to 0 before calling `strtoull` or `strtoll`. */
	errno = 0;

	/* Check unsigned 64-bit integer first, using its own parsing function. */
	if (type == AT_NUM_TYPE_UINT64) {
		if (token.start[0] == MINUS_SIGN) {
			return -ERANGE;
		}

		uint64_t val = strtoull(token.start, NULL, 10);

		if (errno == ERANGE) {
			return -ERANGE;
		}

		*(uint64_t *)value = val;

		return 0;
	}

	int64_t val = strtoll(token.start, NULL, 10);

	switch (type) {
	case AT_NUM_TYPE_INT16:
		if (val < INT16_MIN || val > INT16_MAX) {
			return -ERANGE;
		}
		*(int16_t *)value = (int16_t)val;
		break;
	case AT_NUM_TYPE_UINT16:
		if (val < 0 || val > UINT16_MAX) {
			return -ERANGE;
		}
		*(uint16_t *)value = (uint16_t)val;
		break;
	case AT_NUM_TYPE_INT32:
		if (val < INT32_MIN || val > INT32_MAX) {
			return -ERANGE;
		}
		*(int32_t *)value = (int32_t)val;
		break;
	case AT_NUM_TYPE_UINT32:
		if (val < 0 || val > UINT32_MAX) {
			return -ERANGE;
		}
		*(uint32_t *)value = (uint32_t)val;
		break;
	case AT_NUM_TYPE_INT64:
		if (errno == ERANGE) {
			return -ERANGE;
		}
		*(int64_t *)value = val;
		break;
	default:
	}

	return 0;
}

int at_parser_int16_get(struct at_parser *parser, size_t index, int16_t *value)
{
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_INT16);
}

int at_parser_uint16_get(struct at_parser *parser, size_t index, uint16_t *value)
{
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_UINT16);
}

int at_parser_int32_get(struct at_parser *parser, size_t index, int32_t *value)
{
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_INT32);
}

int at_parser_uint32_get(struct at_parser *parser, size_t index, uint32_t *value)
{
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_UINT32);
}

int at_parser_int64_get(struct at_parser *parser, size_t index, int64_t *value)
{
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_INT64);
}

int at_parser_uint64_get(struct at_parser *parser, size_t index, uint64_t *value)
{
	return at_parser_num_get_impl(parser, index, value, AT_NUM_TYPE_UINT64);
}

static int at_parser_string_common_get_impl(struct at_parser *parser, size_t index, void *ptr,
					    size_t *len, bool is_ptr_get)
{
	int err;
	struct at_token token = {0};

	if (!ptr || !len) {
		return -EINVAL;
	}

	err = at_parser_check(parser);
	if (err) {
		return err;
	}

	err = at_parser_seek(parser, index, &token);
	if (err) {
		return err;
	}

	switch (token.type) {
	/* Acceptable types. */
	case AT_TOKEN_TYPE_CMD_TEST:
	case AT_TOKEN_TYPE_CMD_SET:
	case AT_TOKEN_TYPE_CMD_READ:
	case AT_TOKEN_TYPE_NOTIF:
	case AT_TOKEN_TYPE_QUOTED_STRING:
	case AT_TOKEN_TYPE_STRING:
	case AT_TOKEN_TYPE_ARRAY:
		break;
	default:
		return -EOPNOTSUPP;
	}

	if (is_ptr_get) {
		*((const char **)ptr) = token.start;
		*len = token.len;
	} else {
		/* Check if there is enough memory. */
		if (*len < token.len + 1) {
			return -ENOMEM;
		}

		memcpy((char *)ptr, token.start, token.len);

		/* Null-terminate the string. */
		((char *)ptr)[token.len] = '\0';

		/* Update the length to reflect the copied string length. */
		*len = token.len;
	}

	return 0;
}

int at_parser_string_get(struct at_parser *parser, size_t index, char *str, size_t *len)
{
	return at_parser_string_common_get_impl(parser, index, (void *)str, len, false);
}

int at_parser_string_ptr_get(struct at_parser *parser, size_t index, const char **str_ptr,
			     size_t *len)
{
	return at_parser_string_common_get_impl(parser, index, (void *)str_ptr, len, true);
}

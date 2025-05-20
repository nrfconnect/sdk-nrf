/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_TOKEN_H__
#define AT_TOKEN_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file at_token.h
 *
 * @defgroup at_token Represent an AT command prefix or a subparameter value.
 * @ingroup at_parser
 * @{
 * @brief An AT token represents an AT command prefix or a subparameter value.
 */

/** @brief AT token types that can be matched from an AT command string. */
enum at_token_type {
	/** Invalid token, typically a subparameter that does not exist. */
	AT_TOKEN_TYPE_INVALID,
	/** Token of type test command. */
	AT_TOKEN_TYPE_CMD_TEST,
	/** Token of type read command. */
	AT_TOKEN_TYPE_CMD_READ,
	/** Token of type set command. */
	AT_TOKEN_TYPE_CMD_SET,
	/** Token of type notification. */
	AT_TOKEN_TYPE_NOTIF,
	/** Token of type integer. */
	AT_TOKEN_TYPE_INT,
	/** Token of type quoted string. */
	AT_TOKEN_TYPE_QUOTED_STRING,
	/** Token of type array. */
	AT_TOKEN_TYPE_ARRAY,
	/** Token of type empty. */
	AT_TOKEN_TYPE_EMPTY,
	/** Token of type non-quoted string. */
	AT_TOKEN_TYPE_STRING,
};

/** @brief AT token variants based on the presence or absence of a comma after the subparameter. */
enum at_token_var {
	AT_TOKEN_VAR_NO_COMMA,
	AT_TOKEN_VAR_COMMA
};

/** @brief An AT token is defined with a start pointer, a length, a type, and a variant. */
struct at_token {
	const char *start;
	size_t len;
	enum at_token_type type;
	enum at_token_var var;
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_TOKEN_H__ */

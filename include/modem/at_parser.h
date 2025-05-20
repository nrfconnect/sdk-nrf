/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_PARSER_H__
#define AT_PARSER_H__

#include <stdbool.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file at_parser.h
 *
 * @defgroup at_parser AT parser
 * @{
 * @brief Parser for AT commands and notifications.
 */

/** @brief Identifies the type of a given AT command prefix. */
enum at_parser_cmd_type {
	/** Unknown command, indicates that the actual command type could not
	 *  be resolved.
	 */
	AT_PARSER_CMD_TYPE_UNKNOWN,
	/** AT set command. */
	AT_PARSER_CMD_TYPE_SET,
	/** AT read command. */
	AT_PARSER_CMD_TYPE_READ,
	/** AT test command. */
	AT_PARSER_CMD_TYPE_TEST
};

/**
 * @brief AT parser
 *
 * Holds the parsing state for a configured AT command string.
 *
 */
struct at_parser {
	/* Pointer to the current AT command line. */
	const char *at;
	/* Pointer to the current AT command line value. */
	const char *cursor;
	/* Number of values parsed so far for the current AT command line. */
	size_t count;
	/* Indicates that the next subparameter is empty. */
	bool is_next_empty;
	/* Sentinel value for determining initialization state. */
	uint32_t init_sentinel;
};

/**
 * @brief Type-generic macro for getting an integer value.
 *
 * @param[in]  parser AT parser.
 * @param[in]  index  Subparameter index in the current AT command line configured in @p parser.
 * @param[out] value  Subparameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the type of value at the given index.
 * @retval -ERANGE     Parsed integer value is out of range for the expected type.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
#define at_parser_num_get(parser, index, value)    \
	_Generic((value),                          \
		int16_t * : at_parser_int16_get,   \
		uint16_t * : at_parser_uint16_get, \
		int32_t * : at_parser_int32_get,   \
		uint32_t * : at_parser_uint32_get, \
		int64_t * : at_parser_int64_get,   \
		uint64_t * : at_parser_uint64_get  \
	)(parser, index, value)

/**
 * @brief Initialize an AT parser for a given AT command string.
 *
 * @param[in] parser A pointer to the AT parser.
 * @param[in] at     A pointer to the AT command string to parse.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL One or more of the supplied parameters are invalid.
 */
int at_parser_init(struct at_parser *parser, const char *at);

/**
 * @brief Move the cursor of an AT parser to the next command line of its configured AT command
 *        string.
 *
 * It is possible to call this function successfully with a given AT parser if and only if its
 * configured AT command string has multiple lines and its current line is not the last
 * one.
 *
 * @param[in] parser A pointer to the AT parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP The cursor cannot move to the next line. This error occurs if there is no
 *                     next line to move to or the configured AT command string is malformed.
 */
int at_parser_cmd_next(struct at_parser *parser);

/**
 * @brief Get the type of the command prefix in the current AT command line.
 *
 * @param[in]  parser A pointer to the AT parser.
 * @param[out] type   A pointer to the AT command prefix type.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the given AT command line.
 */
int at_parser_cmd_type_get(struct at_parser *parser, enum at_parser_cmd_type *type);

/**
 * @brief Get the number of values that exist in the current AT command line.
 *
 * @param[in]  parser A pointer to the AT parser.
 * @param[out] count  A pointer to the number of parsed values.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EBADMSG    The AT command string is malformed.
 */
int at_parser_cmd_count_get(struct at_parser *parser, size_t *count);

/**
 * @brief Get a signed 16-bit integer value.
 *
 * @param[in]  parser AT parser.
 * @param[in]  index  Subparameter index in the current AT command line configured in @p parser.
 * @param[out] value  Subparameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the type of value at the given index.
 * @retval -ERANGE     Parsed integer value is out of range for the expected type.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
int at_parser_int16_get(struct at_parser *parser, size_t index, int16_t *value);

/**
 * @brief Get an unsigned 16-bit integer value.
 *
 * @param[in]  parser AT parser.
 * @param[in]  index  Subparameter index in the current AT command line configured in @p parser.
 * @param[out] value  Subparameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the type of value at the given index.
 * @retval -ERANGE     Parsed integer value is out of range for the expected type.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
int at_parser_uint16_get(struct at_parser *parser, size_t index, uint16_t *value);

/**
 * @brief Get a signed 32-bit integer value.
 *
 * @param[in]  parser AT parser.
 * @param[in]  index  Subparameter index in the current AT command line configured in @p parser.
 * @param[out] value  Subparameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the type of value at the given index.
 * @retval -ERANGE     Parsed integer value is out of range for the expected type.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
int at_parser_int32_get(struct at_parser *parser, size_t index, int32_t *value);

/**
 * @brief Get an unsigned 32-bit integer value.
 *
 * @param[in]  parser AT parser.
 * @param[in]  index  Subparameter index in the current AT command line configured in @p parser.
 * @param[out] value  Subparameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the type of value at the given index.
 * @retval -ERANGE     Parsed integer value is out of range for the expected type.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
int at_parser_uint32_get(struct at_parser *parser, size_t index, uint32_t *value);

/**
 * @brief Get a signed 64-bit integer value.
 *
 * @param[in]  parser AT parser.
 * @param[in]  index  Subparameter index in the current AT command line configured in @p parser.
 * @param[out] value  Subparameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the type of value at the given index.
 * @retval -ERANGE     Parsed integer value is out of range for the expected type.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
int at_parser_int64_get(struct at_parser *parser, size_t index, int64_t *value);

/**
 * @brief Get an unsigned 64-bit integer value.
 *
 * @param[in]  parser AT parser.
 * @param[in]  index  Subparameter index in the current AT command line configured in @p parser.
 * @param[out] value  Subparameter value.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the type of value at the given index.
 * @retval -ERANGE     Parsed integer value is out of range for the expected type.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
int at_parser_uint64_get(struct at_parser *parser, size_t index, uint64_t *value);

/**
 * @brief Get a string value.
 *
 * The data type must be a string (quoted or non-quoted), an AT command prefix, or an array,
 * otherwise an error is returned.
 * The string value is copied to the buffer and null-terminated.
 * @p len must be at least string length plus one, or an error is returned.
 *
 * @param[in]     parser  AT parser.
 * @param[in]     index   Index in the current AT command line configured in @p parser.
 * @param[in]     str     Pointer to the buffer where to copy the value.
 * @param[in,out] len     Available space in @p str, returns the length of the copied string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the type of value at the given index.
 * @retval -ENOMEM     @p str is smaller than the null-terminated string to be copied.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
int at_parser_string_get(struct at_parser *parser, size_t index, char *str, size_t *len);

/**
 * @brief Get a pointer to a string value.
 *
 * The data type must be a string (quoted or non-quoted), an AT command prefix, or an array,
 * otherwise an error is returned.
 *
 * @param[in]  parser  AT parser.
 * @param[in]  index   Index in the current AT command line configured in @p parser.
 * @param[out] str_ptr Pointer to the address of the string.
 * @param[out] len     Length of the string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EINVAL     One or more of the supplied parameters are invalid.
 * @retval -EPERM      @p parser has not been initialized.
 * @retval -EOPNOTSUPP Operation not supported for the subparameter type at the given index.
 * @retval -EBADMSG    The AT command string is malformed.
 * @retval -EAGAIN     Parsing of the current AT command line is terminated and a subsequent line is
 *                     available. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 * @retval -EIO        There is nothing more to parse in the AT command string configured in
 *                     @p parser. Returned when @p index is greater than the maximum index for the
 *                     current AT command line.
 */
int at_parser_string_ptr_get(struct at_parser *parser, size_t index, const char **str_ptr,
			     size_t *len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_PARSER_H__ */

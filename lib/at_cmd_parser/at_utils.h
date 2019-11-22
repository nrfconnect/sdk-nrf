/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file at_utils.h
 *
 * @defgroup at_cmd_parser_utils AT Command utility functions.
 * @ingroup at_cmd_parser
 * @{
 * @brief AT parser utility functions to deal with strings.
 */
#ifndef AT_UTILS_H__
#define AT_UTILS_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <ctype.h>

#define AT_PARAM_SEPARATOR ','
#define AT_RSP_SEPARATOR ':'
#define AT_CMD_SEPARATOR '='
#define AT_CMD_BUFFER_TERMINATOR 0
#define AT_CMD_STRING_IDENTIFIER '\"'
#define AT_STANDARD_NOTIFICATION_PREFIX '+'
#define AT_PROP_NOTIFICATION_PREFX '%'
#define AT_CUSTOM_COMMAND_PREFX '#'

/**
 * @brief Check if character is a notification start character
 *
 * This function will check if the character is a + or % which identifies
 * a notification from the AT host.
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If character is + or %
 * @retval false If character is something else
 */
static inline bool is_notification(char chr)
{
	if ((chr == AT_STANDARD_NOTIFICATION_PREFIX) ||
	    (chr == AT_PROP_NOTIFICATION_PREFX)) {
		return true;
	}

	return false;
}

/**
 * @brief Check if a string is a beginning of an AT command
 *
 * This function will check if the character is a "AT+" or "AT%" which
 * identifies an AT command.
 *
 * @param[in] str String to examine
 *
 * @retval true  If the string is an AT command
 * @retval false Otherwise
 */
static inline bool is_command(const char *str)
{
	if (strlen(str) < 3) {
		return false;
	}

	if ((toupper(str[0]) == 'A') && (toupper(str[1]) == 'T') &&
	    ((str[2] == AT_STANDARD_NOTIFICATION_PREFIX) ||
	     (str[2] == AT_PROP_NOTIFICATION_PREFX) ||
	     (str[2] == AT_CUSTOM_COMMAND_PREFX))) {
		return true;
	}

	return false;
}

/**
 * @brief Verify that the character is a valid character
 *
 * Notification ID strings can only contain upper case letters 'A' through 'Z'
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If character is valid
 * @retval false If character is not valid
 */
static inline bool is_valid_notification_char(char chr)
{
	chr = toupper(chr);

	if ((chr >= 'A') && (chr <= 'Z')) {
		return true;
	}

	return false;
}

/**
 * @brief Check if the character identifies the end of the buffer
 *
 * All strings returned from the AT Host is null terminated, this function will
 * check if we have hit the termination character.
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If character is 0
 * @retval false If character is not 0
 */
static inline bool is_terminated(char chr)
{
	if (chr == AT_CMD_BUFFER_TERMINATOR) {
		return true;
	}

	return false;
}

/**
 * @brief Check if character is a valid AT string separator
 *
 * Elements in the AT string can either be separated by ':' or ',', this
 * function will check if the character is either.
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If character is ':' or ','
 * @retval false In all other cases
 */
static inline bool is_separator(char chr)
{
	if ((chr == AT_PARAM_SEPARATOR) || (chr == AT_RSP_SEPARATOR) ||
	    (chr == AT_CMD_SEPARATOR)) {
		return true;
	}

	return false;
}

/**
 * @brief Check if character linefeed or carry return characters
 *
 * A line shift in an AT string is always identified by a '\r\n' sequence
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If an line shift character is detected
 * @retval false If character is something else
 */
static inline bool is_lfcr(char chr)
{
	if ((chr == '\r') || (chr == '\n')) {
		return true;
	}

	return false;
}

/**
 * @brief Check if character is a double quote character
 *
 * A string in an AT string is always started and stopped with a double quote
 * character.
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If character is "
 * @retval false If character is something else
 */
static inline bool is_dblquote(char chr)
{
	if (chr == '"') {
		return true;
	}

	return false;
}

/**
 * @brief Check if character is an array start character
 *
 * An array in an AT string is always started with a left parenthesis
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If character is (
 * @retval false If character is something else
 */
static inline bool is_array_start(char chr)
{
	if (chr == '(') {
		return true;
	}

	return false;
}

/**
 * @brief Check if character is an array stop character
 *
 * An array in an AT string is always stopped with a right parenthesis
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If character is )
 * @retval false If character is something else
 */
static inline bool is_array_stop(char chr)
{
	if (chr == ')') {
		return true;
	}

	return false;
}

/**
 * @brief Check if character is a number character (including + and -)
 *
 * Detect number characters in an AT string, including the + and - sign
 *
 * @param[in] chr Character that should be examined
 *
 * @retval true  If character is number character
 * @retval false If character is something else
 */
static inline bool is_number(char chr)
{
	if (isdigit(chr) || (chr == '-') || (chr == '+')) {
		return true;
	}

	return false;
}

/** @} */

#endif /* AT_UTILS_H__ */

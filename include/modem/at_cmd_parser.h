/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_CMD_PARSER_H__
#define AT_CMD_PARSER_H__

#include <stdlib.h>
#include <zephyr/types.h>

#include <modem/at_params.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file at_cmd_parser.h
 *
 * @defgroup at_cmd_parser AT command parser
 * @{
 * @brief Basic parser for AT commands.
 */

/**
 * @brief Parse a maximum number of AT command or response parameters
 *        from a string.
 *
 * This function parses the parameters from @p at_params_str and saves
 * them in @p list. If there are more parameters than @p max_params_count,
 * they are ignored.
 *
 * @p list must be initialized. It can be reused to parse multiple commands.
 * When calling this function, the list is cleared. The maximum number of AT
 * parameters that can be parsed and stored is limited by the size of @p list.
 *
 * If an error is returned by the parser, the content of @p list should be
 * ignored.
 *
 * @param at_params_str    AT parameters as a null-terminated string. Can be
 *                         numeric or string parameters.
 *
 * @param next_param_str   In the case a string contains multiple notifications,
 *                         the parser will stop parsing when it is done parsing
 *                         the first notification, and return the remainder of
 *                         the string in this pointer. The return code will be
 *                         EAGAIN. If multinotification is not used, this
 *                         pointer can be set to NULL.
 *
 * @param list             Pointer to an initialized list where parameters
 *                         are stored. Must not be NULL.
 * @param max_params_count Maximum number of parameters expected in @p
 *                         at_params_str. Can be set to a smaller value to
 *                         parse only some parameters.
 *
 * @retval 0 If the operation was successful.
 * @retval -EAGAIN New notification detected in string re-run the parser
 *                 with the string pointed to by @p next_param_str.
 * @retval -E2BIG  The at_param_list supplied cannot hold all detected
 *                 parameters in string. The list will contain the maximum
 *                 number of parameters possible.
 * @retval -EINVAL One or more of the supplied parameters are invalid.
 *
 */
int at_parser_max_params_from_str(const char *at_params_str,
				  char **next_param_str,
				  struct at_param_list *const list,
				  size_t max_params_count);

/**
 * @brief Parse AT command or response parameters from a string.
 *
 * This function parses the parameters from @p at_params_str and saves
 * them in @p list.
 *
 * @p list must be initialized. It can be reused to parse multiple commands.
 * When calling this function, the list is cleared. The maximum number of AT
 * parameters that can be parsed and stored is limited by the size of @p list.
 *
 * If an error is returned by the parser, the content of @p list should be
 * ignored.
 *
 * @param at_params_str  AT parameters as a null-terminated string. Can be
 *                       numeric or string parameters.
 *
 * @param next_param_str In the case a string contains multiple notifications,
 *                       the parser will stop parsing when it is done parsing
 *                       the first notification, and return the remainder of
 *                       the string in this pointer. The return code will be
 *                       EAGAIN. If multinotification is not used, this
 *                       pointer can be set to NULL.
 *
 * @param list           Pointer to an initialized list where parameters
 *                       are stored. Must not be NULL.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -EAGAIN New notification detected in string re-run the parser
 *                 with the string pointed to by @p next_param_str.
 * @retval -E2BIG  The at_param_list supplied cannot hold all detected
 *                 parameters in string. The list will contain the maximum
 *                 number of parameters possible.
 * @retval -EINVAL One or more of the supplied parameters are invalid.
 */
int at_parser_params_from_str(const char *at_params_str, char **next_param_str,
			      struct at_param_list *const list);

enum at_cmd_type {
	/** Unknown command, indicates that the actual command type could not
	 *  be resolved.
	 */
	AT_CMD_TYPE_UNKNOWN,
	/** AT set command. */
	AT_CMD_TYPE_SET_COMMAND,
	/** AT read command. */
	AT_CMD_TYPE_READ_COMMAND,
	/** AT test command. */
	AT_CMD_TYPE_TEST_COMMAND
};

/**
 * @brief Identify the AT command type.
 *
 * @param[in] at_cmd A pointer to the AT command string.
 *
 * @return Command type.
 */
enum at_cmd_type at_parser_cmd_type_get(const char *at_cmd);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_CMD_PARSER_H__ */

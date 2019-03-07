/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef AT_CMD_PARSER_H_
#define AT_CMD_PARSER_H_

/**@file at_cmd_parser.h
 *
 * @brief Basic parser for AT commands.
 * @defgroup at_cmd_parser AT command parser
 * @{
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <at_params.h>

/**
 * @brief Parse a maximum number of AT command or response parameters from a string.
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
 * @param list             Pointer to an initialized list where parameters
 *                         are stored. Must not be NULL.
 * @param max_params_count Maximum number of parameters expected in @p
 *                         at_params_str. Can be set to a smaller value to
 *                         parse only some parameters.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_parser_max_params_from_str(char *at_params_str,
				struct at_param_list *list,
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
 * @param at_params_str AT parameters as a null-terminated string. Can be
 *                      numeric or string parameters.
 * @param list          Pointer to an initialized list where parameters
 *                      are stored. Must not be NULL.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int at_parser_params_from_str(char *at_params_str,
				struct at_param_list *list);

/** @} */

#endif /* AT_CMD_PARSER_H_ */

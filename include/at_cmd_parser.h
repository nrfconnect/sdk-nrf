/**@file at_cmd_parser.h
 *
 * @brief Basic parser for AT commands.
 *
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef AT_CMD_PARSER_H_
#define AT_CMD_PARSER_H_

#include <stdlib.h>
#include <zephyr/types.h>
#include <at_params.h>

/**
 * @brief Parse AT command or response parameters from a string. Save parameters
 * in a list.
 *
 * If an error is returned by the parser, the content of @ref list should be
 * ignored. The @ref list list can be reused to parse multiple at commands.
 * When calling this function, the list is cleared. The @ref list list should
 * be initialized. The size of the list defines the maximum number of parameters
 * that can be parsed and stored. If there are more parameters than @ref
 * max_params_count, they will be ignored.
 *
 * @param at_params_str   AT parameters as a null-terminated String. Can be
 * numeric or string parameters.
 * @param list            Pointer to an initialized list where parameters will
 * be stored. Should not be null.
 * @param max_params_count  Maximum number of parameters expected in @ref
 * at_params_str. Can be set to a smaller value to parse only some parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_parser_max_params_from_str(char *at_params_str,
				struct at_param_list *list,
				size_t max_params_count);


/**
 * @brief Parse AT command or response parameters from a string. Save parameters
 * in a list. The size of the @ref list list defines the number of AT
 * parameters than can be parsed and stored.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int at_parser_params_from_str(char *at_params_str,
				struct at_param_list *list);

#endif /* AT_CMD_PARSER_H_ */

/**@file at_utils.h
 *
 * @brief AT parser utility functions to deal with strings.
 *
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef AT_UTILS_H_
#define AT_UTILS_H_

#include <zephyr/types.h>

/**
 * @brief Remove spaces from beginning of returned string
 *
 * Skips spaces from the beginning of the string and moves pointer to point
 * first non-space character. Caller should maintain pointer to block start for
 * deallocation purposes.
 *
 * @param[in] str address of string pointer
 * @param[out] str str pointer changed to point to first non-space
 * character.
 * @return number of removed spaces.
 */
u32_t at_params_space_count_get(char **str);


/**
 * @brief Get command length
 *
 * Counts length of AT command terminated by '\0', '?' or AT_CMD_SEPARATOR.
 *
 * @param[in] str Pointer to AT command string
 * @return length of first AT command.
 */
size_t at_params_cmd_length_get(const char *str);

#endif /* AT_UTILS_H_ */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SLM_AT_HOST_
#define SLM_AT_HOST_

/**@file slm_at_host.h
 *
 * @brief AT host for serial LTE modem
 * @{
 */

#include <zephyr/types.h>
#include <ctype.h>
#include <at_cmd_parser/at_cmd_parser.h>
#include <at_cmd.h>

/**@brief AT command handler type. */
typedef int (*slm_at_handler_t) (enum at_cmd_type);

/**@brief AT command list item type. */
typedef struct slm_at_cmd_list {
	u8_t type;
	char *string;
	slm_at_handler_t handler;
} slm_at_cmd_list_t;

/**
 * @brief Initialize AT host for serial LTE modem
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_host_init(void);

/**
 * @brief Compare name of AT command ignoring case
 *
 * @param cmd Command string received from UART
 * @param slm_cmd Propreiatry command supported by SLM
 * @param length Length of string to compare
 *
 * @retval true If two commands match, false if not.
 */
static inline bool slm_at_cmd_cmp(const char *cmd,
				const char *slm_cmd,
				u8_t length)
{
	for (int i = 0; i < length; i++) {
		if (toupper(*(cmd + i)) != *(slm_cmd + i)) {
			return false;
		}
	}

	return true;
}
/** @} */

#endif /* SLM_AT_HOST_ */

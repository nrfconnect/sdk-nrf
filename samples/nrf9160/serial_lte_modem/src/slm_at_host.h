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
#include <at_cmd_parser/at_cmd_parser.h>
#include <at_cmd.h>

/**@brief AT command handler type. */
typedef int (*slm_at_handler_t) (const char *at_cmd, size_t param_offset);

typedef struct slm_at_cmd_list {
	u8_t type;
	char *string_upper;
	char *string_lower;
	slm_at_handler_t handler;
} slm_at_cmd_list_t;

/**
 * @brief Initialize AT host for serial LTE modem
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_host_init(void);

/** @} */

#endif /* SLM_AT_HOST_ */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
#include <modem/at_cmd_parser.h>
#include <modem/at_cmd.h>

/**@brief AT command handler type. */
typedef int (*slm_at_handler_t) (enum at_cmd_type);

/**@brief AT command list item type. */
typedef struct slm_at_cmd_list {
	uint8_t type;
	char *string;
	slm_at_handler_t handler;
} slm_at_cmd_list_t;

/**@brief Arbitrary data type over AT channel. */
enum slm_data_type_t {
	DATATYPE_HEXADECIMAL,
	DATATYPE_PLAINTEXT,
	DATATYPE_JSON,
	DATATYPE_HTML,
	DATATYPE_OMATLV
};

/**
 * @brief Initialize AT host for serial LTE modem
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_host_init(void);

/** @} */

#endif /* SLM_AT_HOST_ */

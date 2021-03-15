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

/**@brief Operations in datamode. */
enum slm_datamode_operation_t {
	DATAMODE_SEND,  /* Send data in datamode */
	DATAMODE_EXIT   /* Exit data mode */
};

/**@brief Data mode sending handler type. */
typedef int (*slm_datamode_handler_t)(uint8_t op, const uint8_t *data, int len);

/**@brief Arbitrary data type over AT channel. */
enum slm_data_type_t {
	DATATYPE_HEXADECIMAL,
	DATATYPE_PLAINTEXT,
	DATATYPE_JSON,
	DATATYPE_HTML,
	DATATYPE_OMATLV,
	DATATYPE_ARBITRARY = 9 /* reserved for data mode */
};

/**
 * @brief Initialize AT host for serial LTE modem
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_host_init(void);

/**
 * @brief Uninitialize AT host for serial LTE modem
 *
 */
void slm_at_host_uninit(void);

/** @} */

#endif /* SLM_AT_HOST_ */

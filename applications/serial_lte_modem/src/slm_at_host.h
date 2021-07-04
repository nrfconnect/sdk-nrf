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
#include "slm_defines.h"

/**@brief Operations in datamode. */
enum slm_datamode_operation_t {
	DATAMODE_SEND,  /* Send data in datamode */
	DATAMODE_EXIT   /* Exit data mode */
};

/**@brief Data mode sending handler type. */
typedef int (*slm_datamode_handler_t)(uint8_t op, const uint8_t *data, int len);

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

/**
 * @brief Send AT command response
 *
 * @param str Response message
 * @param len Length of response message
 *
 */
void rsp_send(const uint8_t *str, size_t len);

/**
 * @brief Request SLM AT host to enter data mode
 *
 * @param handler Data mode handler provided by requesting module
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int enter_datamode(slm_datamode_handler_t handler);

/**
 * @brief Request SLM AT host to exit data mode
 *
 * @retval true If normal exit from data mode.
 *         false If not in data mode.
 */
bool exit_datamode(void);
/** @} */

#endif /* SLM_AT_HOST_ */

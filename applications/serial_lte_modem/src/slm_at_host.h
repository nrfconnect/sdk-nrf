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
#include <nrf_modem_at.h>
#include <modem/at_monitor.h>
#include <modem/at_cmd_parser.h>
#include "slm_defines.h"

/**@brief Operations in datamode. */
enum slm_datamode_operation {
	DATAMODE_SEND,  /* Send data in datamode */
	DATAMODE_EXIT   /* Exit data mode */
};

/**@brief Exit modes in datamode. */
enum slm_datamode_exit_mode {
	DATAMODE_EXIT_OK,      /* Exit datamode, send OK response */
	DATAMODE_EXIT_ERROR,   /* Exit datamode, send ERROR response */
	DATAMODE_EXIT_URC      /* Exit datamode, send URC notification */
};

/**@brief Data mode sending handler type.
 *
 * @retval 0 means all data is sent successfully.
 *         Positive value means the actual size of bytes that has been sent.
 *         Negative value means error occurrs in sending.
 */
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
void rsp_send(const char *str, size_t len);

/**
 * @brief Send raw data received in data mode
 *
 * @param data Raw data received
 * @param len Length of raw data
 *
 */
void data_send(const uint8_t *data, size_t len);

/**
 * @brief Request SLM AT host to enter data mode
 *
 * No AT unsolicited message or command response allowed in data mode.
 *
 * @param handler Data mode handler provided by requesting module
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int enter_datamode(slm_datamode_handler_t handler);

/**
 * @brief Check whether SLM AT host is in data mode
 *
 * @retval true if yes, false if no.
 */
bool in_datamode(void);

/**
 * @brief Request SLM AT host to exit data mode
 *
 * @param exit_mode Response type. Refer to enum slm_datamode_exit_mode.
 *
 * @retval true If normal exit from data mode.
 *         false If not in data mode.
 */
bool exit_datamode(int exit_mode);
/** @} */

#endif /* SLM_AT_HOST_ */

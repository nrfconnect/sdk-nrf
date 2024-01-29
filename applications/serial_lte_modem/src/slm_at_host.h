/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_HOST_
#define SLM_AT_HOST_

/** @file slm_at_host.h
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

/* This delay is necessary to send AT responses at low baud rates. */
#define SLM_UART_RESPONSE_DELAY K_MSEC(50)

#define SLM_DATAMODE_FLAGS_NONE	0
#define SLM_DATAMODE_FLAGS_MORE_DATA (1 << 0)

extern struct at_param_list slm_at_param_list; /* For AT parser. */
extern uint8_t slm_data_buf[SLM_MAX_MESSAGE_SIZE];  /* For socket data. */
extern uint8_t slm_at_buf[SLM_AT_MAX_CMD_LEN + 1]; /* AT command buffer. */

extern uint16_t slm_datamode_time_limit; /* Send trigger by time in data mode. */

/** @brief Operations in data mode. */
enum slm_datamode_operation {
	DATAMODE_SEND,  /* Send data in datamode */
	DATAMODE_EXIT   /* Exit data mode */
};

/** @brief Data mode sending handler type.
 *
 * @retval 0 means all data is sent successfully.
 *         Positive value means the actual size of bytes that has been sent.
 *         Negative value means error occurrs in sending.
 */
typedef int (*slm_datamode_handler_t)(uint8_t op, const uint8_t *data, int len, uint8_t flags);

/* All the AT backend API functions return 0 on success. */
struct slm_at_backend {
	int (*start)(void);
	int (*send)(const uint8_t *data, size_t len);
	int (*stop)(void);
};
/** @retval 0 on success (the new backend is successfully started). */
int slm_at_set_backend(struct slm_at_backend backend);

/**
 * @brief Sends the given data via the current AT backend.
 *
 * @retval 0 on success.
 */
int slm_at_send(const uint8_t *data, size_t len);

/** @brief Identical to slm_at_send(str, strlen(str)). */
int slm_at_send_str(const char *str);

/** @brief Processes received AT bytes. */
void slm_at_receive(const uint8_t *data, size_t len);

/**
 * @brief Initialize AT host for serial LTE modem
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_host_init(void);

/**
 * @brief Turns the current AT backend and UART power off.
 *
 * @retval 0 on success, or a (negative) error code.
 */
int slm_at_host_power_off(void);

/** @brief Counterpart to @c slm_at_host_power_off(). */
int slm_at_host_power_on(void);

/**
 * @brief Uninitialize AT host for serial LTE modem
 */
void slm_at_host_uninit(void);

/**
 * @brief Runs the SLM-proprietary @c at_cmd if it is one.
 */
int slm_at_parse(const char *cmd_str, size_t cmd_name_len);

/**
 * @brief Send AT command response
 *
 * @param fmt Response message format string
 *
 */
void rsp_send(const char *fmt, ...);

/**
 * @brief Send AT command response of OK
 */
void rsp_send_ok(void);

/**
 * @brief Send AT command response of ERROR
 */
void rsp_send_error(void);

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
 * @brief Indicate that data mode handler has closed
 *
 * @param result Result of sending in data mode.
 *
 * @retval true If handler has closed successfully.
 *         false If not in data mode.
 */
bool exit_datamode_handler(int result);
/** @} */

#endif /* SLM_AT_HOST_ */

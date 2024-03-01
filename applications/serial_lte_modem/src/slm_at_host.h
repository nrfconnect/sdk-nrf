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
#include <modem/at_cmd_custom.h>
#include <modem/at_cmd_parser.h>
#include "slm_defines.h"

/* This delay is necessary to send AT responses at low baud rates. */
#define SLM_UART_RESPONSE_DELAY K_MSEC(50)

#define SLM_DATAMODE_FLAGS_NONE	0
#define SLM_DATAMODE_FLAGS_MORE_DATA (1 << 0)
#define SLM_DATAMODE_FLAGS_EXIT_HANDLER (1 << 1)

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
 * @brief Exit the data mode handler
 *
 * Remove the callback to the data mode handler and start dropping the incoming data, until
 * the data mode is exited.
 *
 * @param result Result of sending in data mode.
 *
 * @retval true If handler has closed successfully.
 *         false If not in data mode.
 */
bool exit_datamode_handler(int result);

/**
 * @brief Get parameter list from AT command.
 *
 * @param at_cmd AT command.
 * @param list Pointer to the parameter list.
 *
 * @retval 0 on success. Otherwise, a (negative) error code is returned.
 */
int slm_get_at_param_list(const char *at_cmd, struct at_param_list **list);

/** @brief SLM AT command callback type. */
typedef int slm_at_callback(enum at_cmd_type cmd_type, const struct at_param_list *param_list,
			    uint32_t param_count);

/**
 * @brief Generic wrapper for a custom SLM AT command callback.
 *
 * This will call the AT command handler callback, which is declared with SLM_AT_CMD_CUSTOM.
 *
 * @param buf Response buffer.
 * @param len Response buffer size.
 * @param at_cmd AT command.
 * @param cb AT command callback.

 * @retval 0 on success.
 */
int slm_at_cb_wrapper(char *buf, size_t len, char *at_cmd, slm_at_callback cb);

/**
 * @brief Define a wrapper for a SLM custom AT command callback.
 *
 * Wrapper will call the generic wrapper, which will call the actual AT command handler.
 *
 * @param entry The entry name.
 * @param _filter The (partial) AT command on which the callback should trigger.
 * @param _callback The AT command handler callback.
 *
 */
#define SLM_AT_CMD_CUSTOM(entry, _filter, _callback)                                               \
	static int _callback(enum at_cmd_type cmd_type, const struct at_param_list *list,          \
			     uint32_t);                                                            \
	static int _callback##_wrapper_##entry(char *buf, size_t len, char *at_cmd)                \
	{                                                                                          \
		return slm_at_cb_wrapper(buf, len, at_cmd, _callback);                             \
	}                                                                                          \
	AT_CMD_CUSTOM(entry, _filter, _callback##_wrapper_##entry);

/** @} */

#endif /* SLM_AT_HOST_ */

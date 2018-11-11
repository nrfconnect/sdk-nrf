/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file at_host.h
 */

#ifndef AT_HOST_H__
#define AT_HOST_H__

#include <zephyr/types.h>
/** @defgroup at_host AT host
 * @{
 * @brief This module provides AT command dispatching so that external
 * applications can send and receive AT commands, responses, and events
 * over a UART interface. The AT commands and events are forwarded raw.
 */

/** @brief Termination Modes. */
enum term_modes {
    MODE_NULL_TERM, /**< Null Termination */
    MODE_CR,        /**< CR Termination */
    MODE_LF,        /**< LF Termination */
    MODE_CR_LF,     /**< CR+LF Termination */
    MODE_COUNT      /* Counter of term_modes */
};


/** @brief UARTs. */
enum select_uart {
    UART_0,
    UART_1,
    UART_2
};


/**
 * @brief Message handler.
 *
 * @param[in] p_context  Context passed to interrupt handler, set on initialization.
 * @param[in] p_data     Pointer to received data.
 * @param[in] len        Length of received data.
 */
typedef void (*at_host_msg_handler_t)(void * p_context, uint8_t * p_data, int32_t len);


/**@brief Structure for initialization. */
typedef struct
{
    enum select_uart        uart;           /**< The UART to use. */
    enum term_modes         mode;           /**< The termination mode. */
    void                  * p_context;      /**< Pointer to context provided by application. */
    at_host_msg_handler_t   p_msg_handler;  /**< Pointer to message handler function. */
} at_host_config_t;


/**
 * @brief Function for initializing the AT host module.
 *
 * The UART is initialized when this function is called.
 * An AT socket is created to communicate with the modem.
 *
 * @param config          Pointer to configuration structure, @ref at_host_config_t.
 * 
 * @return Zero on success or (negative) error code otherwise.
 */
int at_host_init(at_host_config_t * config);

/**
 * @brief Function for reading the AT socket and forwarding the data over UART.
 *
 * This function reads an AT socket in non-blocking
 * mode and transmits the data over UART.
 */
void at_host_process(void);

#endif /* AT_HOST_H_*/

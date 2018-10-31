/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <at_host.h>
#include <zephyr.h>
#include <stdio.h>
#include <uart.h>
#include <net/socket.h>
#include <string.h>

#include <logging/log.h>
#define LOG_DOMAIN at_host
#define LOG_LEVEL CONFIG_LOG_AT_HOST_LEVEL
LOG_MODULE_REGISTER(LOG_DOMAIN);

#define CONFIG_UART_0_NAME "UART_0"
#define CONFIG_UART_1_NAME "UART_1"
#define CONFIG_UART_2_NAME "UART_2"

#define INVALID_DESCRIPTOR -1

#define UART_RX_BUF_SIZE 4096 /**< UART RX buffer size. */
/**
 * @brief Size of the buffer used to parse an AT command.
 * Defines the maximum number of characters of an AT command (including null
 * termination) that we can store to decode. Must be a power of two.
 */
#define AT_MAX_CMD_LEN 4096

static bool at_rdy;
static u8_t term_mode;
static struct device *uart_dev;
static int at_socket_fd = INVALID_DESCRIPTOR;
static int characters;
static bool inside_quotes;
static u8_t at_cmd_read[AT_MAX_CMD_LEN];
static u8_t at_buff[AT_MAX_CMD_LEN];

static const char termination[3] = { '\0', '\r', '\n' };

static void at_host_at_socket_write(const void *data, size_t datalen)
{
	if ((data != NULL) && (datalen > 0)) {
		int bytes_written = send(at_socket_fd, data, datalen, 0);

		if (bytes_written <= 0) {
			LOG_ERR("\n Could not send AT-command to modem: [ %d ]",
				bytes_written);
		}
	}
}

static void uart_rx_handler(u8_t *data)
{
	/* Handling the characters of characters */
	if ((*data == 127) || (*data == 8)) { /* Backspace */
		if (characters > 0) {
			characters--;
			at_buff[characters] = 0;
		}
	} else {
		at_buff[characters++] = *data;
	}
	/* Detect start or end of quotes in the command. */
	if (*data == '"') {
		inside_quotes = !inside_quotes;
	}
	/* Ready to send the AT command. */
	if (inside_quotes == false) {
		switch (term_mode) {
		case MODE_NULL_TERM:
			if (*data == termination[term_mode]) {
				at_rdy = true;
			}
			break;
		case MODE_CR:
			if (*data == termination[term_mode]) {
				at_rdy = true;
			}
			break;
		case MODE_LF:
			if ((at_buff[characters - 2]) &&
			    *data == termination[term_mode]) {
				at_rdy = true;
			}
			break;
		case MODE_CR_LF:
			if ((at_buff[characters - 2] == '\r') &&
			    (*data == termination[term_mode - 1])) {
				at_rdy = true;
			}
			break;
		default:
			break;
		}
	}
}

static void isr(struct device *x)
{
	uart_irq_update(x);

	if (uart_irq_rx_ready(x)) {
		uart_fifo_read(x, at_cmd_read, AT_MAX_CMD_LEN);

		uart_rx_handler(at_cmd_read);
	}
}

static int at_uart_init(char *uart_dev_name)
{
	int err;

	uart_dev = device_get_binding(uart_dev_name);
	if (uart_dev == NULL) {
		LOG_ERR("Cannot bind %s\n", uart_dev_name);
		return EINVAL;
	}
	err = uart_err_check(uart_dev);
	if (err) {
		LOG_ERR("UART check failed\n");
		return EINVAL;
	}

	uart_irq_rx_enable(uart_dev);
	uart_irq_callback_set(uart_dev, isr);
	return err;
}

int at_host_init(enum select_uart uart_id, enum term_modes mode)
{
	char *uart_dev_name;
	int err;
	/* Choosing the termination mode */
	if (mode < MODE_COUNT) {
		term_mode = mode;
	} else {
		return -EINVAL;
	}

	/* Choose which UART to use */
	switch (uart_id) {
	case UART_0:
		uart_dev_name = CONFIG_UART_0_NAME;
		break;
	case UART_1:
		uart_dev_name = CONFIG_UART_1_NAME;
		break;
	case UART_2:
		uart_dev_name = CONFIG_UART_2_NAME;
	default:
		LOG_ERR("Unknown UART instance %d", uart_id);
		return -EINVAL;
	}

	/* Initialize the UART module */
	err = at_uart_init(uart_dev_name);
	if (!err) {
		/* AT socket creation. Handle must be valid to continue. */
		at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);

		if (at_socket_fd == INVALID_DESCRIPTOR) {
			LOG_ERR("Creating at_socket failed\n");
			return -EFAULT;
		}
	}

	return err;
}

void at_host_process(void)
{
	static u8_t at_read_buff[UART_RX_BUF_SIZE];

	if (at_rdy) {
		uart_irq_rx_disable(uart_dev);
		at_rdy = false;
		/* Detect at_buff overflow*/
		if (characters > AT_MAX_CMD_LEN) {
			LOG_ERR("at_buff overflow\n");
		} else {
			at_host_at_socket_write(at_buff, characters);
		}
		characters = 0;
		uart_irq_rx_enable(uart_dev);
	}
	/* Read AT socket in non-blocking mode. */
	int r_bytes = recv(at_socket_fd, at_read_buff, sizeof(at_read_buff),
			   MSG_DONTWAIT);
	/* Forward the data over UART if any. */
	/* If no data, errno is set to EGAIN and we will try again. */
	if (r_bytes > 0) {
		/* Poll out what is in the buffer gathered from the modem */
		for (size_t i = 0; i < r_bytes; i++) {
			uart_poll_out(uart_dev, at_read_buff[i]);
		}
	}
}

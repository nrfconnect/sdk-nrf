/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_DOMAIN at_host
#define LOG_LEVEL CONFIG_LOG_AT_HOST_LEVEL
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <zephyr.h>
#include <stdio.h>
#include <uart.h>
#include <string.h>
#include <init.h>
#include <at_cmd.h>

#define CONFIG_UART_0_NAME 	"UART_0"
#define CONFIG_UART_1_NAME 	"UART_1"
#define CONFIG_UART_2_NAME 	"UART_2"

#define INVALID_DESCRIPTOR 	-1

#define UART_RX_BUF_SIZE 	CONFIG_AT_HOST_UART_BUF_SIZE

#define THREAD_STACK_SIZE 	(CONFIG_AT_HOST_SOCKET_BUF_SIZE + 256)
#define THREAD_PRIORITY 	K_PRIO_PREEMPT(CONFIG_AT_HOST_THREAD_PRIO)

/**
 * @brief Size of the buffer used to parse an AT command.
 * Defines the maximum number of characters of an AT command (including null
 * termination) that we can store to decode. Must be a power of two.
 */
#define AT_MAX_CMD_LEN		CONFIG_AT_HOST_UART_BUF_SIZE

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

static enum term_modes term_mode;
static struct device *uart_dev;
static struct at_cmd_client *at_cmd_client;
static u8_t at_buf[AT_MAX_CMD_LEN];
static size_t at_buf_len;
static struct k_work cmd_send_work;
static const char termination[3] = { '\0', '\r', '\n' };

static void cmd_send(struct k_work *work)
{
	int err;
	struct at_cmd_msg cmd;

	ARG_UNUSED(work);

	cmd.buf = at_buf;
	cmd.len = at_buf_len;

	err = at_cmd_send(at_cmd_client, &cmd);
	if (err) {
		LOG_ERR("Could not send AT command to modem: %d", err);
	}

	uart_irq_rx_enable(uart_dev);
}

static void uart_rx_handler(u8_t character)
{
	static bool inside_quotes;
	static size_t cmd_len;
	size_t pos;

	cmd_len += 1;
	pos = cmd_len - 1;

	/* Handle special characters. */
	switch (character) {
	case 0x08: /* Backspace. */
		/* Fall through. */
	case 0x7F: /* DEL character */
		pos = pos ? pos - 1 : 0;
		at_buf[pos] = 0;
		cmd_len = cmd_len <= 1 ? 0 : cmd_len - 2;
		break;
	case '"':
		inside_quotes = !inside_quotes;
		 /* Fall through. */
	default:
		/* Detect AT command buffer overflow or zero length */
		if (cmd_len > AT_MAX_CMD_LEN) {
			LOG_ERR("Buffer overflow, dropping '%c'\n", character);
			cmd_len = AT_MAX_CMD_LEN;
			return;
		} else if (cmd_len < 1) {
			LOG_ERR("Invalid AT command length: %d", cmd_len);
			cmd_len = 0;
			return;
		}

		at_buf[pos] = character;
		break;
	}

	if (inside_quotes) {
		return;
	}

	/* Check if the character marks line termination. */
	switch (term_mode) {
	case MODE_NULL_TERM:
		/* Fall through. */
	case MODE_CR:
		if (character == termination[term_mode]) {
			goto send;
		}
		break;
	case MODE_LF:
		if ((at_buf[pos - 1]) &&
			character == termination[term_mode]) {
			goto send;
		}
		break;
	case MODE_CR_LF:
		if ((at_buf[pos - 1] == '\r') && (character == '\n')) {
			goto send;
		}
		break;
	default:
		LOG_ERR("Invalid termination mode: %d", term_mode);
		break;
	}

	return;
send:
	uart_irq_rx_disable(uart_dev);
	k_work_submit(&cmd_send_work);
	at_buf_len = cmd_len;
	cmd_len = 0;
}

static void isr(struct device *dev)
{
	u8_t character;

	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while (uart_fifo_read(dev, &character, 1)) {
		uart_rx_handler(character);
	}
}

static int at_uart_init(char *uart_dev_name)
{
	int err;

	uart_dev = device_get_binding(uart_dev_name);
	if (uart_dev == NULL) {
		LOG_ERR("Cannot bind %s\n", uart_dev_name);
		return -EINVAL;
	}
	err = uart_err_check(uart_dev);
	if (err) {
		LOG_ERR("UART check failed\n");
		return -EINVAL;
	}

	uart_irq_callback_set(uart_dev, isr);
	return err;
}

static void response_handler(char *cmd, size_t cmd_len,
			     char *response, size_t response_len)
{
	ARG_UNUSED(cmd);
	ARG_UNUSED(cmd_len);

	/* Forward the data over UART if any. */
	/* If no data, errno is set to EGAIN and we will try again. */
	if (response_len > 0) {
		/* Poll out what is in the buffer gathered from
		 * the modem.
		 */
		for (size_t i = 0; i < response_len; i++) {
			uart_poll_out(uart_dev, response[i]);
		}
	}
}

static int at_host_init(struct device *arg)
{
	char *uart_dev_name;
	int err;
	enum select_uart uart_id = CONFIG_AT_HOST_UART;
	enum term_modes mode = CONFIG_AT_HOST_TERMINATION;

	ARG_UNUSED(arg);

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
	if (err) {
		LOG_ERR("UART could not be initialized: %d", err);
		return -EFAULT;
	}

	at_cmd_client = at_cmd_client_init(response_handler);
	if (!at_cmd_client) {
		LOG_ERR("Could not initialize AT command client");
		return -EFAULT;
	}

	k_work_init(&cmd_send_work, cmd_send);
	uart_irq_rx_enable(uart_dev);

	return err;
}

SYS_INIT(at_host_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

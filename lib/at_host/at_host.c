/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <ctype.h>
#include <logging/log.h>
#include <drivers/uart.h>
#include <string.h>
#include <init.h>
#include <at_cmd.h>
#include <at_notif.h>

LOG_MODULE_REGISTER(at_host, CONFIG_AT_HOST_LOG_LEVEL);

/* Stack definition for AT host workqueue */
#ifdef CONFIG_SIZE_OPTIMIZATIONS
#define AT_HOST_STACK_SIZE 512
#else
#define AT_HOST_STACK_SIZE 1024
#endif

K_THREAD_STACK_DEFINE(at_host_stack_area, AT_HOST_STACK_SIZE);

#define CONFIG_UART_0_NAME      "UART_0"
#define CONFIG_UART_1_NAME      "UART_1"
#define CONFIG_UART_2_NAME      "UART_2"

#define INVALID_DESCRIPTOR      -1

#define OK_STR    "OK\r\n"
#define ERROR_STR "ERROR\r\n"

#if CONFIG_AT_HOST_CMD_MAX_LEN > CONFIG_AT_CMD_RESPONSE_MAX_LEN
#define AT_BUF_SIZE CONFIG_AT_HOST_CMD_MAX_LEN
#else
#define AT_BUF_SIZE CONFIG_AT_CMD_RESPONSE_MAX_LEN
#endif

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
static char at_buf[AT_BUF_SIZE]; /* AT command and modem response buffer */
static struct k_work_q at_host_work_q;
static struct k_work cmd_send_work;



static inline void write_uart_string(char *str)
{
	/* Send characters until, but not including, null */
	for (size_t i = 0; str[i]; i++) {
		uart_poll_out(uart_dev, str[i]);
	}
}

static void response_handler(void *context, char *response)
{
	ARG_UNUSED(context);

	/* Forward the data over UART */
	write_uart_string(response);
}

static void cmd_send(struct k_work *work)
{
	char              str[25];
	enum at_cmd_state state;
	int               err;

	ARG_UNUSED(work);

	err = at_cmd_write(at_buf, at_buf,
			   sizeof(at_buf), &state);
	if (err < 0) {
		LOG_ERR("Error while processing AT command: %d", err);
		state = AT_CMD_ERROR;
	}

	/* Handle the various error responses from modem */
	switch (state) {
	case AT_CMD_OK:
		write_uart_string(at_buf);
		write_uart_string(OK_STR);
		break;
	case AT_CMD_ERROR:
		write_uart_string(ERROR_STR);
		break;
	case AT_CMD_ERROR_CMS:
		sprintf(str, "+CMS ERROR: %d\r\n", err);
		write_uart_string(str);
		break;
	case AT_CMD_ERROR_CME:
		sprintf(str, "+CME ERROR: %d\r\n", err);
		write_uart_string(str);
		break;
	default:
		break;
	}

	uart_irq_rx_enable(uart_dev);
}

static void uart_rx_handler(u8_t character)
{
	static bool inside_quotes;
	static size_t at_cmd_len;

	/* Handle control characters */
	switch (character) {
	case 0x08: /* Backspace. */
		/* Fall through. */
	case 0x7F: /* DEL character */
		if (at_cmd_len > 0) {
			at_cmd_len--;
		}
		return;
	}

	/* Handle termination characters, if outside quotes. */
	if (!inside_quotes) {
		switch (character) {
		case '\0':
			if (term_mode == MODE_NULL_TERM) {
				goto send;
			}
			LOG_WRN("Ignored null; would terminate string early.");
			return;
		case '\r':
			if (term_mode == MODE_CR) {
				goto send;
			}
			break;
		case '\n':
			if (term_mode == MODE_LF) {
				goto send;
			}
			if (term_mode == MODE_CR_LF &&
			    at_cmd_len > 0 &&
			    at_buf[at_cmd_len - 1] == '\r') {
				goto send;
			}
			break;
		}
	}

	/* Detect AT command buffer overflow, leaving space for null */
	if (at_cmd_len + 1 > sizeof(at_buf) - 1) {
		LOG_ERR("Buffer overflow, dropping '%c'\n", character);
		return;
	}

	/* Write character to AT buffer */
	at_buf[at_cmd_len] = character;
	at_cmd_len++;

	/* Handle special written character */
	if (character == '"') {
		inside_quotes = !inside_quotes;
	}

	return;
send:
	at_buf[at_cmd_len] = '\0'; /* Terminate the command string */

	/* Reset UART handler state */
	inside_quotes = false;
	at_cmd_len = 0;

	/* Send the command, if there is one to send */
	if (at_buf[0]) {
		uart_irq_rx_disable(uart_dev); /* Stop UART to protect at_buf */
		k_work_submit_to_queue(&at_host_work_q, &cmd_send_work);
	}
}

static void isr(struct device *dev)
{
	u8_t character;

	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	/*
	 * Check that we are not sending data (buffer must be preserved then),
	 * and that a new character is available before handling each character
	 */
	while ((!k_work_pending(&cmd_send_work)) &&
	       (uart_fifo_read(dev, &character, 1))) {
		uart_rx_handler(character);
	}
}

static int at_uart_init(char *uart_dev_name)
{
	int err;
	u8_t dummy;

	uart_dev = device_get_binding(uart_dev_name);
	if (uart_dev == NULL) {
		LOG_ERR("Cannot bind %s\n", uart_dev_name);
		return -EINVAL;
	}

	u32_t start_time = k_uptime_get_32();

	/* Wait for the UART line to become valid */
	do {
		err = uart_err_check(uart_dev);
		if (err) {
			if (k_uptime_get_32() - start_time >
			    CONFIG_AT_HOST_UART_INIT_TIMEOUT) {
				LOG_ERR("UART check failed: %d. "
					"UART initialization timed out.", err);
				return -EIO;
			}

			LOG_INF("UART check failed: %d. "
				"Dropping buffer and retrying.", err);

			while (uart_fifo_read(uart_dev, &dummy, 1)) {
				/* Do nothing with the data */
			}
			k_sleep(10);
		}
	} while (err);

	uart_irq_callback_set(uart_dev, isr);
	return err;
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
		break;
	default:
		LOG_ERR("Unknown UART instance %d", uart_id);
		return -EINVAL;
	}

	err = at_notif_register_handler(NULL, response_handler);
	if (err != 0) {
		LOG_ERR("Can't register handler err=%d", err);
		return err;
	}

	/* Initialize the UART module */
	err = at_uart_init(uart_dev_name);
	if (err) {
		LOG_ERR("UART could not be initialized: %d", err);
		return -EFAULT;
	}

	k_work_init(&cmd_send_work, cmd_send);
	k_work_q_start(&at_host_work_q, at_host_stack_area,
		       K_THREAD_STACK_SIZEOF(at_host_stack_area),
		       CONFIG_AT_HOST_THREAD_PRIO);
	uart_irq_rx_enable(uart_dev);

	return err;
}

SYS_INIT(at_host_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

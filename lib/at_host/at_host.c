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
#include <net/socket.h>
#include <string.h>
#include <init.h>

#define CONFIG_UART_0_NAME 	"UART_0"
#define CONFIG_UART_1_NAME 	"UART_1"
#define CONFIG_UART_2_NAME 	"UART_2"

#define INVALID_DESCRIPTOR 	-1

#define UART_RX_BUF_SIZE 	CONFIG_AT_HOST_UART_BUF_SIZE

#define THREAD_STACK_SIZE 	(CONFIG_AT_HOST_SOCKET_BUF_SIZE + 512)
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
static int at_socket_fd = INVALID_DESCRIPTOR;
static struct pollfd fds[1];
static int nfds;
static u8_t at_buf[AT_MAX_CMD_LEN];
static size_t at_buf_len;
static struct k_work at_cmd_send_work;
static struct k_thread socket_thread;
static K_THREAD_STACK_DEFINE(socket_thread_stack, THREAD_STACK_SIZE);
static struct k_mutex socket_mutex;


static const char termination[3] = { '\0', '\r', '\n' };

static void at_cmd_send(struct k_work *work)
{
	int bytes_sent;

	ARG_UNUSED(work);

	k_mutex_lock(&socket_mutex, K_FOREVER);
	bytes_sent = send(at_socket_fd, at_buf, at_buf_len, 0);
	k_mutex_unlock(&socket_mutex);

	if (bytes_sent <= 0) {
		LOG_ERR("Could not send AT command to modem: %d", bytes_sent);
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
	k_work_submit(&at_cmd_send_work);
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

static void socket_thread_fn(void *arg1, void *arg2, void *arg3)
{
	u8_t at_read_buff[CONFIG_AT_HOST_SOCKET_BUF_SIZE] = {0};
	int err;
	int r_bytes;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		/* Poll the socket for incoming data. */
		err = poll(fds, nfds, K_FOREVER);
		if (err < 0) {
			LOG_ERR("Poll error: %d\n", err);
		}

		k_mutex_lock(&socket_mutex, K_FOREVER);
		/* Read AT socket in non-blocking mode. */
		r_bytes = recv(at_socket_fd, at_read_buff,
				sizeof(at_read_buff), MSG_DONTWAIT);
		k_mutex_unlock(&socket_mutex);

		/* Forward the data over UART if any. */
		/* If no data, errno is set to EGAIN and we will try again. */
		if (r_bytes > 0) {
			/* Poll out what is in the buffer gathered from
			 * the modem.
			 */
			for (size_t i = 0; i < r_bytes; i++) {
				uart_poll_out(uart_dev, at_read_buff[i]);
			}
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
		break;
	default:
		LOG_ERR("Unknown UART instance %d", uart_id);
		return -EINVAL;
	}

	/* Initialize the UART module */
	err = at_uart_init(uart_dev_name);
	if (err) {
		LOG_ERR("UART could not be initialized: %d", err);
		return -EFAULT;
	} else {
		/* AT socket creation. Handle must be valid to continue. */
		at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
		fds[0].fd = at_socket_fd;
		fds[0].events = ZSOCK_POLLIN;
		nfds += 1;

		if (at_socket_fd == INVALID_DESCRIPTOR) {
			LOG_ERR("Creating at_socket failed\n");
			return -EFAULT;
		}
	}

	k_mutex_init(&socket_mutex);
	k_work_init(&at_cmd_send_work, at_cmd_send);
	k_thread_create(&socket_thread, socket_thread_stack,
			K_THREAD_STACK_SIZEOF(socket_thread_stack),
			socket_thread_fn,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);
	uart_irq_rx_enable(uart_dev);

	return err;
}

SYS_INIT(at_host_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

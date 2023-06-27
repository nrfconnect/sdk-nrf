/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <zephyr/init.h>

#include <nrf_modem_at.h>
#include <modem/at_monitor.h>

LOG_MODULE_REGISTER(at_host, CONFIG_AT_HOST_LOG_LEVEL);

/* Stack definition for AT host workqueue */
#define AT_HOST_STACK_SIZE 1024

K_THREAD_STACK_DEFINE(at_host_stack_area, AT_HOST_STACK_SIZE);

#define AT_BUF_SIZE CONFIG_AT_HOST_CMD_MAX_LEN

AT_MONITOR(at_host, ANY, response_handler);

/** @brief Termination Modes. */
enum term_modes {
	MODE_NULL_TERM, /**< Null Termination */
	MODE_CR,        /**< CR Termination */
	MODE_LF,        /**< LF Termination */
	MODE_CR_LF,     /**< CR+LF Termination */
	MODE_COUNT      /* Counter of term_modes */
};

static enum term_modes term_mode;
#if DT_HAS_CHOSEN(ncs_at_host_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_at_host_uart));
#else
static const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
#endif
static bool at_buf_busy; /* Guards at_buf while processing a command */
static char at_buf[AT_BUF_SIZE]; /* AT command and modem response buffer */
static struct k_work_q at_host_work_q;
static struct k_work cmd_send_work;

static inline void write_uart_string(const char *str)
{
	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART)) {
		LOG_RAW("%s", str);
		return;
	}

	/* Send characters until, but not including, null */
	for (size_t i = 0; str[i]; i++) {
		uart_poll_out(uart_dev, str[i]);
	}
}

static void response_handler(const char *response)
{
	/* Forward the data over UART */
	write_uart_string(response);
}

static void cmd_send(struct k_work *work)
{
	int               err;

	ARG_UNUSED(work);

    /* Sending through string format rather than raw buffer in case
     * the buffer contains characters that need to be escaped
     */
	err = nrf_modem_at_cmd(at_buf, sizeof(at_buf), "%s", at_buf);
	if (err < 0) {
		LOG_ERR("Error while processing AT command: %d", err);
	}

	write_uart_string(at_buf);

	at_buf_busy = false;
	uart_irq_rx_enable(uart_dev);
}

static void uart_rx_handler(uint8_t character)
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

	/* Check for the presence of one printable non-whitespace character */
	for (const char *c = at_buf;; c++) {
		if (*c > ' ') {
			break;
		} else if (*c == '\0') {
			return; /* Drop command, if it has no such character */
		}
	}

	/* Send the command, if there is one to send */
	if (at_buf[0]) {
		uart_irq_rx_disable(uart_dev); /* Stop UART to protect at_buf */
		at_buf_busy = true;
		k_work_submit_to_queue(&at_host_work_q, &cmd_send_work);
	}
}

static void isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uint8_t character;

	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	/*
	 * Check that we are not sending data (buffer must be preserved then),
	 * and that a new character is available before handling each character
	 */
	while ((!at_buf_busy) &&
	       (uart_fifo_read(dev, &character, 1))) {
		uart_rx_handler(character);
	}
}

static int at_uart_init(const struct device *uart_dev)
{
	int err;
	uint8_t dummy;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	uint32_t start_time = k_uptime_get_32();

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
			k_sleep(K_MSEC(10));
		}
	} while (err);

	uart_irq_callback_set(uart_dev, isr);
	return err;
}

static int at_host_init(void)
{
	int err;
	enum term_modes mode = CONFIG_AT_HOST_TERMINATION;


	/* Choosing the termination mode */
	if (mode < MODE_COUNT) {
		term_mode = mode;
	} else {
		return -EINVAL;
	}

	/* Initialize the UART module */
	err = at_uart_init(uart_dev);
	if (err) {
		LOG_ERR("UART could not be initialized: %d", err);
		return -EFAULT;
	}

	k_work_init(&cmd_send_work, cmd_send);
	k_work_queue_start(&at_host_work_q, at_host_stack_area,
			   K_THREAD_STACK_SIZEOF(at_host_stack_area),
			   CONFIG_AT_HOST_THREAD_PRIO, NULL);
	uart_irq_rx_enable(uart_dev);

	return err;
}

SYS_INIT(at_host_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

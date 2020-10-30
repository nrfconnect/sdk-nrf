/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <drivers/gpio.h>
#include <uart_cobs.h>


/* Use 1000 ms timeout for RX/TX */
#define RX_TX_TIMEOUT_MS        1000
#define MSG_LEN                 MAX(sizeof(ping_name), sizeof(pong_name))


/* Struct for associating a UART COBS user with a name. */
struct user_info {
	struct uart_cobs_user user;
	const char *name;
};

static const char ping_name[] = "PING";
static const char pong_name[] = "PONG";
static char rx_data[MSG_LEN + 1];

static void cobs_idle_evt_handler(const struct uart_cobs_user *user,
				  const struct uart_cobs_evt *evt);
static void cobs_user_evt_handler(const struct uart_cobs_user *user,
				  const struct uart_cobs_evt *evt);

UART_COBS_USER_DEFINE(cobs_idle, cobs_idle_evt_handler);
static struct user_info user_ping = {
	.user = {
		.cb = cobs_user_evt_handler
	},
	.name = ping_name,
};
static struct user_info user_pong = {
	.user = {
		.cb = cobs_user_evt_handler
	},
	.name = pong_name,
};

#ifndef CONFIG_BOARD_NRF9160DK_NRF52840

static const struct device *gpiob;
static struct gpio_callback gpio_cb;

static struct k_poll_signal sig_ready = K_POLL_SIGNAL_INITIALIZER(sig_ready);
static struct k_poll_signal sig_gpio = K_POLL_SIGNAL_INITIALIZER(sig_gpio);

#endif // CONFIG_BOARD_NRF9160DK_NRF52840


static void log_error(const char *op, enum uart_cobs_err err)
{
	switch (err) {
	case UART_COBS_ERR_ABORT:
		printk("%s: aborted.\n", op);
		break;
	case UART_COBS_ERR_TIMEOUT:
		printk("%s: timed out.\n", op);
		break;
	case UART_COBS_ERR_BREAK:
		printk("%s: UART break error.\n", op);
		break;
	default:
		break;
	}
}

static bool user_start(struct user_info *info)
{
	int err = uart_cobs_user_start(&info->user);

	if (err != 0 && err != -EINPROGRESS) {
		printk("Error %d starting %s\n", err, info->name);
		return false;
	}
	return true;
}

/* Handler for UART COBS events in the idle state. */
static void cobs_idle_evt_handler(const struct uart_cobs_user *user,
				  const struct uart_cobs_evt *evt)
{

	switch (evt->type) {
	case UART_COBS_EVT_USER_START: {
		int err = uart_cobs_rx_start(&cobs_idle,
					     sizeof(ping_name) - 1);
		__ASSERT(err == 0, "Unable to start RX in idle state.");
		printk("Entered idle state.\n");

#ifndef CONFIG_BOARD_NRF9160DK_NRF52840
		k_poll_signal_raise(&sig_ready, 0);
#endif
		break;
	}
	case UART_COBS_EVT_USER_END:
#ifndef CONFIG_BOARD_NRF9160DK_NRF52840
		k_poll_signal_reset(&sig_ready);
#endif
		printk("Exited idle state.\n");
		break;
	case UART_COBS_EVT_RX:
		/* Received data - check if we recognize it and switch users
		 * if we do.
		 */
		memcpy(rx_data, evt->data.rx.buf,
		       MIN(evt->data.rx.len, MSG_LEN));
		printk("Idle: received \"%s\"\n", rx_data);
		if (strncmp(rx_data, user_ping.name,
			    strlen(user_ping.name)) == 0) {
			user_start(&user_pong);
		}
		break;
	case UART_COBS_EVT_RX_ERR:
		log_error("RX", evt->data.err);
		int err = uart_cobs_rx_start(&cobs_idle,
					     sizeof(ping_name) - 1);
		__ASSERT(err == 0, "Unable to start RX in idle state.");
		break;
	default:
		break;
	}
}

/* Handler for UART COBS events in the PING/PONG states. */
static void cobs_user_evt_handler(const struct uart_cobs_user *user,
				  const struct uart_cobs_evt *evt)
{
	struct user_info *info = CONTAINER_OF(user, struct user_info, user);
	int err;

	switch (evt->type) {
	case UART_COBS_EVT_USER_START: {
		printk("%s started!\n", info->name);
		if (info->name == ping_name) {
			err = uart_cobs_rx_start(user, sizeof(pong_name) - 1);
			__ASSERT(err == 0, "Error %d starting RX.", err);
			err = uart_cobs_rx_timeout_start(user,
							 RX_TX_TIMEOUT_MS);
			__ASSERT(err == 0, "Error %d starting RX timeout.",
				 err);
		}
		/* Send name. */
		printk("%s: sending \"%s\"\n", info->name, info->name);
		err = uart_cobs_tx_buf_write(user, info->name,
					     strnlen(info->name, MSG_LEN));
		__ASSERT(err == 0, "Error %d writing to TX buffer.", err);
		err = uart_cobs_tx_start(user, SYS_FOREVER_MS);
		__ASSERT(err == 0, "Error %d starting TX.", err);
		break;
	}
	case UART_COBS_EVT_USER_END:
		printk("%s exited with status %d.\n",
		       info->name, evt->data.err);
		break;
	case UART_COBS_EVT_RX: {
		size_t len = MIN(evt->data.rx.len, MSG_LEN);

		memcpy(rx_data, evt->data.rx.buf, len);
		printk("%s: received \"%s\"\n", info->name, rx_data);

		if (info->name == ping_name) {
			if (strncmp(rx_data, pong_name, len) == 0) {
				(void) uart_cobs_rx_timeout_stop(user);
				(void) uart_cobs_user_end(user, 0);
			} else {
				err = uart_cobs_rx_start(user,
							 sizeof(pong_name) - 1);
				__ASSERT(err == 0, "Error %d starting RX.",
					 err);
			}
		}
		break;
	}
	case UART_COBS_EVT_TX:
		if (info->name == pong_name) {
			/* Sent pong */
			(void) uart_cobs_user_end(user, 0);
		}
		break;
	case UART_COBS_EVT_RX_ERR:
		log_error("RX", evt->data.err);
		(void) uart_cobs_user_end(user, evt->data.err);
		break;
	case UART_COBS_EVT_TX_ERR:
		log_error("TX", evt->data.err);
		(void) uart_cobs_user_end(user, evt->data.err);
		break;
	default:
		break;
	}
}

#ifndef CONFIG_BOARD_NRF9160DK_NRF52840

static void button_handler(const struct device *gpiob, struct gpio_callback *cb,
			   uint32_t pins)
{
	if (pins == BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios))) {
		k_poll_signal_raise(&sig_gpio, 0);
	}
}

static int buttons_init(void)
{
	gpiob = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));
	if (gpiob == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}
	int err = gpio_pin_configure(gpiob, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INPUT |
				     DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios));

	if (err != 0) {
		goto done;
	}

	err = gpio_pin_interrupt_configure(gpiob,
					   DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		goto done;
	}

	gpio_init_callback(&gpio_cb, button_handler,
			   BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios)));
	err = gpio_add_callback(gpiob, &gpio_cb);
done:
	if (err != 0) {
		printk("Unable to configure SW0 GPIO pin!\n");
	}
	return err;
}

static void sig_evt_reset(struct k_poll_event *evt)
{
	evt->signal->signaled = 0;
	evt->state = K_POLL_STATE_NOT_READY;
}

#endif // CONFIG_BOARD_NRF9160DK_NRF52840

void main(void)
{
	printk("Starting UART COBS example.\n");

#ifdef CONFIG_BOARD_NRF9160DK_NRF52840

	int err = uart_cobs_idle_user_set(&cobs_idle);

	if (err != 0) {
		printk("Unable to set UART COBS idle user.\n");
	}

#else
	k_poll_signal_init(&sig_ready);
	k_poll_signal_init(&sig_gpio);

	struct k_poll_event evt_ready = K_POLL_EVENT_INITIALIZER(
		K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig_ready);
	struct k_poll_event evt_gpio = K_POLL_EVENT_INITIALIZER(
		K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig_gpio);

	int err = uart_cobs_idle_user_set(&cobs_idle);

	if (err != 0) {
		printk("Unable to set UART COBS idle user.\n");
		return;
	}

	k_poll(&evt_ready, 1, K_FOREVER);
	sig_evt_reset(&evt_ready);

	err = buttons_init();
	if (err != 0) {
		printk("Unable to initialize buttons.\n");
		return;
	}

	for (;;) {
		printk("Press button 1 to ping.\n");
		k_poll(&evt_gpio, 1, K_FOREVER);
		sig_evt_reset(&evt_gpio);

		printk("Button 1 pressed.\n");
		if (user_start(&user_ping)) {
			k_poll(&evt_ready, 1, K_FOREVER);
			sig_evt_reset(&evt_ready);
		} else {
			printk("Unable to start!\n");
		}
	}

#endif // CONFIG_BOARD_NRF9160DK_NRF52840
}

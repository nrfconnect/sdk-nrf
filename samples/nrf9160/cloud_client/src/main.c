/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <lte_lc.h>
#include <net/cloud.h>
#include <net/socket.h>
#include <dk_buttons_and_leds.h>

static struct cloud_backend *cloud_backend;
static struct k_delayed_work cloud_update_work;

static void cloud_update_work_fn(struct k_work *work)
{
	int err;

	printk("Publishing message: %s\n", CONFIG_CLOUD_MESSAGE);

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_MSG,
		.buf = CONFIG_CLOUD_MESSAGE,
		.len = sizeof(CONFIG_CLOUD_MESSAGE)
	};

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		printk("cloud_send failed, error: %d\n", err);
	}

#if defined(CONFIG_CLOUD_PUBLICATION_SEQUENTIAL)
	k_delayed_work_submit(
			&cloud_update_work,
			K_SECONDS(CONFIG_CLOUD_MESSAGE_PUBLICATION_INTERVAL));
#endif
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);
	ARG_UNUSED(backend);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTED:
		printk("CLOUD_EVT_CONNECTED\n");
		break;
	case CLOUD_EVT_READY:
		printk("CLOUD_EVT_READY\n");
#if defined(CONFIG_CLOUD_PUBLICATION_SEQUENTIAL)
		k_delayed_work_submit(&cloud_update_work, K_NO_WAIT);
#endif
		break;
	case CLOUD_EVT_DISCONNECTED:
		printk("CLOUD_EVT_DISCONNECTED\n");
		break;
	case CLOUD_EVT_ERROR:
		printk("CLOUD_EVT_ERROR\n");
		break;
	case CLOUD_EVT_DATA_SENT:
		printk("CLOUD_EVT_DATA_SENT\n");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		printk("CLOUD_EVT_DATA_RECEIVED\n");
		printk("Data received from cloud: %s\n", evt->data.msg.buf);
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		printk("CLOUD_EVT_PAIR_REQUEST\n");
		break;
	case CLOUD_EVT_PAIR_DONE:
		printk("CLOUD_EVT_PAIR_DONE\n");
		break;
	case CLOUD_EVT_FOTA_DONE:
		printk("CLOUD_EVT_FOTA_DONE\n");
		break;
	default:
		printk("Unknown cloud event type: %d\n", evt->type);
		break;
	}
}

static void work_init(void)
{
	k_delayed_work_init(&cloud_update_work, cloud_update_work_fn);
}

static void modem_configure(void)
{
#if defined(CONFIG_BSD_LIBRARY)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		printk("Connecting to LTE network. ");
		printk("This may take several minutes.\n");

		err = lte_lc_init_and_connect();
		if (err) {
			printk("LTE link could not be established.\n");
		}

		printk("Connected to LTE network\n");

#if defined(CONFIG_POWER_SAVING_MODE_ENABLE)
		err = lte_lc_psm_req(true);
		if (err) {
			printk("lte_lc_psm_req, error: %d\n", err);
		}

		printk("PSM mode requested\n");
#endif
	}
#endif
}

#if defined(CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS)
static void button_handler(u32_t button_states, u32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		k_delayed_work_submit(&cloud_update_work, K_NO_WAIT);
	}
}
#endif

void main(void)
{
	int err;

	printk("Cloud client has started\n");

	cloud_backend = cloud_get_binding(CONFIG_CLOUD_BACKEND);
	__ASSERT(cloud_backend != NULL, "%s backend not found",
		 CONFIG_CLOUD_BACKEND);

	err = cloud_init(cloud_backend, cloud_event_handler);
	if (err) {
		printk("Cloud backend could not be initialized, error: %d\n",
			err);
	}

	work_init();
	modem_configure();

#if defined(CONFIG_CLOUD_PUBLICATION_BUTTON_PRESS)
	err = dk_buttons_init(button_handler);
	if (err) {
		printk("dk_buttons_init, error: %d\n", err);
	}
#endif

	err = cloud_connect(cloud_backend);
	if (err) {
		printk("cloud_connect, error: %d\n", err);
	}

	struct pollfd fds[] = {
		{
			.fd = cloud_backend->config->socket,
			.events = POLLIN
		}
	};

	while (true) {
		err = poll(fds, ARRAY_SIZE(fds),
			   cloud_keepalive_time_left(cloud_backend));
		if (err < 0) {
			printk("poll() returned an error: %d\n", err);
			continue;
		}

		if (err == 0) {
			cloud_ping(cloud_backend);
			continue;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			cloud_input(cloud_backend);
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			printk("Socket error: POLLNVAL\n");
			printk("The cloud socket was unexpectedly closed.\n");
			return;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			printk("Socket error: POLLHUP\n");
			printk("Connection was closed by the cloud.\n");
			return;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			printk("Socket error: POLLERR\n");
			printk("Cloud connection was unexpectedly closed.\n");
			return;
		}
	}
}

/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include "application.h"
#include "connection.h"
#include "fota_support.h"

LOG_MODULE_REGISTER(main, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

static void msg_thread_fn(void)
{
	/* Continuously consume (that is, send to NRF Cloud) queued-up outgoing messages. */
	while (true) {
		(void)consume_device_message();
	}
}

static void app_thread_fn(void)
{
	/* Wait for first connection before starting the application. */
	(void)await_connection(K_FOREVER);

	/* Afterwards, allow application to run in this thread. */
	main_application();
}

static void conn_thread_fn(void)
{
	/* Continuously manage our connection. */
	manage_connection();
}


K_THREAD_DEFINE(app_thread, CONFIG_APPLICATION_THREAD_STACK_SIZE, app_thread_fn,
		NULL, NULL, NULL, 0, 0, 0);

K_THREAD_DEFINE(msg_thread, CONFIG_MESSAGE_THREAD_STACK_SIZE, msg_thread_fn,
		NULL, NULL, NULL, 0, 0, 0);

/* The connection thread is given higher priority (-1) so that it can preempt the other threads,
 * for instance in the event of a call to disconnect_cloud().
 *
 * Priority -1 is also a non-preeemptible priority level, so other threads, even of higher
 * priority, cannot interrupt the connection thread until it yields.
 */
K_THREAD_DEFINE(conn_thread, CONFIG_CONNECTION_THREAD_STACK_SIZE, conn_thread_fn,
		NULL, NULL, NULL, -1, 0, 0);


void main(void)
{
	LOG_INF("nRF Cloud MQTT multi-service sample has started.");
}

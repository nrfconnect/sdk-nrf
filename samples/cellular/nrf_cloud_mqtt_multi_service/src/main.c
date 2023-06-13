/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include "application.h"
#include "cloud_connection.h"
#include "message_queue.h"
#include "led_control.h"

LOG_MODULE_REGISTER(main, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

/* Here, we start the various threads that our application will run in */

#ifndef CONFIG_LED_INDICATION_DISABLED
/* Define, and automatically start the LED animation thread. See led_control.c */
K_THREAD_DEFINE(led_thread, CONFIG_LED_THREAD_STACK_SIZE, led_animation_thread_fn,
		NULL, NULL, NULL, 0, 0, 0);
#endif

/* Define, and automatically start the main application thread. See application.c */
K_THREAD_DEFINE(app_thread, CONFIG_APPLICATION_THREAD_STACK_SIZE, main_application_thread_fn,
		NULL, NULL, NULL, 0, 0, 0);

/* Define, and automatically start the message queue thread. See message_queue.c */
K_THREAD_DEFINE(msg_thread, CONFIG_MESSAGE_THREAD_STACK_SIZE, message_queue_thread_fn,
		NULL, NULL, NULL, 0, 0, 0);

/* Define, and automatically start the cloud connection thread. See cloud_connection.c */
K_THREAD_DEFINE(con_thread, CONFIG_CONNECTION_THREAD_STACK_SIZE, cloud_connection_thread_fn,
		NULL, NULL, NULL, 0, 0, 0);

/* main() is called from the main thread, which defaults to priority zero,
 * but for illustrative purposes we don't use it. main_application() could be called directly
 * from this function, rather than given its own dedicated thread.
 */
int main(void)
{
	LOG_INF("nRF Cloud MQTT multi-service sample has started, version: %s", CONFIG_APP_VERSION);
	return 0;
}

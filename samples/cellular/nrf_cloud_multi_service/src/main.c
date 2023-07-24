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
#include "fota_support_coap.h"
#include "shadow_support_coap.h"

LOG_MODULE_REGISTER(main, CONFIG_MULTI_SERVICE_LOG_LEVEL);

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

#if defined(CONFIG_NRF_CLOUD_COAP)
/* Define, and automatically start the CoAP FOTA check thread. See fota_support_coap.c */
K_THREAD_DEFINE(coap_fota, CONFIG_COAP_FOTA_THREAD_STACK_SIZE, coap_fota_thread_fn,
		NULL, NULL, NULL, 0, 0, 0);

/* Define, and automatically start the CoAP shadow check thread. See shadow_support_coap.c */
K_THREAD_DEFINE(coap_shadow, CONFIG_COAP_SHADOW_THREAD_STACK_SIZE, coap_shadow_thread_fn,
		NULL, NULL, NULL, 0, 0, 0);
#endif

/* main() is called from the main thread, which defaults to priority zero,
 * but for illustrative purposes we don't use it. main_application() could be called directly
 * from this function, rather than given its own dedicated thread.
 */
int main(void)
{
	const char *protocol;

	if (IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		protocol = "MQTT";
	} else if (IS_ENABLED(CONFIG_NRF_CLOUD_COAP)) {
		protocol = "CoAP";
	}

	LOG_INF("nRF Cloud multi-service sample has started, version: %s, protocol: %s",
		CONFIG_APP_VERSION, protocol);

	return 0;
}

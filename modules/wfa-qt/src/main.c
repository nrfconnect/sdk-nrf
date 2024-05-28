/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <unistd.h>

#include "eloop.h"
#include "indigo_api.h"
#include "utils.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(wfa_qt, CONFIG_WFA_QT_LOG_LEVEL);
int control_socket_init(int port);
void qt_main(void);
int wpa_supp_events_register(void);
int wait_for_wpa_s_ready(void);
K_THREAD_DEFINE(qt_main_tid,
		CONFIG_WFA_QT_THREAD_STACK_SIZE,
		qt_main,
		NULL,
		NULL,
		NULL,
		0,
		0,
		0);

/* Show the welcome message with role and version */
static void print_welcome(void)
{
	LOG_INF("Welcome to use QuickTrack Control App DUT version %s", TLV_VALUE_APP_VERSION);
}

void qt_main(void)
{
	int service_socket = -1;
	int ret;

	/* Welcome message */
	print_welcome();

	/* Set default wireless interface information */
	set_wireless_interface(CONFIG_WFA_QT_DEFAULT_INTERFACE);

	/* Print the run-time information */
	LOG_INF("QuickTrack control app running at: %d", get_service_port());
	LOG_INF("Wireless Interface: %s", CONFIG_WFA_QT_DEFAULT_INTERFACE);

	ret = wpa_supp_events_register();
	if (ret < 0) {
		LOG_ERR("Failed to register WPA supplicant events");
	}

	ret = wait_for_wpa_s_ready();
	if (ret < 0) {
		LOG_ERR("Failed to wait for WPA supplicant to be ready");
	}

	/* Register the callback */
	register_apis();

	/* Start eloop */
	qt_eloop_init(NULL);

	/* Bind the service port and register to eloop */
	service_socket = control_socket_init(get_service_port());
	if (service_socket >= 0) {
		qt_eloop_run();
	} else {
		LOG_ERR("Failed to initiate the UDP socket: %s", strerror(errno));
	}

	/* Stop eloop */
	qt_eloop_destroy();
	LOG_INF("ControlAppC stops");
	if (service_socket >= 0) {
		LOG_INF("Close service port: %d", get_service_port());
		close(service_socket);
	}

}

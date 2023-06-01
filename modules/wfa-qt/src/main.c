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

#define WIRELESS_INTERFACE_DEFAULT "wlan0"

LOG_MODULE_REGISTER(wfa_qt, CONFIG_WFA_QT_LOG_LEVEL);
int control_socket_init(int port);
void qt_main(void);
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

	/* Welcome message */
	print_welcome();

	/* Print the run-time information */
	LOG_INF("QuickTrack control app running at: %d", get_service_port());
	LOG_INF("Wireless Interface: %s", WIRELESS_INTERFACE_DEFAULT);

	/* Register the callback */
	register_apis();

	/* Start eloop */
	qt_eloop_init(NULL);

	/* Bind the service port and register to eloop */
	service_socket = control_socket_init(get_service_port());
	if (service_socket >= 0) {
		qt_eloop_run();
	} else {
		LOG_ERR("Failed to initiate the UDP socket");
	}

	/* Stop eloop */
	qt_eloop_destroy();
	LOG_INF("ControlAppC stops");
	if (service_socket >= 0) {
		LOG_INF("Close service port: %d", get_service_port());
		close(service_socket);
	}

}

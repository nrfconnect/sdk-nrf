/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr.h>
#include <device.h>
#include <shell/shell.h>
#include <modem/lte_lc.h>

#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif
#include "at/at_shell.h"
#if defined(CONFIG_MOSH_PING)
#include "ping/icmp_ping_shell.h"
#endif
#if defined(CONFIG_MOSH_SOCK)
#include "sock/sock_shell.h"
#endif
#if defined(CONFIG_MOSH_IPERF3)
#include <posix/sys/select.h>
#include <iperf_api.h>
#endif
#if defined(CONFIG_MOSH_LINK)
#include "link_shell.h"
#endif
#if defined(CONFIG_MOSH_CURL)
#include <nrf_curl.h>
#endif
#if defined(CONFIG_MOSH_GNSS)
#include "gnss/gnss_shell.h"
#endif
#if defined(CONFIG_MOSH_SMS)
#include "sms/sms_shell.h"
#endif
#if defined(CONFIG_MOSH_LOCATION)
#include "location_shell.h"
#endif
#if defined(CONFIG_MOSH_PPP)
#include "ppp/ppp_shell.h"
#endif
#if defined(CONFIG_MOSH_REST)
#include "rest_shell.h"
#endif
#include "uart/uart_shell.h"
#include "mosh_print.h"

extern struct k_sem nrf_modem_lib_initialized;
extern struct k_poll_signal mosh_signal;
/**
 * @brief Overriding modem library error handler.
 */
void nrf_modem_recoverable_error_handler(uint32_t error)
{
	mosh_error("modem lib recoverable error: %u\n", error);
}

#if defined(CONFIG_LWM2M_CARRIER)
void lwm2m_print_err(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_error_t *err = (lwm2m_carrier_event_error_t *)evt->data;

	static const char * const strerr[] = {
		[LWM2M_CARRIER_ERROR_NO_ERROR] =
			"No error",
		[LWM2M_CARRIER_ERROR_BOOTSTRAP] =
			"Bootstrap error",
		[LWM2M_CARRIER_ERROR_CONNECT_FAIL] =
			"Failed to connect to the LTE network",
		[LWM2M_CARRIER_ERROR_DISCONNECT_FAIL] =
			"Failed to disconnect from the LTE network",
		[LWM2M_CARRIER_ERROR_FOTA_PKG] =
			"Package refused from modem",
		[LWM2M_CARRIER_ERROR_FOTA_PROTO] =
			"Protocol error",
		[LWM2M_CARRIER_ERROR_FOTA_CONN] =
			"Connection to remote server failed",
		[LWM2M_CARRIER_ERROR_FOTA_CONN_LOST] =
			"Connection to remote server lost",
		[LWM2M_CARRIER_ERROR_FOTA_FAIL] =
			"Modem firmware update failed",
	};

	mosh_error("%s, reason %d\n", strerr[err->code], err->value);
}

void lwm2m_print_deferred(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_deferred_t *def = (lwm2m_carrier_event_deferred_t *)evt->data;

	static const char *const strdef[] = {
		[LWM2M_CARRIER_DEFERRED_NO_REASON] =
			"No reason given",
		[LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE] =
			"Failed to activate PDN",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE] =
			"No route to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT] =
			"Failed to connect to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE] =
			"Bootstrap sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE] =
			"No route to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_CONNECT] =
			"Failed to connect to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION] =
			"Server registration sequence not completed",
	};

	mosh_error("Reason: %s, timeout: %d seconds\n", strdef[def->reason], def->timeout);
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_MODEM_INIT:
		mosh_print("LwM2M carrier event: modem lib initialized");
		break;
	case LWM2M_CARRIER_EVENT_CONNECTING:
		mosh_print("LwM2M carrier event: connecting");
		k_sem_give(&nrf_modem_lib_initialized);
		break;
	case LWM2M_CARRIER_EVENT_CONNECTED:
		mosh_print("LwM2M carrier event: connected");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECTING:
		mosh_print("LwM2M carrier event: disconnecting");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECTED:
		mosh_print("LwM2M carrier event: disconnected");
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		mosh_print("LwM2M carrier event: bootstrapped");
		break;
	case LWM2M_CARRIER_EVENT_LTE_READY:
		mosh_print("LwM2M carrier event: LTE ready");
		break;
	case LWM2M_CARRIER_EVENT_REGISTERED:
		mosh_print("LwM2M carrier event: registered");
		break;
	case LWM2M_CARRIER_EVENT_DEFERRED:
		mosh_print("LwM2M carrier event: deferred");
		lwm2m_print_deferred(event);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		mosh_print("LwM2M carrier event: fota start");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		mosh_print("LwM2M carrier event: reboot");
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		mosh_print("LwM2M carrier event: error");
		lwm2m_print_err(event);
		break;
	}

	return 0;
}
#endif

#if defined(CONFIG_MOSH_IPERF3)
static int cmd_iperf3(const struct shell *shell, size_t argc, char **argv)
{
	(void)iperf_main(argc, argv, NULL, 0, &mosh_signal);
	return 0;
}
#endif

#if defined(CONFIG_MOSH_CURL)
static int cmd_curl(const struct shell *shell, size_t argc, char **argv)
{
	(void)curl_tool_main(argc, argv, &mosh_signal);
	mosh_print("\nDONE");
	return 0;
}
SHELL_CMD_REGISTER(curl, NULL, "For curl usage, just type \"curl --manual\"", cmd_curl);
#endif

SHELL_CMD_REGISTER(at, NULL, "Execute an AT command.", at_shell);

#if defined(CONFIG_MOSH_SOCK)
SHELL_CMD_REGISTER(sock, NULL,
	"Commands for socket operations such as connect and send.",
	sock_shell);
#endif

#if defined(CONFIG_MOSH_PING)
SHELL_CMD_REGISTER(ping, NULL, "For ping usage, just type \"ping\"", icmp_ping_shell);
#endif

#if defined(CONFIG_MOSH_LINK)
SHELL_CMD_REGISTER(link, NULL,
	"Commands for LTE link controlling and status information.",
	link_shell);
#endif

#if defined(CONFIG_MOSH_IPERF3)
SHELL_CMD_REGISTER(iperf3, NULL, "For iperf3 usage, just type \"iperf3 --manual\"", cmd_iperf3);
#endif

#if defined(CONFIG_MOSH_SMS)
SHELL_CMD_REGISTER(sms, NULL, "Commands for sending and receiving SMS.", sms_shell);
#endif

#if defined(CONFIG_MOSH_LOCATION)
SHELL_CMD_REGISTER(location, NULL,
	"Commands for using the Location library.",
	location_shell);
#endif

#if defined(CONFIG_MOSH_PPP)
SHELL_CMD_REGISTER(ppp, NULL,
	"Commands for controlling PPP.",
	ppp_shell_cmd);
#endif

#if defined(CONFIG_MOSH_REST)
SHELL_CMD_REGISTER(rest, NULL,
	"REST client.",
	rest_shell);
#endif

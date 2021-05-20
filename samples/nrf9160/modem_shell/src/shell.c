/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr.h>
#include <device.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
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
#if defined(CONFIG_MOSH_PPP)
#include "ppp/ppp_shell.h"
#endif

extern const struct shell *shell_global;
extern struct k_sem nrf_modem_lib_initialized;

/**
 * @brief Overriding modem library error handler.
 */
void nrf_modem_recoverable_error_handler(uint32_t error)
{
	shell_error(shell_global, "modem lib recoverable error: %u\n", error);
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

	shell_error(shell_global, "%s, reason %d\n", strerr[err->code], err->value);
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

	shell_error(shell_global, "Reason: %s, timeout: %d seconds\n",
		    strdef[def->reason], def->timeout);
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	shell_global = shell_backend_uart_get_ptr();

	switch (event->type) {
	case LWM2M_CARRIER_EVENT_BSDLIB_INIT:
		shell_print(shell_global, "LwM2M carrier event: modem lib initialized");
		break;
	case LWM2M_CARRIER_EVENT_CONNECTING:
		shell_print(shell_global, "LwM2M carrier event: connecting");
		k_sem_give(&nrf_modem_lib_initialized);
		break;
	case LWM2M_CARRIER_EVENT_CONNECTED:
		shell_print(shell_global, "LwM2M carrier event: connected");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECTING:
		shell_print(shell_global, "LwM2M carrier event: disconnecting");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECTED:
		shell_print(shell_global, "LwM2M carrier event: disconnected");
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		shell_print(shell_global, "LwM2M carrier event: bootstrapped");
		break;
	case LWM2M_CARRIER_EVENT_LTE_READY:
		shell_print(shell_global, "LwM2M carrier event: LTE ready");
		break;
	case LWM2M_CARRIER_EVENT_REGISTERED:
		shell_print(shell_global, "LwM2M carrier event: registered");
		break;
	case LWM2M_CARRIER_EVENT_DEFERRED:
		shell_print(shell_global, "LwM2M carrier event: deferred");
		lwm2m_print_deferred(event);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		shell_print(shell_global, "LwM2M carrier event: fota start");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		shell_print(shell_global, "LwM2M carrier event: reboot");
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		shell_print(shell_global, "LwM2M carrier event: error");
		lwm2m_print_err(event);
		break;
	}

	return 0;
}
#endif

#if defined(CONFIG_MOSH_IPERF3)
static int cmd_iperf3(const struct shell *shell, size_t argc, char **argv)
{
	(void)iperf_main(argc, argv);
	return 0;
}
#endif

#if defined(CONFIG_MOSH_CURL)
static int cmd_curl(const struct shell *shell, size_t argc, char **argv)
{
	(void)curl_tool_main(argc, argv);
	shell_print(shell, "\nDONE");
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

#if defined(CONFIG_MOSH_PPP)
SHELL_CMD_REGISTER(ppp, NULL,
	"Commands for controlling PPP.",
	ppp_shell_cmd);
#endif

static void disable_uarts(void)
{
	const struct device *uart_dev;

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_LOW_POWER, NULL, NULL);
	}

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart1)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_LOW_POWER, NULL, NULL);
	}
}

static void enable_uarts(void)
{
	const struct device *uart_dev;

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_ACTIVE, NULL, NULL);
	}

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart1)));
	if (uart_dev) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_ACTIVE, NULL, NULL);
	}
}

static int cmd_uart_disable(const struct shell *shell, size_t argc, char **argv)
{
	int sleep_time;

	sleep_time = atoi(argv[1]);
	if (sleep_time < 0) {
		shell_error(shell, "disable: invalid sleep time");
		return -EINVAL;
	}

	if (sleep_time > 0) {
		shell_print(shell, "disable: disabling UARTs for %d seconds", sleep_time);
	} else {
		shell_print(shell, "disable: disabling UARTs indefinitely");
	}

	k_sleep(K_MSEC(500));
	disable_uarts();

	if (sleep_time > 0) {
		k_sleep(K_SECONDS(sleep_time));

		enable_uarts();

		shell_print(shell, "disable: UARTs enabled");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_uart,
	SHELL_CMD_ARG(
		disable,
		NULL,
		"<time in seconds>\nDisable UARTs for a given number of seconds. 0 means that "
		"UARTs remain disabled indefinitely.",
		cmd_uart_disable,
		2,
		0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(uart, &sub_uart, "Commands for disabling UARTs for power measurement.", NULL);

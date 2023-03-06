/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <malloc.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/shell/shell.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif
#if defined(CONFIG_MOSH_PING)
#include "ping/icmp_ping_shell.h"
#endif
#if defined(CONFIG_MOSH_SOCK)
#include "sock/sock_shell.h"
#endif
#if defined(CONFIG_MOSH_IPERF3)
#include <zephyr/posix/sys/select.h>
#include <iperf_api.h>
#endif
#if defined(CONFIG_MOSH_LINK)
#if defined(CONFIG_LWM2M_CARRIER)
#include "link.h"
#include <modem/pdn.h>
#include "link_settings.h"
#endif /* CONFIG_LWM2M_CARRIER */
#include "link_shell.h"
#endif /* CONFIG_MOSH_LINK */
#if defined(CONFIG_MOSH_CURL)
#include <nrf_curl.h>
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

extern struct k_poll_signal mosh_signal;

#if defined(CONFIG_LWM2M_CARRIER)
void lwm2m_handle_error(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_error_t *err = evt->data.error;

	static const char * const strerr[] = {
		[LWM2M_CARRIER_ERROR_NO_ERROR] =
			"No error",
		[LWM2M_CARRIER_ERROR_BOOTSTRAP] =
			"Bootstrap error",
		[LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL] =
			"Failed to connect to the LTE network",
		[LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL] =
			"Failed to disconnect from the LTE network",
		[LWM2M_CARRIER_ERROR_FOTA_FAIL] =
			"Modem firmware update failed",
		[LWM2M_CARRIER_ERROR_CONFIGURATION] =
			"Illegal object configuration detected",
		[LWM2M_CARRIER_ERROR_INIT] =
			"Initialization failure",
		[LWM2M_CARRIER_ERROR_RUN] =
			"Configuration failure",
	};

	mosh_error("%s, reason %d\n", strerr[err->type], err->value);
}

void lwm2m_print_deferred(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_deferred_t *def = evt->data.deferred;

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
		[LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE] =
			"Server in maintenance mode",
		[LWM2M_CARRIER_DEFERRED_SIM_MSISDN] =
			"Waiting for SIM MSISDN",
	};

	mosh_error("Reason: %s, timeout: %d seconds\n", strdef[def->reason], def->timeout);
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	int err = 0;

	switch (event->type) {
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP:
		mosh_print("LwM2M carrier event: request LTE Link up");
#if defined(CONFIG_LTE_LINK_CONTROL) && defined(CONFIG_MOSH_LINK)
		link_func_mode_set(LTE_LC_FUNC_MODE_NORMAL,
				   link_sett_is_normal_mode_autoconn_rel14_used());
		return 0;
#else
		return lte_lc_normal();
#endif
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN:
		mosh_print("LwM2M carrier event: request LTE Link down");
#if defined(CONFIG_LTE_LINK_CONTROL) && defined(CONFIG_MOSH_LINK)
		link_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE, false);
#else
		err = lte_lc_offline();
#endif
		break;
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		mosh_print("LwM2M carrier event: request LTE Power off");
#if defined(CONFIG_LTE_LINK_CONTROL) && defined(CONFIG_MOSH_LINK)
		link_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF, false);
#else
		err = lte_lc_power_off();
#endif
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		mosh_print("LwM2M carrier event: bootstrapped");
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
	case LWM2M_CARRIER_EVENT_FOTA_SUCCESS:
		mosh_print("LwM2M carrier event: fota success");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		mosh_print("LwM2M carrier event: reboot");
		break;
	case LWM2M_CARRIER_EVENT_MODEM_INIT:
		mosh_print("LwM2M carrier event: modem init");
		err = nrf_modem_lib_init();
		break;
	case LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN:
		mosh_print("LwM2M carrier event: modem shutdown");
		err = nrf_modem_lib_shutdown();
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		mosh_print("LwM2M carrier event: error");
		lwm2m_handle_error(event);
		break;
	}

	return err;
}
#endif

int sleep_shell(const struct shell *shell, size_t argc, char **argv)
{
	long sleep_duration = strtol(argv[1], NULL, 10);

	if (sleep_duration > 0) {
		k_sleep(K_SECONDS(sleep_duration));
		return 0;
	}
	mosh_print("sleep: duration must be greater than zero");

	return -EINVAL;
}

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS)
extern struct sys_heap _system_heap;
#endif

int heap_shell(const struct shell *shell, size_t argc, char **argv)
{
	struct mallinfo system_stats;

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS)
	int err;
	struct sys_memory_stats kernel_stats;

	err = sys_heap_runtime_stats_get(&_system_heap, &kernel_stats);
	if (err) {
		mosh_error("heap: failed to read kernel heap statistics, error: %d", err);
	} else {
		mosh_print("kernel heap statistics:");
		mosh_print("free:           %6d", kernel_stats.free_bytes);
		mosh_print("allocated:      %6d", kernel_stats.allocated_bytes);
		mosh_print("max. allocated: %6d\n", kernel_stats.max_allocated_bytes);
	}
#endif /* CONFIG_SYS_HEAP_RUNTIME_STATS */

	/* Calculate the system heap maximum size. */
#define USED_RAM_END_ADDR POINTER_TO_UINT(&_end)
#define HEAP_BASE USED_RAM_END_ADDR
#define MAX_HEAP_SIZE (KB(CONFIG_SRAM_SIZE) - (HEAP_BASE - CONFIG_SRAM_BASE_ADDRESS))

	system_stats = mallinfo();

	mosh_print("system heap statistics:");
	mosh_print("max. size:      %6d", MAX_HEAP_SIZE);
	mosh_print("size:           %6d", system_stats.arena);
	mosh_print("free:           %6d", system_stats.fordblks);
	mosh_print("allocated:      %6d", system_stats.uordblks);

	return 0;
}

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

#if defined(CONFIG_MOSH_SOCK)
SHELL_CMD_REGISTER(sock, NULL,
	"Commands for socket operations such as connect and send.",
	sock_shell);
#endif

#if defined(CONFIG_MOSH_PING)
SHELL_CMD_REGISTER(ping, NULL, "For ping usage, just type \"ping\"", icmp_ping_shell);
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

SHELL_CMD_ARG_REGISTER(sleep, NULL,
	"Sleep for n seconds.",
	sleep_shell, 2, 0);

SHELL_CMD_ARG_REGISTER(heap, NULL,
	"Print heap usage statistics.",
	heap_shell, 1, 0);

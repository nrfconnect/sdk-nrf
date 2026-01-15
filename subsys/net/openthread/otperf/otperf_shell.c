/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(otperf, CONFIG_OTPERF_LOG_LEVEL);

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "otperf_internal.h"
#include "otperf_session.h"

#include <openthread/icmp6.h>
#include <openthread/message.h>
#include <openthread/instance.h>
#include <openthread/error.h>
#include <openthread.h>

struct ot_ping_ctx {
	struct k_sem sem;
	uint16_t id;
	bool success;
};

otIcmp6Handler ping_handler;
bool ping_handler_registered;

const uint32_t TIME_US[] = {60 * 1000 * 1000, 1000 * 1000, 1000, 0};
const char *TIME_US_UNIT[] = {"m", "s", "ms", "us"};
const uint32_t KBPS[] = {1000, 0};
const char *KBPS_UNIT[] = {"Mbps", "Kbps"};
const uint32_t K[] = {1000 * 1000, 1000, 0};
const char *K_UNIT[] = {"M", "K", ""};

static void print_number(const struct shell *sh, uint32_t value, const uint32_t *divisor_arr,
			 const char **units)
{
	const char **unit;
	const uint32_t *divisor;
	uint32_t dec, radix;

	unit = units;
	divisor = divisor_arr;

	while (value < *divisor) {
		divisor++;
		unit++;
	}

	if (*divisor != 0U) {
		radix = value / *divisor;
		dec = (value % *divisor) * 100U / *divisor;
		shell_fprintf(sh, SHELL_NORMAL, "%u.%s%u %s", radix, (dec < 10) ? "0" : "", dec,
			      *unit);
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%u %s", value, *unit);
	}
}

static void print_number_64(const struct shell *sh, uint64_t value, const uint32_t *divisor_arr,
			    const char **units)
{
	const char **unit;
	const uint32_t *divisor;
	uint32_t dec;
	uint64_t radix;

	unit = units;
	divisor = divisor_arr;

	while (value < *divisor) {
		divisor++;
		unit++;
	}

	if (*divisor != 0U) {
		radix = value / *divisor;
		dec = (value % *divisor) * 100U / *divisor;
		shell_fprintf(sh, SHELL_NORMAL, "%llu.%s%u %s", radix, (dec < 10) ? "0" : "", dec,
			      *unit);
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%llu %s", value, *unit);
	}
}

static long parse_number(const char *string, const uint32_t *divisor_arr, const char **units)
{
	const char **unit;
	const uint32_t *divisor;
	char *suffix;
	long dec;
	int cmp;

	dec = strtoul(string, &suffix, 10);
	unit = units;
	divisor = divisor_arr;

	do {
		cmp = strncasecmp(suffix, *unit++, 1);
	} while (cmp != 0 && *++divisor != 0U);

	return (*divisor == 0U) ? dec : dec * *divisor;
}

static int parse_ipv6_addr(const struct shell *sh, char *host, char *port, struct otSockAddr *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}

	ret = otIp6AddressFromString(host, &addr->mAddress);
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING, "Cannot parse address: %s\n", host);
		return -EDESTADDRREQ;
	}

	addr->mPort = strtoul(port, NULL, 10);
	if (!addr->mPort) {
		shell_fprintf(sh, SHELL_WARNING, "Invalid port %s\n", port);
		return -EINVAL;
	}

	return 0;
}
#ifdef CONFIG_OTPERF_SERVER

static int otperf_bind_host(const struct shell *sh, size_t argc, char *argv[],
			    struct otperf_download_params *param)
{
	int ret;

	if (argc >= 2) {
		param->addr.mPort = strtoul(argv[1], NULL, 10);
	} else {
		param->addr.mPort = CONFIG_OTPERF_DEFAULT_PORT;
	}

	if (argc >= 3) {
		char *addr_str = argv[2];
		struct otIp6Address addr;

		memset(&addr, 0, sizeof(addr));

		ret = otIp6AddressFromString(addr_str, &addr);
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING, "Cannot parse address \"%s\"\n", addr_str);
			return ret;
		}
		param->addr.mAddress = addr;
	}

	return 0;
}

#endif

#ifdef CONFIG_OTPERF_SERVER

static void udp_session_cb(enum otperf_status status, struct otperf_results *result,
			   void *user_data)
{
	const struct shell *sh = user_data;

	switch (status) {
	case OTPERF_SESSION_STARTED:
		shell_fprintf(sh, SHELL_NORMAL, "New session started.\n");
		break;

	case OTPERF_SESSION_FINISHED: {
		uint32_t rate_in_kbps;

		/* Compute baud rate */
		if (result->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)((result->total_len * 8ULL * USEC_PER_SEC) /
						  (result->time_in_us * 1000ULL));
		} else {
			rate_in_kbps = 0U;
		}

		shell_fprintf(sh, SHELL_NORMAL, "End of session!\n");

		shell_fprintf(sh, SHELL_NORMAL, " duration:\t\t");
		print_number_64(sh, result->time_in_us, TIME_US, TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		shell_fprintf(sh, SHELL_NORMAL, " received packets:\t%u\n",
			      result->nb_packets_rcvd);
		shell_fprintf(sh, SHELL_NORMAL, " nb packets lost:\t%u\n", result->nb_packets_lost);
		shell_fprintf(sh, SHELL_NORMAL, " nb packets outorder:\t%u\n",
			      result->nb_packets_outorder);

		shell_fprintf(sh, SHELL_NORMAL, " jitter:\t\t\t");
		print_number(sh, result->jitter_in_us, TIME_US, TIME_US_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		shell_fprintf(sh, SHELL_NORMAL, " rate:\t\t\t");
		print_number(sh, rate_in_kbps, KBPS, KBPS_UNIT);
		shell_fprintf(sh, SHELL_NORMAL, "\n");

		break;
	}

	case OTPERF_SESSION_ERROR:
		shell_fprintf(sh, SHELL_ERROR, "UDP session error.\n");
		break;

	default:
		break;
	}
}

static int cmd_udp_download_stop(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;

	ret = otperf_udp_download_stop();
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING, "UDP server not running!\n");
		return -ENOEXEC;
	}

	shell_fprintf(sh, SHELL_NORMAL, "UDP server stopped\n");

	return 0;
}

static int cmd_udp_download(const struct shell *sh, size_t argc, char *argv[])
{
	struct otperf_download_params param = {0};
	int ret;

	ret = otperf_bind_host(sh, argc, argv, &param);
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING, "Unable to bind host.\n");
		shell_help(sh);
		return -ENOEXEC;
	}

	ret = otperf_udp_download(&param, udp_session_cb, (void *)sh);
	if (ret == -EALREADY) {
		shell_fprintf(sh, SHELL_WARNING, "UDP server already started!\n");
		return -ENOEXEC;
	} else if (ret < 0) {
		shell_fprintf(sh, SHELL_ERROR, "Failed to start UDP server!\n");
		return -ENOEXEC;
	}

	k_yield();

	shell_fprintf(sh, SHELL_NORMAL, "UDP server started on port %u\n", param.addr.mPort);

	return 0;
}

#endif

static void shell_udp_upload_print_stats(const struct shell *sh, struct otperf_results *results)
{
	uint64_t rate_in_kbps, client_rate_in_kbps;

	shell_fprintf(sh, SHELL_NORMAL, "-\nUpload completed!\n");

	if (results->time_in_us != 0U) {
		rate_in_kbps = (uint32_t)((results->total_len * 8 * USEC_PER_SEC) /
					  (results->time_in_us * 1000U));
	} else {
		rate_in_kbps = 0U;
	}

	if (results->client_time_in_us != 0U) {
		client_rate_in_kbps = (uint32_t)(((uint64_t)results->nb_packets_sent *
						  (uint64_t)results->packet_size * (uint64_t)8 *
						  (uint64_t)USEC_PER_SEC) /
						 (results->client_time_in_us * 1000U));
	} else {
		client_rate_in_kbps = 0U;
	}

	if (!rate_in_kbps) {
		shell_fprintf(sh, SHELL_ERROR, "LAST PACKET NOT RECEIVED!!!\n");
	}

	shell_fprintf(sh, SHELL_NORMAL, "Statistics:\t\tserver\t(client)\n");
	shell_fprintf(sh, SHELL_NORMAL, "Duration:\t\t");
	print_number_64(sh, results->time_in_us, TIME_US, TIME_US_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, "\t(");
	print_number_64(sh, results->client_time_in_us, TIME_US, TIME_US_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, ")\n");

	shell_fprintf(sh, SHELL_NORMAL, "Num packets:\t\t%u\t(%u)\n", results->nb_packets_rcvd,
		      results->nb_packets_sent);

	shell_fprintf(sh, SHELL_NORMAL, "Num packets out order:\t%u\n",
		      results->nb_packets_outorder);
	shell_fprintf(sh, SHELL_NORMAL, "Num packets lost:\t%u\n", results->nb_packets_lost);

	shell_fprintf(sh, SHELL_NORMAL, "Jitter:\t\t\t");
	print_number(sh, results->jitter_in_us, TIME_US, TIME_US_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	shell_fprintf(sh, SHELL_NORMAL, "Rate:\t\t\t");
	print_number(sh, rate_in_kbps, KBPS, KBPS_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, "\t(");
	print_number(sh, client_rate_in_kbps, KBPS, KBPS_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, ")\n");
}

static void ot_ping_handler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo,
			    const otIcmp6Header *aIcmpHeader)
{
	struct ot_ping_ctx *ctx = (struct ot_ping_ctx *)aContext;

	if (aIcmpHeader->mType == OT_ICMP6_TYPE_ECHO_REPLY &&
	    sys_be16_to_cpu(aIcmpHeader->mData.m16[0]) == ctx->id) {
		ctx->success = true;
		k_sem_give(&ctx->sem);
	}
}

static void send_ping(const struct shell *sh, struct otIp6Address *addr, int timeout_ms)
{
	static struct ot_ping_ctx ctx;
	otInstance *instance = openthread_get_default_instance();
	otMessageInfo messageInfo;
	otMessage *message = NULL;
	otError error;
	static uint16_t id;

	if (!ping_handler_registered) {
		ping_handler.mReceiveCallback = ot_ping_handler;
		ping_handler.mContext = &ctx;
		uint32_t err = otIcmp6RegisterHandler(instance, &ping_handler);

		if (err) {
			LOG_ERR("Failed to register handler, %u", err);
		}
		ping_handler_registered = true;
	}

	k_sem_init(&ctx.sem, 0, 1);
	ctx.success = false;
	ctx.id = id++;

	message = otIp6NewMessage(instance, NULL);
	if (message == NULL) {
		shell_fprintf(sh, SHELL_WARNING, "Failed to allocate ping message\n");
	}

	memset(&messageInfo, 0, sizeof(messageInfo));
	messageInfo.mPeerAddr = *addr;
	error = otIcmp6SendEchoRequest(instance, message, &messageInfo, ctx.id);

	if (error != OT_ERROR_NONE) {
		shell_fprintf(sh, SHELL_WARNING, "Cannot send ping: %d\n", error);
		otMessageFree(message);
	}

	int ret = k_sem_take(&ctx.sem, K_MSEC(timeout_ms));

	if (ret == -EAGAIN) {
		char addr_str[OT_IP6_ADDRESS_STRING_SIZE];

		otIp6AddressToString(addr, addr_str, sizeof(addr_str));
		shell_fprintf(sh, SHELL_WARNING, "ping %s timeout\n", addr_str);
	} else if (ctx.success) {
		shell_fprintf(sh, SHELL_NORMAL, "Ping reply received!\n");
	}
}

static int execute_upload(const struct shell *sh, const struct otperf_upload_params *param)
{
	struct otperf_results results = {0};
	int ret;

	shell_fprintf(sh, SHELL_NORMAL, "Duration:\t");
	print_number_64(sh, (uint64_t)param->duration_ms * USEC_PER_MSEC, TIME_US, TIME_US_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, "\n");
	shell_fprintf(sh, SHELL_NORMAL, "Packet size:\t%u bytes\n", param->packet_size);
	shell_fprintf(sh, SHELL_NORMAL, "Rate:\t\t");
	print_number(sh, param->rate_kbps, KBPS, KBPS_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, "\n");
	shell_fprintf(sh, SHELL_NORMAL, "Starting...\n");

	struct otSockAddr *ipv6 = (struct otSockAddr *)&param->peer_addr;
	/* For IPv6, we should make sure that neighbor discovery
	 * has been done for the peer. So send ping here, wait
	 * some time and start the test after that.
	 */
	send_ping(sh, &ipv6->mAddress, MSEC_PER_SEC);

	uint32_t packet_duration = otperf_packet_duration(param->packet_size, param->rate_kbps);

	shell_fprintf(sh, SHELL_NORMAL, "Rate:\t\t");
	print_number(sh, param->rate_kbps, KBPS, KBPS_UNIT);
	shell_fprintf(sh, SHELL_NORMAL, "\n");

	if (packet_duration > 1000U) {
		shell_fprintf(sh, SHELL_NORMAL, "Packet duration %u ms\n",
			      (unsigned int)(packet_duration / 1000U));
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "Packet duration %u us\n",
			      (unsigned int)packet_duration);
	}

	ret = otperf_udp_upload(param, &results);
	if (ret < 0) {
		shell_fprintf(sh, SHELL_ERROR, "UDP upload failed (%d)\n", ret);
		return ret;
	}

	shell_udp_upload_print_stats(sh, &results);

	return 0;
}

static int shell_cmd_upload(const struct shell *sh, size_t argc, char *argv[])
{
	struct otperf_upload_params param = {0};
	struct otSockAddr ipv6 = {0};
	char *port_str;
	int ret;

	param.unix_offset_us = k_uptime_get() * USEC_PER_MSEC;

	if (argc < 2) {
		shell_fprintf(sh, SHELL_WARNING, "Not enough parameters.\n");
		shell_help(sh);
		return -ENOEXEC;
	}

	if (argc > 2) {
		port_str = argv[2];
		shell_fprintf(sh, SHELL_NORMAL, "Remote port is %s\n", port_str);
	} else {
		port_str = DEF_PORT_STR;
	}

	ret = parse_ipv6_addr(sh, argv[1], port_str, &ipv6);
	if (ret == -EDESTADDRREQ) {
		shell_fprintf(sh, SHELL_WARNING, "Invalid IPv6 address %s\n", argv[1]);
	}
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Please specify the IP address of the "
			      "remote server.\n");
		return -ENOEXEC;
	}

	char addr_str[OT_IP6_ADDRESS_STRING_SIZE];

	otIp6AddressToString(&ipv6.mAddress, addr_str, OT_IP6_ADDRESS_STRING_SIZE);
	shell_fprintf(sh, SHELL_NORMAL, "Connecting to %s\n", addr_str);

	param.peer_addr = ipv6;

	if (argc > 3) {
		param.duration_ms = MSEC_PER_SEC * strtoul(argv[3], NULL, 10);
	} else {
		param.duration_ms = MSEC_PER_SEC * CONFIG_OTPERF_DEFAULT_DURATION_S;
	}

	if (argc > 4) {
		param.packet_size = parse_number(argv[4], K, K_UNIT);
	} else {
		param.packet_size = CONFIG_OTPERF_DEFAULT_PACKET_SIZE;
	}

	if (argc > 5) {
		param.rate_kbps = (parse_number(argv[5], K, K_UNIT) + 999) / 1000;
	} else {
		param.rate_kbps = CONFIG_OTPERF_DEFAULT_RATE_KBPS;
	}

	return execute_upload(sh, &param);
}

static int cmd_udp_upload(const struct shell *sh, size_t argc, char *argv[])
{
	return shell_cmd_upload(sh, argc, argv);
}

static int cmd_udp(const struct shell *sh, size_t argc, char *argv[])
{

	shell_help(sh);
	return -ENOEXEC;
}

static int cmd_version(const struct shell *sh, size_t argc, char *argv[])
{
	shell_fprintf(sh, SHELL_NORMAL, "Version: %s\n", OTPERF_VERSION);

	return 0;
}

#ifdef CONFIG_OTPERF_SERVER
SHELL_STATIC_SUBCMD_SET_CREATE(otperf_cmd_udp_download,
			       SHELL_CMD(stop, NULL, "Stop UDP server\n", cmd_udp_download_stop),
			       SHELL_SUBCMD_SET_END);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(otperf_cmd_udp,
			       SHELL_CMD(upload, NULL,
					 "<dest ip> [<dest port> <duration> <packet size>[K] "
					 "<baud rate>[K|M]]\n"
					 "<dest ip>     IP destination\n"
					 "<dest port>   port destination\n"
					 "<duration>    of the test in seconds "
					 "(default " DEF_DURATION_SECONDS_STR ")\n"
					 "<packet size> in byte or kilobyte "
					 "(with suffix K) "
					 "(default " DEF_PACKET_SIZE_STR ")\n"
					 "<baud rate>   in kilobits or megabits "
					 "(default " DEF_RATE_KBPS_STR "K)\n"
					 "Example: udp upload 192.0.2.2 1111 1 1K 1M\n"
					 "Example: udp upload 2001:db8::2\n",
					 cmd_udp_upload),
#ifdef CONFIG_OTPERF_SERVER
			       SHELL_CMD(download, &otperf_cmd_udp_download,
					 "[<port>]:  Server port to listen on/connect to\n"
					 "[<host>]:  Bind to <host>, an interface address\n"
					 "Example: udp download 5001 192.168.0.1\n",
					 cmd_udp_download),
#endif
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(otperf_commands,
			       SHELL_CMD(udp, &otperf_cmd_udp, "Upload/Download UDP data", cmd_udp),
			       SHELL_CMD(version, NULL, "Otperf version", cmd_version),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(otperf, &otperf_commands, "Otperf commands", NULL);

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <zephyr/net/net_ip.h>

#include <openthread/cli.h>
#include <openthread/coap.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/netdata.h>
#include <openthread/message.h>

#include <string.h>

static otUdpSocket udp_socket;
static const char udp_payload[] = "Hello OpenThread World!";
#define PORT 1212

static int ot_cli_output_cb(void *context, const char *format, va_list arg)
{
	const struct shell *sh = context;

	shell_vfprintf(sh, SHELL_NORMAL, format, arg);

	return 0;
}

static void ot_cli_lazy_init(const struct shell *sh)
{
	static bool is_initialized;

	if (is_initialized) {
		return;
	}

	otCliInit(NULL, ot_cli_output_cb, (void *)sh);
	is_initialized = true;
}

typedef otError(*ot_cli_command_handler_t)(const struct shell *, size_t argc, char *argv[]);

/*
 * Invokes the OT shell command handler with the provided arguments. Then prints either
 * "Done", if the handler returns no error, or an OpenThread error code that the handler
 * returns.
 *
 * This is meant to duplicate the behavior of the OpenThread's CLI subsystem, which prints
 * the command result in a similar manner.
 */
static int ot_cli_command_invoke(ot_cli_command_handler_t handler, const struct shell *sh,
				 size_t argc, char *argv[])
{
	otError error;

	error = handler(sh, argc, argv);

	if (error != OT_ERROR_NONE) {
		shell_print(sh, "Error %d", error);
		return -EINVAL;
	}

	shell_print(sh, "Done");
	return 0;
}

/*
 * Sends the command to the RCP server as a line of text. The command is then processed by
 * the RPC server and its result is returned asynchronously using OT CLI output callback.
 *
 * This is a fallback function for commands that cannot be processed by the RPC client
 * because serialization of the underlying OpenThread APIs is not yet implemented.
 */
static int ot_cli_command_send(const struct shell *sh, size_t argc, char *argv[])
{
	char buffer[CONFIG_SHELL_CMD_BUFF_SIZE];
	char *ptr = buffer;
	char *end = buffer + sizeof(buffer);

	ot_cli_lazy_init(sh);

	for (size_t i = 1; i < argc; i++) {
		int arg_len = snprintk(ptr, end - ptr, "%s", argv[i]);

		if (arg_len < 0) {
			return arg_len;
		}

		ptr += arg_len;

		if (ptr >= end) {
			shell_fprintf(sh, SHELL_WARNING, "OT shell buffer full\n");
			return -ENOEXEC;
		}

		if (i + 1 < argc) {
			*ptr++ = ' ';
		}
	}

	*ptr = '\0';
	otCliInputLine(buffer);

	return 0;
}

static void discover_cb(otActiveScanResult *result, void *ctx)
{
	const struct shell *sh = (const struct shell *)ctx;

	if (result) {
		uint64_t ext_addr;
		uint64_t ext_pan_id;

		memcpy(&ext_addr, result->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
		shell_print(sh, "Extended address: 0x%llx", ext_addr);

		shell_print(sh, "Network name: %s", result->mNetworkName.m8);

		memcpy(&ext_pan_id, result->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE);
		shell_print(sh, "Extended PanId: 0x%llx", ext_pan_id);

		shell_print(sh, "%s", "Steering Data: ");
		shell_hexdump(sh, result->mSteeringData.m8, OT_STEERING_DATA_MAX_LENGTH);

		shell_print(sh, "PandId: 0x%4x", result->mPanId);
		shell_print(sh, "Joiner UDP port: %u", result->mJoinerUdpPort);
		shell_print(sh, "Channel: %u", result->mChannel);
		shell_print(sh, "rssi: %i, lqi: %u", result->mRssi, result->mLqi);
		shell_print(sh, "Version %u", result->mVersion);
		shell_print(sh, "is native: %s, is joinable: %s, discover: %s",
			    !!result->mIsNative ? "true" : "false",
			    !!result->mIsJoinable ? "true" : "false",
			    !!result->mDiscover ? "true" : "false");
	} else {
		shell_print(sh, "%s", "Active scanning didn't find anything");
	}
}

static otError ot_cli_command_discover(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t scan_channels;
	uint16_t pan_id;
	bool joiner;
	bool enable_eui64_filtering;
	int err = 0;

	if (argc <= 1) {
		shell_print(sh, "%s",
			    otDatasetIsCommissioned(NULL) ? "already commissioned"
							  : "not comissioned");
		return OT_ERROR_NONE;
	}

	if (argc != 5) {
		return OT_ERROR_INVALID_COMMAND;
	}

	scan_channels = shell_strtoul(argv[1], 0, &err);
	pan_id = shell_strtol(argv[2], 0, &err);
	joiner = shell_strtobool(argv[3], 0, &err);
	enable_eui64_filtering = shell_strtobool(argv[4], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string arg");
		return OT_ERROR_INVALID_COMMAND;
	}

	return otThreadDiscover(NULL, scan_channels, pan_id, joiner, enable_eui64_filtering,
				discover_cb, (void *)sh);
}

static int cmd_discover(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_discover, sh, argc, argv);
}

static otError ot_cli_command_ifconfig(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc <= 1) {
		shell_print(sh, "%s", otIp6IsEnabled(NULL) ? "up" : "down");
		return OT_ERROR_NONE;
	}

	if (strcmp(argv[1], "up") == 0) {
		return otIp6SetEnabled(NULL, true);
	}

	if (strcmp(argv[1], "down") == 0) {
		return otIp6SetEnabled(NULL, false);
	}

	return OT_ERROR_INVALID_COMMAND;
}

static otError ot_cli_command_ipmaddr(const struct shell *sh, size_t argc, char *argv[])
{
	struct in6_addr in6_addr;

	if (argc <= 1) {
		const otNetifMulticastAddress *addr;
		char addr_string[NET_IPV6_ADDR_LEN];

		for (addr = otIp6GetMulticastAddresses(NULL); addr != NULL; addr = addr->mNext) {
			memcpy(in6_addr.s6_addr, addr->mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);

			if (!net_addr_ntop(AF_INET6, &in6_addr, addr_string, sizeof(addr_string))) {
				return OT_ERROR_FAILED;
			}

			shell_print(sh, "%s", addr_string);
		}

		return OT_ERROR_NONE;
	}

	if (argc == 3 && strcmp(argv[1], "add") == 0) {
		otIp6Address addr;

		if (net_addr_pton(AF_INET6, argv[2], &in6_addr)) {
			return OT_ERROR_INVALID_ARGS;
		}

		memcpy(addr.mFields.m8, in6_addr.s6_addr, OT_IP6_ADDRESS_SIZE);

		return otIp6SubscribeMulticastAddress(NULL, &addr);
	}

	if (argc == 3 && strcmp(argv[1], "del") == 0) {
		otIp6Address addr;

		if (net_addr_pton(AF_INET6, argv[2], &in6_addr)) {
			return OT_ERROR_INVALID_ARGS;
		}

		memcpy(addr.mFields.m8, in6_addr.s6_addr, OT_IP6_ADDRESS_SIZE);

		return otIp6UnsubscribeMulticastAddress(NULL, &addr);
	}

	return OT_ERROR_INVALID_ARGS;
}

static otError ot_cli_command_mode(const struct shell *sh, size_t argc, char *argv[])
{
	otLinkModeConfig mode;

	if (argc <= 1) {
		char mode_string[3];
		size_t i = 0;

		mode = otThreadGetLinkMode(NULL);

		if (mode.mRxOnWhenIdle) {
			mode_string[i++] = 'r';
		}

		if (mode.mDeviceType) {
			mode_string[i++] = 'd';
		}

		if (mode.mNetworkData) {
			mode_string[i++] = 'n';
		}

		if (i == 0) {
			mode_string[i++] = '-';
		}

		shell_print(sh, "%.*s", i, mode_string);
		return OT_ERROR_NONE;
	}

	memset(&mode, 0, sizeof(mode));

	if (strcmp(argv[1], "-") != 0) {
		for (const char *c = argv[1]; *c; c++) {
			switch (*c) {
			case 'r':
				mode.mRxOnWhenIdle = true;
				break;
			case 'd':
				mode.mDeviceType = true;
				break;
			case 'n':
				mode.mNetworkData = true;
				break;
			default:
				return OT_ERROR_INVALID_ARGS;
			}
		}
	}

	return otThreadSetLinkMode(NULL, mode);
}

static otError ot_cli_command_pollperiod(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc <= 1) {
		shell_print(sh, "%u", (unsigned int)otLinkGetPollPeriod(NULL));
		return OT_ERROR_NONE;
	}

	return otLinkSetPollPeriod(NULL, strtoul(argv[1], NULL, 10));
}

static otError ot_cli_command_state(const struct shell *sh, size_t argc, char *argv[])
{
	static const char *const role_str[] = {"disabled", "detached", "child", "router", "leader"};

	const otDeviceRole role = otThreadGetDeviceRole(NULL);

	switch (role) {
	case OT_DEVICE_ROLE_DISABLED:
	case OT_DEVICE_ROLE_DETACHED:
	case OT_DEVICE_ROLE_CHILD:
	case OT_DEVICE_ROLE_ROUTER:
	case OT_DEVICE_ROLE_LEADER:
		shell_print(sh, "%s", role_str[role]);
		return OT_ERROR_NONE;
	default:
		return OT_ERROR_FAILED;
	}
}

static otError ot_cli_command_thread(const struct shell *sh, size_t argc, char *argv[])
{
	if (strcmp(argv[1], "start") == 0) {
		return otThreadSetEnabled(NULL, true);
	}

	if (strcmp(argv[1], "stop") == 0) {
		return otThreadSetEnabled(NULL, false);
	}

	return OT_ERROR_INVALID_COMMAND;
}

static int cmd_ifconfig(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_ifconfig, sh, argc, argv);
}

static int cmd_ipmaddr(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_ipmaddr, sh, argc, argv);
}

static int cmd_mode(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_mode, sh, argc, argv);
}

static int cmd_pollperiod(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_pollperiod, sh, argc, argv);
}

static int cmd_state(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc > 1) {
		/*
		 * Serialization of OT APIs for enforcing the role is to be done,
		 * so send the command to be processed by the RPC server.
		 */
		return ot_cli_command_send(sh, argc + 1, argv - 1);
	}

	return ot_cli_command_invoke(ot_cli_command_state, sh, argc, argv);
}

static int cmd_thread(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_invoke(ot_cli_command_thread, sh, argc, argv);
}

static int cmd_test_message(const struct shell *sh, size_t argc, char *argv[])
{
	otError error = OT_ERROR_NONE;
	otMessage *message = NULL;
	uint16_t offset = 0;
	uint16_t read = 0;
	uint16_t length = 0;
	char buf[128] = {0};

	message = otUdpNewMessage(NULL, NULL);
	if (message == NULL) {
		error = OT_ERROR_NO_BUFS;
		goto exit;
	};

	offset = otMessageGetOffset(message);

	error = otMessageAppend(message, udp_payload, sizeof(udp_payload));
	if (error != OT_ERROR_NONE) {
		goto exit;
	}

	length = otMessageGetLength(message);
	shell_print(sh, "length %d, offset %d", length, offset);
	read = otMessageRead(message, offset, buf, length);
	shell_print(sh, "read data: %s", buf);

	if (strcmp(udp_payload, buf) == 0) {
		shell_print(sh, "Payload matches.");
	} else {
		shell_print(sh, "Payload doesn't match.");
	}

	read = otMessageRead((otMessage *)2, offset, buf, length);
	shell_print(sh, "read data len %d should be 0.", read);

exit:
	if (message != NULL) {
		otMessageFree(message);
	}

	return 0;
}

static void handle_udp_receive(void *context, otMessage *message, const otMessageInfo *message_info)
{
	uint16_t length;
	uint16_t offset;
	uint16_t read;
	char buf[128] = {0};

	offset = otMessageGetOffset(message);
	length = otMessageGetLength(message);

	read = otMessageRead(message, offset, buf, length);

	if (read > 0) {
		printk("RECEIVED: '%s'\n", buf);
	} else {
		printk("message empty\n");
	}
}

static int cmd_test_udp_init(const struct shell *sh, size_t argc, char *argv[])
{
	otSockAddr listen_sock_addr;

	memset(&udp_socket, 0, sizeof(udp_socket));
	memset(&listen_sock_addr, 0, sizeof(listen_sock_addr));

	listen_sock_addr.mPort = PORT;

	otUdpOpen(NULL, &udp_socket, handle_udp_receive, NULL);
	otUdpBind(NULL, &udp_socket, &listen_sock_addr, OT_NETIF_THREAD);

	return 0;
}

static int cmd_test_udp_send(const struct shell *sh, size_t argc, char *argv[])
{
	otError error = OT_ERROR_NONE;
	otMessage *message = NULL;
	otMessageInfo message_info;
	otIp6Address destination_addr;

	memset(&message_info, 0, sizeof(message_info));
	memset(&destination_addr, 0, sizeof(destination_addr));

	destination_addr.mFields.m8[0] = 0xff;
	destination_addr.mFields.m8[1] = 0x03;
	destination_addr.mFields.m8[15] = 0x01;
	message_info.mPeerAddr = destination_addr;
	message_info.mPeerPort = PORT;

	message = otUdpNewMessage(NULL, NULL);
	if (message == NULL) {
		error = OT_ERROR_NO_BUFS;
		goto exit;
	};

	error = otMessageAppend(message, udp_payload, sizeof(udp_payload));
	if (error != OT_ERROR_NONE) {
		goto exit;
	}
	error = otUdpSend(NULL, &udp_socket, message, &message_info);
	shell_print(sh, "Udp message send.");

exit:
	if (error != OT_ERROR_NONE && message != NULL) {
		otMessageFree(message);
	}
	return 0;
}

static int cmd_test_udp_close(const struct shell *sh, size_t argc, char *argv[])
{
	otUdpClose(NULL, &udp_socket);
	shell_print(sh, "Udp socket closed.");
	return 0;
}

static int cmd_test_net_data(const struct shell *sh, size_t argc, char *argv[])
{
	const size_t net_data_len = 255;
	uint8_t buf[net_data_len];
	uint8_t data_len = net_data_len;

	if (otNetDataGet(NULL, false, buf, &data_len) == OT_ERROR_NONE) {
		shell_print(sh, "Net dataget success.");
	} else {
		shell_print(sh, "Net dataget failed.");
	}

	if (otNetDataGet(NULL, true, buf, &data_len) == OT_ERROR_NONE) {
		shell_print(sh, "Net dataget success.");
	} else {
		shell_print(sh, "Net dataget failed.");
	}

	return 0;
}

static int cmd_test_net_data_mesh_prefix(const struct shell *sh, size_t argc, char *argv[])
{
	otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
	otBorderRouterConfig config;
	otError error;

	while ((error = otNetDataGetNextOnMeshPrefix(NULL, &iterator, &config)) == OT_ERROR_NONE) {
		shell_print(sh, "iterator %d", iterator);
	}
	shell_print(sh, "OT error: %d", error);
	return 0;
}

static int cmd_test_net_data_next_service(const struct shell *sh, size_t argc, char *argv[])
{
	otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
	otServiceConfig config;
	otError error;

	while ((error = otNetDataGetNextService(NULL, &iterator, &config)) == OT_ERROR_NONE) {
		shell_print(sh, "iterator %d", iterator);
	}
	shell_print(sh, "OT error: %d", error);

	return 0;
}

static void coap_message_print(const struct shell *sh, otMessage *message)
{
	otCoapType type;
	otCoapCode code;
	uint16_t id;
	uint8_t token[32];
	uint8_t token_len;

	type = otCoapMessageGetType(message);
	code = otCoapMessageGetCode(message);
	id = otCoapMessageGetMessageId(message);
	token_len = otCoapMessageGetTokenLength(message);
	memcpy(token, otCoapMessageGetToken(message), token_len);

	shell_print(sh, "CoAP type %u, code %u, id %u, token:", type, code, id);
	shell_hexdump(sh, token, token_len);
}

static void coap_response_handler(void *ctx, otMessage *message, const otMessageInfo *message_info,
				  otError error)
{
	const struct shell *sh = ctx;

	shell_print(sh, "Called CoAP response handler, result %u", error);

	if (error == OT_ERROR_NONE) {
		coap_message_print(sh, message);
	}
}

static int cmd_test_coap_send(const struct shell *sh, size_t argc, char *argv[])
{
	otMessage *message;
	otMessageInfo msg_info;
	otError error;

	memset(&msg_info, 0, sizeof(msg_info));
	net_addr_pton(AF_INET6, argv[1], msg_info.mPeerAddr.mFields.m8);
	msg_info.mPeerPort = OT_DEFAULT_COAP_PORT;

	message = otCoapNewMessage(NULL, NULL);

	if (!message) {
		error = OT_ERROR_NO_BUFS;
		goto exit;
	}

	otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);

	error = otCoapMessageAppendUriPathOptions(message, argv[2]);

	if (error != OT_ERROR_NONE) {
		goto exit;
	}

	error = otCoapMessageSetPayloadMarker(message);

	if (error != OT_ERROR_NONE) {
		goto exit;
	}

	error = otMessageAppend(message, "request", strlen("request"));

	if (error != OT_ERROR_NONE) {
		goto exit;
	}

	error = otCoapSendRequest(NULL, message, &msg_info, coap_response_handler, (void *)sh);

exit:
	if (error != OT_ERROR_NONE && message) {
		otMessageFree(message);
	}

	if (error != OT_ERROR_NONE) {
		shell_error(sh, "Error: %u", error);
	}

	return 0;
}

static void coap_default_handler(void *ctx, otMessage *message, const otMessageInfo *message_info)
{
	const struct shell *sh = ctx;

	shell_print(sh, "Called CoAP default handler");
	coap_message_print(sh, message);
}

static void coap_resource_handler(void *ctx, otMessage *message, const otMessageInfo *message_info)
{
	const struct shell *sh = ctx;
	otError error = OT_ERROR_NONE;

	shell_print(sh, "Called CoAP resource handler");
	coap_message_print(sh, message);

	if (otCoapMessageGetCode(message) == OT_COAP_CODE_GET) {
		otMessage *response;

		shell_print(sh, "Responding to GET request");

		response = otCoapNewMessage(NULL, NULL);

		if (!response) {
			error = OT_ERROR_NO_BUFS;
			goto exit;
		}

		error = otCoapMessageInitResponse(response, message, OT_COAP_TYPE_ACKNOWLEDGMENT,
						  OT_COAP_CODE_CONTENT);

		if (error != OT_ERROR_NONE) {
			goto exit;
		}

		error = otCoapMessageSetPayloadMarker(response);

		if (error != OT_ERROR_NONE) {
			goto exit;
		}

		error = otMessageAppend(response, "response", strlen("response"));

		if (error != OT_ERROR_NONE) {
			goto exit;
		}

		error = otCoapSendResponse(NULL, response, message_info);
	}

exit:
	if (error != OT_ERROR_NONE) {
		shell_error(sh, "Error: %u", error);
	}
}

static otCoapResource coap_resource = {
	.mUriPath = "resource",
};

static int cmd_test_coap_start(const struct shell *sh, size_t argc, char *argv[])
{
	otError error;

	coap_resource.mHandler = coap_resource_handler;
	coap_resource.mContext = (void *)sh;

	otCoapSetDefaultHandler(NULL, coap_default_handler, (void *)sh);
	otCoapAddResource(NULL, &coap_resource);

	shell_print(sh, "CoAP handlers added");

	error = otCoapStart(NULL, OT_DEFAULT_COAP_PORT);

	shell_print(sh, "CoAP started");

	return 0;
}

static int cmd_test_coap_stop(const struct shell *sh, size_t argc, char *argv[])
{
	otError error;

	error = otCoapStop(NULL);

	if (error != OT_ERROR_NONE) {
		goto exit;
	}

	shell_print(sh, "CoAP stopped");

	otCoapSetDefaultHandler(NULL, NULL, NULL);
	otCoapRemoveResource(NULL, &coap_resource);

	shell_print(sh, "CoAP handlers removed");

exit:
	if (error != OT_ERROR_NONE) {
		shell_error(sh, "Error: %u", error);
	}

	return 0;
}

static int cmd_ot(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_send(sh, argc, argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	ot_cmds, SHELL_CMD_ARG(ifconfig, NULL, "Interface management", cmd_ifconfig, 1, 1),
	SHELL_CMD_ARG(ipmaddr, NULL, "IPv6 multicast configuration", cmd_ipmaddr, 1, 2),
	SHELL_CMD_ARG(mode, NULL, "Mode configuration", cmd_mode, 1, 1),
	SHELL_CMD_ARG(pollperiod, NULL, "Polling configuration", cmd_pollperiod, 1, 1),
	SHELL_CMD_ARG(state, NULL, "Current role", cmd_state, 1, 1),
	SHELL_CMD_ARG(thread, NULL, "Role management", cmd_thread, 2, 0),
	SHELL_CMD_ARG(discover, NULL, "Thread discovery scan", cmd_discover, 1, 4),
	SHELL_CMD_ARG(test_message, NULL, "Test message API", cmd_test_message, 1, 0),
	SHELL_CMD_ARG(test_udp_init, NULL, "Test udp init API", cmd_test_udp_init, 1, 0),
	SHELL_CMD_ARG(test_udp_send, NULL, "Test udp send API", cmd_test_udp_send, 1, 0),
	SHELL_CMD_ARG(test_udp_close, NULL, "Test udp close API", cmd_test_udp_close, 1, 0),
	SHELL_CMD_ARG(test_net_data, NULL, "Test netdata API", cmd_test_net_data, 1, 0),
	SHELL_CMD_ARG(test_net_data_mesh_prefix, NULL, "Test netdata msh prefix API",
		      cmd_test_net_data_mesh_prefix, 1, 0),
	SHELL_CMD_ARG(test_net_data_next_service, NULL, "Test netdata next service API",
		      cmd_test_net_data_next_service, 1, 0),
	SHELL_CMD_ARG(test_coap_send, NULL, "Test CoAP send API", cmd_test_coap_send, 3, 0),
	SHELL_CMD_ARG(test_coap_start, NULL, "Test CoAP start API", cmd_test_coap_start, 1, 0),
	SHELL_CMD_ARG(test_coap_stop, NULL, "Test CoAP stop API", cmd_test_coap_stop, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(ot, &ot_cmds,
		       "OpenThread subcommands\n"
		       "Use \"ot help\" to get the list of subcommands",
		       cmd_ot, 2, 255);

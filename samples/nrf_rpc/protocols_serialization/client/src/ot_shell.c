/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_shell.h"

#include <zephyr/shell/shell.h>
#include <zephyr/net/net_ip.h>

#include <openthread/cli.h>
#include <openthread/coap.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/netdata.h>
#include <openthread/netdiag.h>
#include <openthread/message.h>
#include <openthread/srp_client.h>
#include <openthread/dns_client.h>

#include <string.h>

#define PORT 1212

#define MAX_SUBTYPES 4
#define MAX_TXT_SIZE 128
#define MAX_TXT_ENTRIES 6
#define MAX_HOST_ADDRESSES 4
#define SERVICE_NUM 4

struct srp_service_buffers {
	char service[OT_DNS_MAX_NAME_SIZE];
	char instance[OT_DNS_MAX_LABEL_SIZE];
	otDnsTxtEntry txt_entries[MAX_TXT_ENTRIES];
	uint8_t txt_buffer[MAX_TXT_SIZE];
	const char *subtypes[MAX_SUBTYPES + 1 /* For NULL ptr */];
	char subtype_buffer[OT_DNS_MAX_NAME_SIZE];
};

struct srp_service {
	struct srp_service_buffers buffers;
	otSrpClientService service;
	bool used;
};

static bool ot_cli_is_initialized;
static otUdpSocket udp_socket;
static const char udp_payload[] = "Hello OpenThread World!";
static char srp_client_host_name[OT_DNS_MAX_NAME_SIZE];
static struct srp_service services[SERVICE_NUM];

struct srp_service *service_alloc(void)
{
	for (int i = 0; i < SERVICE_NUM; ++i) {
		if (!services[i].used) {
			services[i].used = true;
			services[i].service.mName = services[i].buffers.service;
			services[i].service.mInstanceName = services[i].buffers.instance;
			services[i].service.mTxtEntries = services[i].buffers.txt_entries;
			services[i].service.mSubTypeLabels = services[i].buffers.subtypes;

			return &services[i];
		}
	}

	return NULL;
}

static void service_free(otSrpClientService *service)
{
	struct srp_service *elem;

	if (IS_ARRAY_ELEMENT(services, service)) {
		elem  = CONTAINER_OF(service, struct srp_service, service);
		memset(elem, 0, sizeof(struct srp_service));
	}
}

static int ot_cli_output_cb(void *context, const char *format, va_list arg)
{
	const struct shell *sh = context;

	shell_vfprintf(sh, SHELL_NORMAL, format, arg);

	return 0;
}

static void ot_cli_lazy_init(const struct shell *sh)
{
	if (ot_cli_is_initialized) {
		return;
	}

	otCliInit(NULL, ot_cli_output_cb, (void *)sh);
	ot_cli_is_initialized = true;
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

static int cmd_test_srp_client_autostart(const struct shell *sh, size_t argc, char *argv[])
{
	bool enabled;
	int err = 0;

	if (argc > 1) {
		enabled = shell_strtobool(argv[1], 0, &err);

		if (err) {
			shell_error(sh, "Invalid argument: %s", argv[1]);

			return -EINVAL;
		}

		if (enabled) {
			otSrpClientEnableAutoStartMode(NULL, NULL, NULL);
		} else {
			otSrpClientDisableAutoStartMode(NULL);
		}

	} else {
		shell_print(sh, "Autostart mode: %s",
			    otSrpClientIsAutoStartModeEnabled(NULL) ? "enabled" : "disabled");
	}
	return 0;
}

static int cmd_test_srp_client_host_info(const struct shell *sh, size_t argc, char *argv[])
{
	const otSrpClientHostInfo *info = otSrpClientGetHostInfo(NULL);
	char addr_string[INET6_ADDRSTRLEN];

	shell_print(sh, "Name: %s", info->mName ? info->mName : "-");
	shell_print(sh, "State: %s", otSrpClientItemStateToString(info->mState));

	if (info->mAutoAddress) {
		shell_print(sh, "Address: auto");
	} else {
		for (int i = 0; i < info->mNumAddresses; ++i) {
			net_addr_ntop(AF_INET6, (struct in6_addr *)&info->mAddresses[i],
				      addr_string, sizeof(addr_string));

			shell_print(sh, "Address #%u: %s", i, addr_string);
		}
	}

	return 0;
}

static int cmd_test_srp_client_host_name(const struct shell *sh, size_t argc, char *argv[])
{
	memset(srp_client_host_name, 0, sizeof(srp_client_host_name));
	strncpy(srp_client_host_name, argv[1], sizeof(srp_client_host_name) - 1);

	otSrpClientSetHostName(NULL, srp_client_host_name);

	shell_print(sh, "Name set to: %s", argv[1]);

	return 0;
}

static int cmd_test_srp_client_host_address_auto(const struct shell *sh, size_t argc, char *argv[])
{
	otSrpClientEnableAutoHostAddress(NULL);

	return 0;
}

static int cmd_test_srp_client_host_remove(const struct shell *sh, size_t argc, char *argv[])
{
	bool remove_key_lease = false;
	bool send_unreg = false;
	int err = 0;

	if (argc > 1) {
		remove_key_lease = shell_strtobool(argv[1], 0, &err);

		if (err) {
			shell_error(sh, "Invalid argument: %s", argv[1]);

			return -EINVAL;
		}
	}

	if (argc > 2) {
		send_unreg = shell_strtobool(argv[2], 0, &err);

		if (err) {
			shell_error(sh, "Invalid argument: %s", argv[2]);

			return -EINVAL;
		}
	}

	otSrpClientRemoveHostAndServices(NULL, remove_key_lease, send_unreg);

	return 0;
}

static int cmd_test_srp_client_host_clear(const struct shell *sh, size_t argc, char *argv[])
{
	otSrpClientClearHostAndServices(NULL);
	return 0;
}

static int cmd_test_srp_client_key_lease_interval(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t interval;

	if (argc > 1) {
		interval = shell_strtoul(argv[1], 0, &err);

		if (err) {
			shell_print(sh, "Invalid arg: %s", argv[1]);

			return -EINVAL;
		}

		otSrpClientSetKeyLeaseInterval(NULL, interval);

		return 0;
	}

	interval = otSrpClientGetKeyLeaseInterval(NULL);

	shell_print(sh, "Key lease interval: %u", interval);

	return 0;
}

static int cmd_test_srp_client_lease_interval(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t interval;

	if (argc > 1) {
		interval = shell_strtoul(argv[1], 0, &err);

		if (err) {
			shell_print(sh, "Invalid arg: %s", argv[1]);

			return -EINVAL;
		}

		otSrpClientSetLeaseInterval(NULL, interval);

		return 0;
	}

	interval = otSrpClientGetLeaseInterval(NULL);

	shell_print(sh, "Lease interval: %u", interval);

	return 0;
}

static int cmd_test_srp_client_server(const struct shell *sh, size_t argc, char *argv[])
{
	const otSockAddr *sockaddr = otSrpClientGetServerAddress(NULL);
	char addr_string[NET_IPV6_ADDR_LEN];

	if (!net_addr_ntop(AF_INET6, (struct in6_addr *)&sockaddr->mAddress, addr_string,
			   sizeof(addr_string))) {
		return -EINVAL;
	}

	shell_print(sh, "Server: [%s]:%u", addr_string, sockaddr->mPort);

	return 0;
}

static int cmd_test_srp_client_start(const struct shell *sh, size_t argc, char *argv[])
{
	otSockAddr sockaddr;
	int err = 0;

	if (net_addr_pton(AF_INET6, argv[1], (struct in6_addr *)&sockaddr.mAddress)) {
		shell_error(sh, "Invalid IPv6 address: %s", argv[1]);

		return -EINVAL;
	}

	sockaddr.mPort = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_error(sh, "Invalid port: %s", argv[2]);

		return -EINVAL;
	}

	otSrpClientStart(NULL, &sockaddr);

	return 0;
}

static int cmd_test_srp_client_state(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "%s", otSrpClientIsRunning(NULL) ? "enabled" : "disabled");

	return 0;
}

static int cmd_test_srp_client_stop(const struct shell *sh, size_t argc, char *argv[])
{
	otSrpClientStop(NULL);

	return 0;
}

static int cmd_test_srp_client_ttl(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t ttl;
	int err = 0;

	if (argc > 1) {
		ttl = shell_strtoul(argv[1], 0, &err);

		if (err) {
			shell_error(sh, "Invalid TTL: %s", argv[1]);

			return -EINVAL;
		}

		otSrpClientSetTtl(NULL, ttl);

		return 0;
	}

	ttl = otSrpClientGetTtl(NULL);

	shell_print(sh, "TTL: %u", ttl);

	return 0;
}

static int cmd_test_vendor_data(const struct shell *sh, size_t argc, char *argv[])
{
	otThreadSetVendorName(NULL, argv[1]);
	otThreadSetVendorModel(NULL, argv[2]);
	otThreadSetVendorSwVersion(NULL, argv[3]);

	shell_print(sh, "Vendor name set to: %s", otThreadGetVendorName(NULL));
	shell_print(sh, "Vendor model set to: %s", otThreadGetVendorModel(NULL));
	shell_print(sh, "Vendor SW version set to: %s", otThreadGetVendorSwVersion(NULL));

	return 0;
}

static void print_txt_entry(const struct shell *sh, const otDnsTxtEntry *entry)
{
	char buffer[128];
	size_t written = 0;

	if (entry->mKey) {
		written += snprintf(&buffer[written], sizeof(buffer) - written - 1, "%s",
				    entry->mKey);
	}

	if (entry->mValue) {
		written += snprintf(&buffer[written], sizeof(buffer) - written - 1, "=");

		for (int i = 0; i < entry->mValueLength; ++i) {
			written += snprintf(&buffer[written], sizeof(buffer) - written - 1, "%02x",
					    *(&entry->mValue[i]));
		}
	}

	buffer[written] = '\0';

	shell_print(sh, "\t\t%s", buffer);
}

static void print_srp_service_info(const struct shell *sh, const otSrpClientService *service)
{
	const char *const *subtype = service->mSubTypeLabels;
	size_t index = 0;

	shell_print(sh, "\tInstance: %s", service->mInstanceName);
	shell_print(sh, "\tName: %s", service->mName);

	subtype = service->mSubTypeLabels;

	while (subtype[index]) {
		shell_print(sh, "\tSubtype #%u: %s", index, subtype[index]);
		index++;
	}

	shell_print(sh, "\tPort: %u", service->mPort);
	shell_print(sh, "\tWeight: %u", service->mWeight);
	shell_print(sh, "\tPriority: %u", service->mPriority);

	shell_print(sh, "\tState: %s", otSrpClientItemStateToString(service->mState));
	shell_print(sh, "\tLease interval: %u", service->mLease);
	shell_print(sh, "\tKey lease interval: %u", service->mKeyLease);

	if (service->mTxtEntries == 0) {
		shell_print(sh, "TXT data: -");
	} else {
		shell_print(sh, "\tTXT data:");
		for (int i = 0; i < service->mNumTxtEntries; ++i) {
			print_txt_entry(sh, &service->mTxtEntries[i]);
		}
	}
}

static int cmd_test_srp_client_services(const struct shell *sh, size_t argc, char *argv[])
{
	const otSrpClientService *service = otSrpClientGetServices(NULL);

	shell_print(sh, "Services: ");

	if (!service) {
		shell_print(sh, "-");

		return 0;
	}

	while (service) {
		shell_print(sh, "Service 0x%p", service);
		print_srp_service_info(sh, service);

		service = service->mNext;
	}

	return 0;
}

static int cmd_test_srp_client_service_add(const struct shell *sh, size_t argc, char *argv[])
{
	struct srp_service *service = service_alloc();
	int err = 0;
	int arg = 3;
	size_t bpos = 0;
	size_t size = 0;
	otError ot_error;
	size_t left;

	if (!service) {
		shell_error(sh, "No more service buffers available");
		return -ENOEXEC;
	}

	/* Format: <instance> <service> [<subtype>]* <port> <priority> <weight> [<key>=<value>]* */
	strncpy(service->buffers.instance, argv[1], sizeof(service->buffers.instance) - 1);
	strncpy(service->buffers.service, argv[2], sizeof(service->buffers.service) - 1);

	left = sizeof(service->buffers.subtype_buffer);

	/* Go thorugh subtypes */
	for (int i = 0; i < MAX_SUBTYPES && arg < argc - 3; ++i, ++arg) {
		if (argv[arg][0] != '_') {
			break;
		}

		size = strlen(argv[arg]) + 1;

		if (size > left) {
			break;
		}

		strcpy(&service->buffers.subtype_buffer[bpos], argv[arg]);
		service->buffers.subtypes[i] = &service->buffers.subtype_buffer[bpos];
		bpos += size;
		left -= size;
	}

	service->service.mPort = shell_strtoul(argv[arg++], 0, &err);
	service->service.mPriority = shell_strtoul(argv[arg++], 0, &err);
	service->service.mWeight = shell_strtoul(argv[arg++], 0, &err);

	if (err) {
		shell_error(sh, "Failed to parse service info");

		return -EINVAL;
	}

	bpos = 0;
	left = sizeof(service->buffers.txt_buffer);

	for (int i = 0; i < MAX_TXT_ENTRIES && arg < argc; ++i, ++arg) {
		size = hex2bin(argv[arg], strlen(argv[arg]), &service->buffers.txt_buffer[bpos],
			       left);

		service->buffers.txt_entries[i].mKey = NULL;
		service->buffers.txt_entries[i].mValue = &service->buffers.txt_buffer[bpos];
		service->buffers.txt_entries[i].mValueLength = size;
	}

	ot_error = otSrpClientAddService(NULL, &service->service);

	shell_print(sh, "Status: %u", ot_error);

	return 0;
}

static int cmd_test_srp_client_service_remove(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	otError error;
	otSrpClientService *service;

	service = (otSrpClientService *)shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_error(sh, "Invalid arg: %s", argv[1]);

		return -EINVAL;
	}

	error = otSrpClientRemoveService(NULL, service);

	shell_print(sh, "Status: %u", error);

	return 0;
}

static int cmd_test_srp_client_service_clear(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	otError error;
	otSrpClientService *service;

	service = (otSrpClientService *)shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_error(sh, "Invalid arg: %s", argv[1]);

		return -EINVAL;
	}

	error = otSrpClientClearService(NULL, service);

	service_free(service);

	shell_print(sh, "Status: %u", error);

	return 0;
}

static int cmd_test_srp_client_host_addresses(const struct shell *sh, size_t argc, char *argv[])
{
	otError error;
	otIp6Address addresses[MAX_HOST_ADDRESSES];

	for (int arg = 1; arg < argc; ++arg) {
		if (net_addr_pton(AF_INET6, argv[arg], &addresses[arg - 1])) {
			shell_error(sh, "Failed to parse IPv6 address: %s", argv[arg]);

			return -EINVAL;
		}
	}

	error = otSrpClientSetHostAddresses(NULL, addresses, argc - 1);

	shell_print(sh, "Status: %u", error);

	return 0;
}

static int cmd_dns_client_config_get(const struct shell *sh, size_t argc, char *argv[])
{
	const otDnsQueryConfig *config;
	struct in6_addr in6_addr;
	char addr_string[NET_IPV6_ADDR_LEN];

	config = otDnsClientGetDefaultConfig(NULL);

	memcpy(in6_addr.s6_addr, config->mServerSockAddr.mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);

	if (!net_addr_ntop(AF_INET6, &in6_addr, addr_string, sizeof(addr_string))) {
		shell_error(sh, "Failed to convert the IPv6 address");
		return -EINVAL;
	}

	shell_print(sh, "Address: [%s]:%u", addr_string, config->mServerSockAddr.mPort);
	shell_print(sh, "Response timeout: %u", config->mResponseTimeout);
	shell_print(sh, "Max attempts: %u", config->mMaxTxAttempts);
	shell_print(sh, "Recursion flag: %u", config->mRecursionFlag);
	shell_print(sh, "NAT64 mode: %u", config->mNat64Mode);
	shell_print(sh, "Service mode: %u", config->mServiceMode);
	shell_print(sh, "Transport protocol: %u", config->mTransportProto);

	return 0;
}

static int cmd_dns_client_config_set(const struct shell *sh, size_t argc, char *argv[])
{
	struct in6_addr in6_addr;
	otDnsQueryConfig config;
	unsigned long port;
	unsigned long timeout;
	unsigned long attempts;
	unsigned long recursion;
	unsigned long nat_mode;
	unsigned long service_mode;
	unsigned long transport;
	int err = 0;

	if (net_addr_pton(AF_INET6, argv[1], &in6_addr)) {
		shell_error(sh, "Invalid address");
		return -EINVAL;
	}

	port = shell_strtoul(argv[2], 0, &err);
	timeout = shell_strtoul(argv[3], 0, &err);
	attempts = shell_strtoul(argv[4], 0, &err);
	recursion = shell_strtoul(argv[5], 0, &err);
	nat_mode = shell_strtoul(argv[6], 0, &err);
	service_mode = shell_strtoul(argv[7], 0, &err);
	transport = shell_strtoul(argv[8], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse config string");
		return -EINVAL;
	}

	memcpy(config.mServerSockAddr.mAddress.mFields.m8, in6_addr.s6_addr, OT_IP6_ADDRESS_SIZE);

	config.mServerSockAddr.mPort = (uint16_t)port;
	config.mResponseTimeout = (uint32_t)timeout;
	config.mMaxTxAttempts = (uint32_t)attempts;
	config.mRecursionFlag = (otDnsRecursionFlag)recursion;
	config.mNat64Mode = (otDnsNat64Mode)nat_mode;
	config.mServiceMode = (otDnsServiceMode)service_mode;
	config.mTransportProto = (otDnsTransportProto)transport;

	otDnsClientSetDefaultConfig(NULL, &config);

	return 0;
}

static void dns_resolve_cb(otError error, const otDnsAddressResponse *response, void *context)
{
	const struct shell *sh = (const struct shell *)context;
	char hostname[128];
	otError getter_err;
	uint16_t index = 0;
	otIp6Address address;
	uint32_t ttl;
	char addr_string[NET_IPV6_ADDR_LEN];

	if (error == OT_ERROR_NONE) {
		getter_err = otDnsAddressResponseGetHostName(response, hostname, sizeof(hostname));

		if (getter_err == OT_ERROR_NONE) {
			shell_print(sh, "Hostname %s", hostname);
		} else {
			shell_error(sh, "Hostname error: %u", getter_err);
			return;
		}

		while (otDnsAddressResponseGetAddress(response, index++, &address, &ttl) ==
		       OT_ERROR_NONE) {
			if (!net_addr_ntop(AF_INET6, (struct in6_addr *)&address, addr_string,
					   sizeof(addr_string))) {
				shell_error(sh, "Failed to convert the IPv6 address");
				return;
			}

			shell_print(sh, "Address #%u: %s ttl: %u", index, addr_string, ttl);
		}
	} else {
		shell_error(sh, "DNS resolve callback error: %u", error);
	}
}

static int cmd_dns_client_resolve(const struct shell *sh, size_t argc, char *argv[])
{
	otError error;
	const otDnsQueryConfig *config;

	config = otDnsClientGetDefaultConfig(NULL);

	if (net_addr_pton(AF_INET6, argv[2],
			  (struct in6_addr *)&config->mServerSockAddr.mAddress)) {
		shell_error(sh, "Invalid server address");
		return -EINVAL;
	}

	if (strcmp(argv[0], "test_dns_client_resolve4") == 0) {
		error = otDnsClientResolveIp4Address(NULL, argv[1], dns_resolve_cb, (void *)sh,
						     config);
	} else {
		error = otDnsClientResolveAddress(NULL, argv[1], dns_resolve_cb, (void *)sh,
						  config);
	}

	shell_print(sh, "Resolve requested, err: %u", error);

	return 0;
}

static void print_dns_service_info(const struct shell *sh, const otDnsServiceInfo *service_info)
{
	char addr[INET6_ADDRSTRLEN];

	net_addr_ntop(AF_INET6, (struct in6_addr *)&service_info->mHostAddress, addr, sizeof(addr));

	shell_print(sh, "Port:%d, Priority:%d, Weight:%d, TTL:%u", service_info->mPort,
		    service_info->mPriority, service_info->mWeight, service_info->mTtl);
	shell_print(sh, "Host: %s", service_info->mHostNameBuffer);
	shell_print(sh, "Address: %s", addr);
	shell_print(sh, "Address TTL: %u", service_info->mHostAddressTtl);
	shell_print(sh, "TXT:");

	shell_hexdump(sh, service_info->mTxtData, service_info->mTxtDataSize);

	shell_print(sh, "TXT TTL: %u", service_info->mTxtDataTtl);
}


static void dns_service_cb(otError error, const otDnsServiceResponse *response, void *context)
{
	const struct shell *sh = (const struct shell *)context;
	char name[OT_DNS_MAX_NAME_SIZE];
	char label[OT_DNS_MAX_LABEL_SIZE];
	uint8_t txtBuffer[CONFIG_OPENTHREAD_RPC_DNS_MAX_TXT_DATA_SIZE];
	otDnsServiceInfo service_info;

	otDnsServiceResponseGetServiceName(response, label, sizeof(label), name, sizeof(name));

	if (error == OT_ERROR_NONE) {
		shell_print(sh, "DNS service resolution response for %s for service %s", label,
			    name);

		service_info.mHostNameBuffer = name;
		service_info.mHostNameBufferSize = sizeof(name);
		service_info.mTxtData = txtBuffer;
		service_info.mTxtDataSize = sizeof(txtBuffer);

		if (otDnsServiceResponseGetServiceInfo(response, &service_info) == OT_ERROR_NONE) {
			print_dns_service_info(sh, &service_info);
		}
	} else {
		shell_error(sh, "DNS service resolution error: %u", error);
	}
}

static int cmd_dns_client_service(const struct shell *sh, size_t argc, char *argv[])
{
	const otDnsQueryConfig *config;
	otError error;

	config = otDnsClientGetDefaultConfig(NULL);

	if (net_addr_pton(AF_INET6, argv[3],
			  (struct in6_addr *)&config->mServerSockAddr.mAddress)) {
		shell_error(sh, "Invalid server address");
		return -EINVAL;
	}

	if (strcmp(argv[0], "test_dns_client_servicehost") == 0) {
		error = otDnsClientResolveService(NULL, argv[1], argv[2], dns_service_cb,
						 (void *)sh, config);
	} else {
		error = otDnsClientResolveServiceAndHostAddress(NULL, argv[1], argv[2],
								dns_service_cb, (void *)sh,
								config);
	}

	shell_print(sh, "Service resolve requested, err: %u", error);

	return 0;
}

static void dns_browse_cb(otError error, const otDnsBrowseResponse *response, void *context)
{
	const struct shell *sh = (const struct shell *)context;
	char name[OT_DNS_MAX_NAME_SIZE];
	char label[OT_DNS_MAX_LABEL_SIZE];
	uint8_t txtBuffer[CONFIG_OPENTHREAD_RPC_DNS_MAX_TXT_DATA_SIZE];
	otDnsServiceInfo info;

	otDnsBrowseResponseGetServiceName(response, name, sizeof(name));

	shell_print(sh, "DNS browse response for: %s", name);

	if (error == OT_ERROR_NONE) {
		uint16_t index = 0;

		while (otDnsBrowseResponseGetServiceInstance(response, index++, label,
							     sizeof(label)) == OT_ERROR_NONE) {
			shell_print(sh, "%s", label);

			info.mHostNameBuffer     = name;
			info.mHostNameBufferSize = sizeof(name);
			info.mTxtData            = txtBuffer;
			info.mTxtDataSize        = sizeof(txtBuffer);

			if (otDnsBrowseResponseGetServiceInfo(response, label, &info) ==
			    OT_ERROR_NONE) {
				print_dns_service_info(sh, &info);
			}
		}
	}
}

static int cmd_dns_client_browse(const struct shell *sh, size_t argc, char *argv[])
{
	const otDnsQueryConfig *config;
	otError error;

	config = otDnsClientGetDefaultConfig(NULL);

	if (net_addr_pton(AF_INET6, argv[2],
			  (struct in6_addr *)&config->mServerSockAddr.mAddress)) {
		shell_error(sh, "Invalid server address");
		return -EINVAL;
	}

	error = otDnsClientBrowse(NULL, argv[1], dns_browse_cb, (void *)sh, config);

	shell_print(sh, "Browse requested, err: %u", error);

	return 0;
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
	SHELL_CMD_ARG(test_srp_client_autostart, NULL, "Test SRP client autostart API",
		      cmd_test_srp_client_autostart, 1, 1),
	SHELL_CMD_ARG(test_srp_client_host_info, NULL, "Test SRP client host info API",
		      cmd_test_srp_client_host_info, 1, 0),
	SHELL_CMD_ARG(test_srp_client_host_name, NULL, "Test SRP client host name setting API",
		      cmd_test_srp_client_host_name, 2, 0),
	SHELL_CMD_ARG(test_srp_client_host_remove, NULL, "Test SRP client host removal API",
		      cmd_test_srp_client_host_remove, 1, 2),
	SHELL_CMD_ARG(test_srp_client_host_clear, NULL, "Test SRP client host clear API",
		      cmd_test_srp_client_host_clear, 1, 0),
	SHELL_CMD_ARG(test_srp_client_key_lease_interval, NULL,
		      "Test SRP client key lease interval API",
		      cmd_test_srp_client_key_lease_interval, 1, 1),
	SHELL_CMD_ARG(test_srp_client_lease_interval, NULL, "Test SRP client lease interval API",
		      cmd_test_srp_client_lease_interval, 1, 1),
	SHELL_CMD_ARG(test_srp_client_server, NULL, "Test SRP client server info API",
		      cmd_test_srp_client_server, 1, 0),
	SHELL_CMD_ARG(test_srp_client_start, NULL, "Test SRP client start API",
		      cmd_test_srp_client_start, 3, 0),
	SHELL_CMD_ARG(test_srp_client_state, NULL, "Test SRP client state API",
		      cmd_test_srp_client_state, 1, 0),
	SHELL_CMD_ARG(test_srp_client_stop, NULL, "Test SRP client stop API",
		      cmd_test_srp_client_stop, 1, 0),
	SHELL_CMD_ARG(test_srp_client_ttl, NULL, "Test SRP client TTL get/set API",
		      cmd_test_srp_client_ttl, 1, 1),
	SHELL_CMD_ARG(test_srp_client_services, NULL, "Test SRP client service list API",
		      cmd_test_srp_client_services, 1, 0),
	SHELL_CMD_ARG(test_srp_client_add_service_add, NULL, "Test SRP client service add API",
		      cmd_test_srp_client_service_add, 6, MAX_SUBTYPES + MAX_TXT_ENTRIES),
	SHELL_CMD_ARG(cmd_test_srp_client_service_clear, NULL, "Test SRP client service clear API",
		      cmd_test_srp_client_service_clear, 2, 0),
	SHELL_CMD_ARG(test_srp_client_add_service_remove, NULL,
		      "Test SRP client service remove API",
		      cmd_test_srp_client_service_remove, 2, 0),
	SHELL_CMD_ARG(test_srp_client_host_address_auto, NULL,
		      "Test SRP client host address auto API",
		      cmd_test_srp_client_host_address_auto, 1, 0),
	SHELL_CMD_ARG(test_srp_client_host_addresses, NULL, "Test SRP client host addresses API",
		      cmd_test_srp_client_host_addresses, 2, MAX_HOST_ADDRESSES - 1),
	SHELL_CMD_ARG(test_dns_client_config_get, NULL, "Get default config",
		      cmd_dns_client_config_get, 1, 0),
	SHELL_CMD_ARG(test_dns_client_config_set, NULL,
		      "Set default config, args:\n"
		      "\t<address> <port> <timeout> <attempts> <recursion>"
		      " <nat mode> <service_mode> <transport>",
		      cmd_dns_client_config_set, 3, 6),
	SHELL_CMD_ARG(test_dns_client_resolve, NULL, "Resolve IPv6 address, args: <name> <server>",
		      cmd_dns_client_resolve, 3, 0),
	SHELL_CMD_ARG(test_dns_client_resolve4, NULL, "Resolve IPv4 address, args: <name> <server>",
		      cmd_dns_client_resolve, 3, 0),
	SHELL_CMD_ARG(test_dns_client_service, NULL, "Service instance resolution, args: <instance>"
						     "<service> <server>",
		      cmd_dns_client_service, 4, 0),
	SHELL_CMD_ARG(test_dns_client_servicehost, NULL, "Service instance resolution, args:"
							 " <instance> <service> <server>",
		      cmd_dns_client_service, 4, 0),
	SHELL_CMD_ARG(test_dns_client_browse, NULL, "Service browsing, args <service> <server>",
		      cmd_dns_client_browse, 3, 0),
	SHELL_CMD_ARG(test_vendor_data, NULL, "Vendor data, args: <vendor-name> <vendor-model>"
					      " <vendor-sw-version>",
		      cmd_test_vendor_data, 4, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(ot, &ot_cmds,
		       "OpenThread subcommands\n"
		       "Use \"ot help\" to get the list of subcommands",
		       cmd_ot, 2, 255);

void ot_shell_server_restarted(void)
{
	ot_cli_is_initialized = false;
}

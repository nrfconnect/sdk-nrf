/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_shell.h"
#include <net/ot_rpc.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/shell/shell.h>

#include <openthread/cli.h>
#include <openthread/coap.h>
#include <openthread/dns_client.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/link_raw.h>
#include <openthread/link.h>
#include <openthread/mesh_diag.h>
#include <openthread/message.h>
#include <openthread/netdata.h>
#include <openthread/netdiag.h>
#include <openthread/srp_client.h>
#include <openthread/thread.h>
#include <openthread/udp.h>

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
static bool udp_tx_callback_enabled;
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
 * Executes the local shell command handler with the provided arguments.
 * Then it prints either "Done", if the handler returns no error, or "Error X", where X is
 * an OpenThread error code returned by the handler.
 *
 * This is meant to duplicate the behavior of the OpenThread's CLI subsystem, which prints
 * the command result in a similar manner.
 */
static int ot_cli_command_exec(ot_cli_command_handler_t handler, const struct shell *sh,
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

	for (size_t i = 0; i < argc; i++) {
		int arg_len = snprintk(ptr, end - ptr, "%s", argv[i]);

		if (arg_len < 0) {
			return arg_len;
		}

		ptr += arg_len;

		if (ptr >= end) {
			shell_warn(sh, "OT shell buffer full");
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
	return ot_cli_command_exec(ot_cli_command_discover, sh, argc, argv);
}

static otError cmd_radio_impl(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc != 2 || strcmp(argv[1], "time") != 0) {
		return OT_ERROR_INVALID_COMMAND;
	}

	shell_print(sh, "%" PRIu64, otLinkRawGetRadioTime(NULL));

	return OT_ERROR_NONE;
}

static int cmd_radio(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 2 && strcmp(argv[1], "time") == 0) {
		return ot_cli_command_exec(cmd_radio_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc, argv);
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
			otIp6AddressToString(&addr->mAddress, addr_string, sizeof(addr_string));
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
	return ot_cli_command_exec(ot_cli_command_ifconfig, sh, argc, argv);
}

static int cmd_ipmaddr(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(ot_cli_command_ipmaddr, sh, argc, argv);
}

static int cmd_mode(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(ot_cli_command_mode, sh, argc, argv);
}

static int cmd_pollperiod(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(ot_cli_command_pollperiod, sh, argc, argv);
}

static int cmd_state(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc > 1) {
		/*
		 * Serialization of OT APIs for enforcing the role is to be done,
		 * so send the command to be processed by the RPC server.
		 */
		return ot_cli_command_send(sh, argc, argv);
	}

	return ot_cli_command_exec(ot_cli_command_state, sh, argc, argv);
}

static int cmd_thread(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(ot_cli_command_thread, sh, argc, argv);
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
	return ot_cli_command_send(sh, argc - 1, argv + 1);
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
	otError error;

	memset(srp_client_host_name, 0, sizeof(srp_client_host_name));
	strncpy(srp_client_host_name, argv[1], sizeof(srp_client_host_name) - 1);

	error = otSrpClientSetHostName(NULL, srp_client_host_name);
	if (error) {
		shell_error(sh, "otSrpClientSetHostName() error: %u", error);
		return -ENOEXEC;
	}

	shell_print(sh, "Name set to: %s", argv[1]);

	return 0;
}

static int cmd_test_srp_client_host_address_auto(const struct shell *sh, size_t argc, char *argv[])
{
	otError error = otSrpClientEnableAutoHostAddress(NULL);

	if (error) {
		shell_error(sh, "otSrpClientEnableAutoHostAddress() error: %u", error);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_test_srp_client_host_remove(const struct shell *sh, size_t argc, char *argv[])
{
	bool remove_key_lease = false;
	bool send_unreg = false;
	int err = 0;
	otError error;

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

	error = otSrpClientRemoveHostAndServices(NULL, remove_key_lease, send_unreg);
	if (error) {
		shell_error(sh, "otSrpClientRemoveHostAndServices() error: %u", error);
		return -ENOEXEC;
	}

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
	otError error;

	if (net_addr_pton(AF_INET6, argv[1], (struct in6_addr *)&sockaddr.mAddress)) {
		shell_error(sh, "Invalid IPv6 address: %s", argv[1]);

		return -EINVAL;
	}

	sockaddr.mPort = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_error(sh, "Invalid port: %s", argv[2]);

		return -EINVAL;
	}

	error = otSrpClientStart(NULL, &sockaddr);
	if (error) {
		shell_error(sh, "otSrpClientStart() error: %u", error);
		return -ENOEXEC;
	}

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
	otError error;

	error = otThreadSetVendorName(NULL, argv[1]);
	if (error) {
		shell_error(sh, "otThreadSetVendorName() error: %u", error);
		return -EINVAL;
	}

	error = otThreadSetVendorModel(NULL, argv[2]);
	if (error) {
		shell_error(sh, "otThreadSetVendorModel() error: %u", error);
		return -EINVAL;
	}

	error = otThreadSetVendorSwVersion(NULL, argv[3]);
	if (error) {
		shell_error(sh, "otThreadSetVendorSwVersion() error: %u", error);
		return -EINVAL;
	}

	shell_print(sh, "Vendor name set to: %s", otThreadGetVendorName(NULL));
	shell_print(sh, "Vendor model set to: %s", otThreadGetVendorModel(NULL));
	shell_print(sh, "Vendor SW version set to: %s", otThreadGetVendorSwVersion(NULL));

	return 0;
}

static void handle_receive_diagnostic_get(otError error, otMessage *message,
					  const otMessageInfo *message_info, void *context)
{
	otNetworkDiagTlv diagTlv;
	otNetworkDiagIterator iterator = OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT;

	const struct shell *sh = (const struct shell *)context;
	char addr_string[NET_IPV6_ADDR_LEN];

	if (error != OT_ERROR_NONE) {
		shell_error(sh, "Failed to get the diagnostic response, error: %d", error);
		return;
	}

	if (!net_addr_ntop(AF_INET6, message_info->mPeerAddr.mFields.m8,
			   addr_string, sizeof(addr_string))) {
		shell_error(sh, "Failed to convert the IPv6 address");
		return;
	}

	shell_print(sh, "------------------------------------------------------------------");
	shell_print(sh, "Received DIAG_GET.rsp/ans from %s", addr_string);

	while (otThreadGetNextDiagnosticTlv(message, &iterator, &diagTlv) == OT_ERROR_NONE) {
		shell_fprintf(sh, SHELL_NORMAL, "\nTLV type: 0x%x ", diagTlv.mType);
		switch (diagTlv.mType) {
		case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
			shell_print(sh, "(MAC Extended Address TLV)");
			shell_hexdump(sh, diagTlv.mData.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_EUI64:
			shell_print(sh, "(EUI64 TLV)");
			shell_hexdump(sh, diagTlv.mData.mEui64.m8, OT_EXT_ADDRESS_SIZE);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
			shell_print(sh, "(Mode TLV)");
			shell_print(sh, "RX on when idle: %s", diagTlv.mData.mMode.mRxOnWhenIdle ?
				    "true" : "false");
			shell_print(sh, "Device type: %s", diagTlv.mData.mMode.mDeviceType ?
				    "true" : "false");
			shell_print(sh, "Network data: %s", diagTlv.mData.mMode.mNetworkData ?
				    "true" : "false");
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
			shell_print(sh, "(Connectivity TLV)");
			shell_print(sh, "Parent priority: %d",
				    diagTlv.mData.mConnectivity.mParentPriority);
			shell_print(sh, "Link quality 3: %u",
				    diagTlv.mData.mConnectivity.mLinkQuality3);
			shell_print(sh, "Link quality 2: %u",
				    diagTlv.mData.mConnectivity.mLinkQuality2);
			shell_print(sh, "Link quality 1: %u",
				    diagTlv.mData.mConnectivity.mLinkQuality1);
			shell_print(sh, "Leader cost: %u",
				    diagTlv.mData.mConnectivity.mLeaderCost);
			shell_print(sh, "ID sequence: %u", diagTlv.mData.mConnectivity.mIdSequence);
			shell_print(sh, "Active routers: %u",
				    diagTlv.mData.mConnectivity.mActiveRouters);
			shell_print(sh, "SED buffer size: %u",
				    diagTlv.mData.mConnectivity.mSedBufferSize);
			shell_print(sh, "SED datagram count: %u",
				    diagTlv.mData.mConnectivity.mSedDatagramCount);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
			shell_print(sh, "(Route64 TLV)");
			shell_print(sh, "ID sequence: %u", diagTlv.mData.mRoute.mIdSequence);
			shell_print(sh, "Route count: %u", diagTlv.mData.mRoute.mRouteCount);

			for (int i = 0; i < diagTlv.mData.mRoute.mRouteCount; i++) {
				shell_print(sh, "\tRouter ID: %u",
					    diagTlv.mData.mRoute.mRouteData[i].mRouterId);
				shell_print(sh, "\tLink quality in: %u",
					    diagTlv.mData.mRoute.mRouteData[i].mLinkQualityIn);
				shell_print(sh, "\tLink quality out: %u",
					    diagTlv.mData.mRoute.mRouteData[i].mLinkQualityOut);
				shell_print(sh, "\tRoute cost: %u\n",
					    diagTlv.mData.mRoute.mRouteData[i].mRouteCost);
			}
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
			shell_print(sh, "(Leader Data TLV)");
			shell_print(sh, "Partition ID: 0x%x",
				    diagTlv.mData.mLeaderData.mPartitionId);
			shell_print(sh, "Weighting: %u", diagTlv.mData.mLeaderData.mWeighting);
			shell_print(sh, "Data version: %u", diagTlv.mData.mLeaderData.mDataVersion);
			shell_print(sh, "Stable data version: %u",
				    diagTlv.mData.mLeaderData.mStableDataVersion);
			shell_print(sh, "Leader router ID: 0x%x",
				    diagTlv.mData.mLeaderData.mLeaderRouterId);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
			shell_print(sh, "(MAC Counters TLV)");
			shell_print(sh, "IfInUnknownProtos: %u",
				    diagTlv.mData.mMacCounters.mIfInUnknownProtos);
			shell_print(sh, "IfInErrors: %u", diagTlv.mData.mMacCounters.mIfInErrors);
			shell_print(sh, "IfOutErrors: %u", diagTlv.mData.mMacCounters.mIfOutErrors);
			shell_print(sh, "IfInUcastPkts: %u",
				    diagTlv.mData.mMacCounters.mIfInUcastPkts);
			shell_print(sh, "IfInBroadcastPkts: %u",
				    diagTlv.mData.mMacCounters.mIfInBroadcastPkts);
			shell_print(sh, "IfInDiscards: %u",
				    diagTlv.mData.mMacCounters.mIfInDiscards);
			shell_print(sh, "IfOutUcastPkts: %u",
				    diagTlv.mData.mMacCounters.mIfOutUcastPkts);
			shell_print(sh, "IfOutBroadcastPkts: %u",
				    diagTlv.mData.mMacCounters.mIfOutBroadcastPkts);
			shell_print(sh, "IfOutDiscards: %u",
				    diagTlv.mData.mMacCounters.mIfOutDiscards);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS:
			shell_print(sh, "(MLE Counters TLV)");
			shell_print(sh, "DisabledRole: %u",
				    diagTlv.mData.mMleCounters.mDisabledRole);
			shell_print(sh, "DetachedRole: %u",
				    diagTlv.mData.mMleCounters.mDetachedRole);
			shell_print(sh, "ChildRole: %u", diagTlv.mData.mMleCounters.mChildRole);
			shell_print(sh, "RouterRole: %u", diagTlv.mData.mMleCounters.mRouterRole);
			shell_print(sh, "LeaderRole: %u", diagTlv.mData.mMleCounters.mLeaderRole);
			shell_print(sh, "AttachAttempts: %u",
				    diagTlv.mData.mMleCounters.mAttachAttempts);
			shell_print(sh, "PartitionIdChanges: %u",
				    diagTlv.mData.mMleCounters.mPartitionIdChanges);
			shell_print(sh, "BetterPartitionAttachAttempts: %u",
				    diagTlv.mData.mMleCounters.mBetterPartitionAttachAttempts);
			shell_print(sh, "ParentChanges: %u",
				    diagTlv.mData.mMleCounters.mParentChanges);
			shell_print(sh, "TrackedTime: %llu",
				    diagTlv.mData.mMleCounters.mTrackedTime);
			shell_print(sh, "DisabledTime: %llu",
				    diagTlv.mData.mMleCounters.mDisabledTime);
			shell_print(sh, "DetachedTime: %llu",
				    diagTlv.mData.mMleCounters.mDetachedTime);
			shell_print(sh, "ChildTime: %llu", diagTlv.mData.mMleCounters.mChildTime);
			shell_print(sh, "RouterTime: %llu", diagTlv.mData.mMleCounters.mRouterTime);
			shell_print(sh, "LeaderTime: %llu", diagTlv.mData.mMleCounters.mLeaderTime);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:
			shell_print(sh, "(Battery Level TLV)");
			shell_print(sh, "Battery level: %u", diagTlv.mData.mBatteryLevel);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT:
			shell_print(sh, "(Timeout TLV)");
			shell_print(sh, "Timeout: %u", diagTlv.mData.mTimeout);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT:
			shell_print(sh, "(Max Child Timeout TLV)");
			shell_print(sh, "Max child timeout: %u", diagTlv.mData.mMaxChildTimeout);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
			shell_print(sh, "(Address16 TLV)");
			shell_print(sh, "Rloc16: 0x%x", diagTlv.mData.mAddr16);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE:
			shell_print(sh, "(Supply Voltage TLV)");
			shell_print(sh, "Supply voltage: %u", diagTlv.mData.mSupplyVoltage);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_VERSION:
			shell_print(sh, "(Thread Version TLV)");
			shell_print(sh, "Version: %u", diagTlv.mData.mVersion);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME:
			shell_print(sh, "(Vendor Name TLV)");
			shell_print(sh, "Vendor name: %s", diagTlv.mData.mVendorName);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL:
			shell_print(sh, "(Vendor Model TLV)");
			shell_print(sh, "Vendor model: %s", diagTlv.mData.mVendorModel);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION:
			shell_print(sh, "(Vendor SW Version TLV)");
			shell_print(sh, "Vendor SW version: %s", diagTlv.mData.mVendorSwVersion);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION:
			shell_print(sh, "(Thread Stack Version TLV)");
			shell_print(sh, "Thread stack version: %s",
				    diagTlv.mData.mThreadStackVersion);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL:
			shell_print(sh, "(Vendor App URL TLV)");
			shell_print(sh, "Vendor app URL: %s", diagTlv.mData.mVendorAppUrl);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:
			shell_print(sh, "(Network Data TLV)");
			shell_hexdump(sh, diagTlv.mData.mNetworkData.m8,
				      diagTlv.mData.mNetworkData.mCount);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES:
			shell_print(sh, "(Channel Pages TLV)");
			shell_hexdump(sh, diagTlv.mData.mChannelPages.m8,
				      diagTlv.mData.mChannelPages.mCount);
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
			shell_print(sh, "(IPv6 Address List TLV)");
			for (int i = 0; i < diagTlv.mData.mIp6AddrList.mCount; i++) {
				shell_hexdump(sh, diagTlv.mData.mIp6AddrList.mList[i].mFields.m8,
					      OT_IP6_ADDRESS_SIZE);
			}
			break;
		case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:
			shell_print(sh, "(Child Table TLV)");
			for (int i = 0; i < diagTlv.mData.mChildTable.mCount; i++) {
				shell_print(sh, "\tChild ID: %u",
					    diagTlv.mData.mChildTable.mTable[i].mChildId);
				shell_print(sh, "\tTimeout: %u",
					    diagTlv.mData.mChildTable.mTable[i].mTimeout);
				shell_print(sh, "\tLink quality: %u",
					    diagTlv.mData.mChildTable.mTable[i].mLinkQuality);
				shell_print(sh, "\tRX on when idle: %s",
					    diagTlv.mData.mChildTable.mTable[i].mMode.mRxOnWhenIdle
					    ? "true" : "false");
				shell_print(sh, "\tDevice type: %s",
					    diagTlv.mData.mChildTable.mTable[i].mMode.mDeviceType ?
					    "true" : "false");
				shell_print(sh, "\tNetwork data: %s",
					    diagTlv.mData.mChildTable.mTable[i].mMode.mNetworkData ?
					    "true" : "false");
			}
			break;

		default:
			shell_print(sh, "(Unknown TLV)");
		}
	}
}

static int cmd_test_net_diag(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t tlv_types[35];
	uint8_t count = 0;
	otIp6Address addr;
	otError error;

	if (net_addr_pton(AF_INET6, argv[2], addr.mFields.m8)) {
		shell_error(sh, "Failed to parse IPv6 address: %s", argv[1]);
		return -EINVAL;
	}

	for (int arg = 3; arg < argc; ++arg) {
		tlv_types[count] = shell_strtoul(argv[arg], 0, NULL);
		count++;
	}

	if (strcmp(argv[1], "get") == 0) {
		error = otThreadSendDiagnosticGet(NULL, &addr, tlv_types, count,
						  handle_receive_diagnostic_get, (void *)sh);
		if (error) {
			shell_error(sh, "otThreadSendDiagnosticGet() error: %u", error);
			return -ENOEXEC;
		}

	} else if (strcmp(argv[1], "reset") == 0) {
		error = otThreadSendDiagnosticReset(NULL, &addr, tlv_types, count);
		if (error) {
			shell_error(sh, "otThreadSendDiagnosticGet() error: %u", error);
			return -ENOEXEC;
		}
	} else {
		shell_error(sh, "Invalid argument %s", argv[1]);
		return -EINVAL;
	}

	return 0;
}

static void handle_mesh_diag_discover(otError error, otMeshDiagRouterInfo *router_info,
				      void *context)
{
	const struct shell *sh = (const struct shell *)context;

	if (error != OT_ERROR_NONE && error != OT_ERROR_PENDING) {
		shell_error(sh, "Error in mesh diag discovery :%d", error);
		return;
	}

	shell_print(sh, "------------------------------------------------------------------");
	shell_print(sh, "Received mesh diagnostic response from Router ID: %u",
		    router_info->mRouterId);
	shell_print(sh, "Rloc16: 0x%x", router_info->mRloc16);
	shell_print(sh, "Extended Address:");
	shell_hexdump(sh, router_info->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
	if (router_info->mVersion != OT_MESH_DIAG_VERSION_UNKNOWN) {
		shell_print(sh, "Version: %u", router_info->mVersion);
	}
	shell_print(sh, "IsThisDeviceParent: %s",
		    router_info->mIsThisDeviceParent ? "true" : "false");
	shell_print(sh, "IsLeader: %s", router_info->mIsLeader ? "true" : "false");
	shell_print(sh, "IsThisDevice: %s", router_info->mIsThisDevice ? "true" : "false");
	shell_print(sh, "IsBorderRouter:  %s", router_info->mIsBorderRouter ? "true" : "false");

	for (int i = 0; i <= OT_NETWORK_MAX_ROUTER_ID; i++) {
		if (router_info->mLinkQualities[i]) {
			shell_print(sh, "Link quality to Router %u: %u", i,
				    router_info->mLinkQualities[i]);
		}
	}

	if (router_info->mIp6AddrIterator) {
		otIp6Address address;

		error = otMeshDiagGetNextIp6Address(router_info->mIp6AddrIterator, &address);
		if (error == OT_ERROR_NONE) {
			shell_print(sh, "\nIPv6 addresses:");
			do {
				shell_hexdump(sh, address.mFields.m8, OT_IP6_ADDRESS_SIZE);
				error = otMeshDiagGetNextIp6Address(router_info->mIp6AddrIterator,
								    &address);

			} while (error == OT_ERROR_NONE);
		}
	}

	if (router_info->mChildIterator) {
		otMeshDiagChildInfo child_info;

		error = otMeshDiagGetNextChildInfo(router_info->mChildIterator, &child_info);
		if (error == OT_ERROR_NONE) {
			shell_print(sh, "\nChildren:");
			do {
				shell_print(sh, "\tRloc16 0x%x", child_info.mRloc16);
				shell_print(sh, "\tLink quality: %u", child_info.mLinkQuality);
				shell_print(sh, "\tRxOnWhenIdle: %s",
					    child_info.mMode.mRxOnWhenIdle ? "true" : "false");
				shell_print(sh, "\tDeviceType: %s",
					    child_info.mMode.mDeviceType ? "true" : "false");
				shell_print(sh, "\tNetworkData: %s",
					    child_info.mMode.mNetworkData ? "true" : "false");
				shell_print(sh, "\tIsThisDevice: %s",
					    child_info.mIsThisDevice ? "true" : "false");
				shell_print(sh, "\tIsBorderRouter: %s\n",
					    child_info.mIsBorderRouter ? "true" : "false");
				error = otMeshDiagGetNextChildInfo(router_info->mChildIterator,
								   &child_info);
			} while (error == OT_ERROR_NONE);
		}
	}
}

static int cmd_test_mesh_diag(const struct shell *sh, size_t argc, char *argv[])
{
	otMeshDiagDiscoverConfig config = {
		.mDiscoverIp6Addresses = false,
		.mDiscoverChildTable = false,
	};
	otError error;

	if (strcmp(argv[1], "cancel") == 0) {
		otMeshDiagCancel(NULL);
		return 0;
	} else if (strcmp(argv[1], "start") != 0) {
		shell_error(sh, "Invalid argument %s", argv[1]);
		return -EINVAL;
	}

	for (int i = 2; i < argc; i++) {
		if (strcmp(argv[i], "ip6") == 0) {
			config.mDiscoverIp6Addresses = true;
		} else if (strcmp(argv[i], "children") == 0) {
			config.mDiscoverChildTable = true;
		} else {
			shell_error(sh, "Invalid argument %s", argv[i]);
			return -EINVAL;
		}
	}

	error = otMeshDiagDiscoverTopology(NULL, &config, handle_mesh_diag_discover, (void *)sh);
	if (error) {
		shell_error(sh, "otMeshDiagDiscoverTopology() error: %u", error);
		return -ENOEXEC;
	}

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

	if (otDnsServiceResponseGetServiceName(response, label, sizeof(label), name,
					       sizeof(name))) {
		shell_error(sh, "otDnsServiceResponseGetServiceName() error");
		return;
	}

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

	if (otDnsBrowseResponseGetServiceName(response, name, sizeof(name))) {
		shell_error(sh, "otDnsBrowseResponseGetServiceName() error");
		return;
	}

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

static otError ot_cli_command_eui64(const struct shell *sh, size_t argc, char *argv[])
{
	otExtAddress ext_addr;

	if (argc == 1) {
		/* Read current EUI64 */
		otLinkGetFactoryAssignedIeeeEui64(NULL, &ext_addr);
		shell_print(sh, "%02x%02x%02x%02x%02x%02x%02x%02x", ext_addr.m8[0], ext_addr.m8[1],
			    ext_addr.m8[2], ext_addr.m8[3], ext_addr.m8[4], ext_addr.m8[5],
			    ext_addr.m8[6], ext_addr.m8[7]);
		return OT_ERROR_NONE;
	}

	/* Set new EUI64 */
	if (hex2bin(argv[1], strlen(argv[1]), ext_addr.m8, sizeof(ext_addr.m8)) !=
	    sizeof(ext_addr.m8)) {
		return OT_ERROR_INVALID_ARGS;
	}

	return ot_rpc_set_factory_assigned_ieee_eui64(&ext_addr);
}

static int cmd_eui64(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(ot_cli_command_eui64, sh, argc, argv);
}

static otError cmd_rloc16_impl(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "%04x", otThreadGetRloc16(NULL));

	return OT_ERROR_NONE;
}

static int cmd_rloc16(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(cmd_rloc16_impl, sh, argc, argv);
}

static otError cmd_factoryreset_impl(const struct shell *sh, size_t argc, char *argv[])
{
	otInstanceFactoryReset(NULL);

	return OT_ERROR_NONE;
}

static int cmd_factoryreset(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(cmd_factoryreset_impl, sh, argc, argv);
}

static void handle_udp_receive(void *context, otMessage *msg, const otMessageInfo *msg_info)
{
	uint16_t offset;
	uint16_t length;
	char addr_string[OT_IP6_ADDRESS_STRING_SIZE];
	otThreadLinkInfo link_info;
	const struct shell *sh = context;

	offset = otMessageGetOffset(msg);
	length = otMessageGetLength(msg);
	otIp6AddressToString(&msg_info->mPeerAddr, addr_string, sizeof(addr_string));

	shell_print(sh, "%d bytes from %s %d", length - offset, addr_string, msg_info->mPeerPort);

	while (offset < length) {
		uint8_t buf[64];
		uint16_t read = otMessageRead(msg, offset, buf, sizeof(buf));

		shell_hexdump(sh, buf, read);
		offset += read;
	}

	if (otMessageGetThreadLinkInfo(msg, &link_info) == OT_ERROR_NONE) {
		shell_print(sh, "PanId: %x", link_info.mPanId);
		shell_print(sh, "Channel: %u", link_info.mChannel);
		shell_print(sh, "Rss: %d", link_info.mRss);
		shell_print(sh, "Lqi: %u", link_info.mLqi);
		shell_print(sh, "LinkSecurity: %c", link_info.mLinkSecurity ? 'y' : 'n');
	}
}

static otError cmd_udp_open_impl(const struct shell *sh, size_t argc, char *argv[])
{
	if (otUdpIsOpen(NULL, &udp_socket)) {
		return OT_ERROR_ALREADY;
	}

	return otUdpOpen(NULL, &udp_socket, handle_udp_receive, (void *)sh);
}

static int cmd_udp_open(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(cmd_udp_open_impl, sh, argc, argv);
}

static otError cmd_udp_bind_impl(const struct shell *sh, size_t argc, char *argv[])
{
	otNetifIdentifier netif;
	otSockAddr sock_addr;
	int rc = 0;

	if (strcmp(argv[1], "-u") == 0) {
		netif = OT_NETIF_UNSPECIFIED;
	} else if (strcmp(argv[1], "-b") == 0) {
		netif = OT_NETIF_BACKBONE;
	} else {
		netif = OT_NETIF_THREAD;
	}

	if (net_addr_pton(AF_INET6, argv[argc - 2], (struct in6_addr *)&sock_addr.mAddress)) {
		return OT_ERROR_INVALID_ARGS;
	}

	sock_addr.mPort = shell_strtoul(argv[argc - 1], 0, &rc);

	if (rc) {
		return OT_ERROR_INVALID_ARGS;
	}

	return otUdpBind(NULL, &udp_socket, &sock_addr, netif);
}

static int cmd_udp_bind(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(cmd_udp_bind_impl, sh, argc, argv);
}

static otError cmd_udp_connect_impl(const struct shell *sh, size_t argc, char *argv[])
{
	otSockAddr sock_addr;
	int rc = 0;

	if (net_addr_pton(AF_INET6, argv[1], (struct in6_addr *)&sock_addr.mAddress)) {
		return OT_ERROR_INVALID_ARGS;
	}

	sock_addr.mPort = shell_strtoul(argv[2], 0, &rc);

	if (rc) {
		return OT_ERROR_INVALID_ARGS;
	}

	return otUdpConnect(NULL, &udp_socket, &sock_addr);
}

static int cmd_udp_connect(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(cmd_udp_connect_impl, sh, argc, argv);
}

static void udp_tx_callback(const otMessage *msg, otError error, void *ctx)
{
	struct shell *sh = ctx;

	shell_print(sh, "UDP tx result: %d", error);
}

static otError cmd_udp_send_impl(const struct shell *sh, size_t argc, char *argv[])
{
	otError error;
	otMessageInfo msg_info;
	otMessageSettings msg_settings = {
		.mLinkSecurityEnabled = true,
		.mPriority = OT_MESSAGE_PRIORITY_NORMAL,
	};
	otMessage *msg;
	int rc = 0;

	if (!otUdpIsOpen(NULL, &udp_socket)) {
		return OT_ERROR_INVALID_STATE;
	}

	memset(&msg_info, 0, sizeof(msg_info));

	if (argc >= 4) {
		if (net_addr_pton(AF_INET6, argv[1], (struct in6_addr *)&msg_info.mPeerAddr)) {
			return OT_ERROR_INVALID_ARGS;
		}

		msg_info.mPeerPort = shell_strtoul(argv[2], 0, &rc);
		if (rc) {
			return OT_ERROR_INVALID_ARGS;
		}
	}

	msg = otUdpNewMessage(NULL, &msg_settings);
	if (msg == NULL) {
		return OT_ERROR_NO_BUFS;
	}

	error = otMessageAppend(msg, argv[argc - 1], strlen(argv[argc - 1]));
	if (error != OT_ERROR_NONE) {
		otMessageFree(msg);
		return error;
	}

	if (udp_tx_callback_enabled) {
		otMessageRegisterTxCallback(msg, udp_tx_callback, (void *)sh);
	}

	error = otUdpSend(NULL, &udp_socket, msg, &msg_info);
	if (error != OT_ERROR_NONE) {
		otMessageFree(msg);
	}

	return error;
}

static int cmd_udp_send(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(cmd_udp_send_impl, sh, argc, argv);
}

static otError cmd_udp_close_impl(const struct shell *sh, size_t argc, char *argv[])
{
	return otUdpClose(NULL, &udp_socket);
}

static int cmd_udp_close(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(cmd_udp_close_impl, sh, argc, argv);
}

static otError cmd_udp_txcallback_impl(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		shell_print(sh, "%s", udp_tx_callback_enabled ? "enabled" : "disabled");
	} else if (strcmp(argv[1], "enable") == 0) {
		udp_tx_callback_enabled = true;
	} else if (strcmp(argv[1], "disable") == 0) {
		udp_tx_callback_enabled = false;
	} else {
		return OT_ERROR_INVALID_ARGS;
	}

	return OT_ERROR_NONE;
}

static int cmd_udp_txcallback(const struct shell *sh, size_t argc, char *argv[])
{
	return ot_cli_command_exec(cmd_udp_txcallback_impl, sh, argc, argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	udp_cmds, SHELL_CMD_ARG(open, NULL, "Open socket", cmd_udp_open, 1, 0),
	SHELL_CMD_ARG(bind, NULL, "Bind socket [-u|-b] <addr> <port>", cmd_udp_bind, 3, 1),
	SHELL_CMD_ARG(connect, NULL, "Connect socket <addr> <port>", cmd_udp_connect, 3, 0),
	SHELL_CMD_ARG(send, NULL, "Send message [addr port] <message>", cmd_udp_send, 2, 2),
	SHELL_CMD_ARG(close, NULL, "Close socket", cmd_udp_close, 1, 0),
	SHELL_CMD_ARG(txcallback, NULL, "Enable/disable tx callback", cmd_udp_txcallback, 1, 1),
	SHELL_SUBCMD_SET_END);

static otError cmd_channel_impl(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "%u", otLinkGetChannel(NULL));
	return OT_ERROR_NONE;
}

static int cmd_channel(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		return ot_cli_command_exec(cmd_channel_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc, argv);
}

static otError cmd_counters_mac_impl(const struct shell *sh, size_t argc, char *argv[])
{
	const otMacCounters *counters = otLinkGetCounters(NULL);

	shell_print(sh, "TxTotal: %u", counters->mTxTotal);

	shell_print(sh, "    TxUnicast: %u", counters->mTxUnicast);
	shell_print(sh, "    TxBroadcast: %u", counters->mTxBroadcast);
	shell_print(sh, "    TxAckRequested: %u", counters->mTxAckRequested);
	shell_print(sh, "    TxAcked: %u", counters->mTxAcked);
	shell_print(sh, "    TxNoAckRequested: %u", counters->mTxNoAckRequested);
	shell_print(sh, "    TxData: %u", counters->mTxData);
	shell_print(sh, "    TxDataPoll: %u", counters->mTxDataPoll);
	shell_print(sh, "    TxBeacon: %u", counters->mTxBeacon);
	shell_print(sh, "    TxBeaconRequest: %u", counters->mTxBeaconRequest);
	shell_print(sh, "    TxOther: %u", counters->mTxOther);
	shell_print(sh, "    TxRetry: %u", counters->mTxRetry);
	shell_print(sh, "    TxErrCca: %u", counters->mTxErrCca);
	shell_print(sh, "    TxErrBusyChannel: %u", counters->mTxErrBusyChannel);
	shell_print(sh, "    TxErrAbort: %u", counters->mTxErrAbort);
	shell_print(sh, "    TxDirectMaxRetryExpiry: %u", counters->mTxDirectMaxRetryExpiry);
	shell_print(sh, "    TxIndirectMaxRetryExpiry: %u", counters->mTxIndirectMaxRetryExpiry);

	shell_print(sh, "RxTotal: %u", counters->mRxTotal);

	shell_print(sh, "    RxUnicast: %u", counters->mRxUnicast);
	shell_print(sh, "    RxBroadcast: %u", counters->mRxBroadcast);
	shell_print(sh, "    RxData: %u", counters->mRxData);
	shell_print(sh, "    RxDataPoll: %u", counters->mRxDataPoll);
	shell_print(sh, "    RxBeacon: %u", counters->mRxBeacon);
	shell_print(sh, "    RxBeaconRequest: %u", counters->mRxBeaconRequest);
	shell_print(sh, "    RxOther: %u", counters->mRxOther);
	shell_print(sh, "    RxAddressFiltered: %u", counters->mRxAddressFiltered);
	shell_print(sh, "    RxDestAddrFiltered: %u", counters->mRxDestAddrFiltered);
	shell_print(sh, "    RxDuplicated: %u", counters->mRxDuplicated);
	shell_print(sh, "    RxErrNoFrame: %u", counters->mRxErrNoFrame);
	shell_print(sh, "    RxErrNoUnknownNeighbor: %u", counters->mRxErrUnknownNeighbor);
	shell_print(sh, "    RxErrInvalidSrcAddr: %u", counters->mRxErrInvalidSrcAddr);
	shell_print(sh, "    RxErrSec: %u", counters->mRxErrSec);
	shell_print(sh, "    RxErrFcs: %u", counters->mRxErrFcs);
	shell_print(sh, "    RxErrOther: %u", counters->mRxErrOther);

	return OT_ERROR_NONE;
}

static int cmd_counters_mac(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		return ot_cli_command_exec(cmd_counters_mac_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc + 1, argv - 1);
}

static otError cmd_counters_mle_impl(const struct shell *sh, size_t argc, char *argv[])
{
	const otMleCounters *counters = otThreadGetMleCounters(NULL);

	shell_print(sh, "Role Disabled: %u", counters->mDisabledRole);
	shell_print(sh, "Role Detached: %u", counters->mDetachedRole);
	shell_print(sh, "Role Child: %u", counters->mChildRole);
	shell_print(sh, "Role Router: %u", counters->mRouterRole);
	shell_print(sh, "Role Leader: %u", counters->mLeaderRole);
	shell_print(sh, "Attach Attempts: %u", counters->mAttachAttempts);
	shell_print(sh, "Partition Id Changes: %u", counters->mPartitionIdChanges);
	shell_print(sh, "Better Partition Attach Attempts: %u",
		    counters->mBetterPartitionAttachAttempts);
	shell_print(sh, "Parent Changes: %u", counters->mParentChanges);
	shell_print(sh, "Time Disabled Milli: %llu", counters->mDisabledTime);
	shell_print(sh, "Time Detached Milli: %llu", counters->mDetachedTime);
	shell_print(sh, "Time Child Milli: %llu", counters->mChildTime);
	shell_print(sh, "Time Router Milli: %llu", counters->mRouterTime);
	shell_print(sh, "Time Leader Milli: %llu", counters->mLeaderTime);
	shell_print(sh, "Time Tracked Milli: %llu", counters->mTrackedTime);

	return OT_ERROR_NONE;
}

static int cmd_counters_mle(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		return ot_cli_command_exec(cmd_counters_mle_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc + 1, argv - 1);
}

SHELL_STATIC_SUBCMD_SET_CREATE(counters_cmds,
			       SHELL_CMD_ARG(mac, NULL, "MAC counters", cmd_counters_mac, 1, 1),
			       SHELL_CMD_ARG(mle, NULL, "MLE counters", cmd_counters_mle, 1, 1),
			       SHELL_SUBCMD_SET_END);

static otError cmd_extaddr_impl(const struct shell *sh, size_t argc, char *argv[])
{
	const otExtAddress *ext_addr = otLinkGetExtendedAddress(NULL);

	shell_print(sh, "%016llx", sys_get_be64(ext_addr->m8));

	return OT_ERROR_NONE;
}

static int cmd_extaddr(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		return ot_cli_command_exec(cmd_extaddr_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc, argv);
}

static otError cmd_networkname_impl(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "%s", otThreadGetNetworkName(NULL));
	return OT_ERROR_NONE;
}

static int cmd_networkname(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		return ot_cli_command_exec(cmd_networkname_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc, argv);
}

static otError cmd_panid_impl(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "0x%04x", otLinkGetPanId(NULL));
	return OT_ERROR_NONE;
}

static int cmd_panid(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		return ot_cli_command_exec(cmd_panid_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc, argv);
}

static otError cmd_partitionid_impl(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "%u", otThreadGetPartitionId(NULL));
	return OT_ERROR_NONE;
}

static int cmd_partitionid(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		return ot_cli_command_exec(cmd_partitionid_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc, argv);
}

static otError cmd_version_impl(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "%s", otGetVersionString());
	return OT_ERROR_NONE;
}

static int cmd_version(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc == 1) {
		return ot_cli_command_exec(cmd_version_impl, sh, argc, argv);
	}

	return ot_cli_command_send(sh, argc, argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	ot_cmds, SHELL_CMD_ARG(channel, NULL, "Channel configuration", cmd_channel, 1, 255),
	SHELL_CMD_ARG(cli, NULL, "Send command as text", cmd_ot, 1, 255),
	SHELL_CMD_ARG(counters, &counters_cmds, "Counters subcommands", NULL, 1, 0),
	SHELL_CMD_ARG(discover, NULL, "Thread discovery scan", cmd_discover, 1, 4),
	SHELL_CMD_ARG(eui64, NULL, "EUI64 configuration", cmd_eui64, 1, 1),
	SHELL_CMD_ARG(extaddr, NULL, "Ext address configuration", cmd_extaddr, 1, 1),
	SHELL_CMD_ARG(factoryreset, NULL, "Factory reset", cmd_factoryreset, 1, 0),
	SHELL_CMD_ARG(ifconfig, NULL, "Interface management", cmd_ifconfig, 1, 1),
	SHELL_CMD_ARG(ipmaddr, NULL, "IPv6 multicast configuration", cmd_ipmaddr, 1, 2),
	SHELL_CMD_ARG(mode, NULL, "Mode configuration", cmd_mode, 1, 1),
	SHELL_CMD_ARG(networkname, NULL, "Network name configuration", cmd_networkname, 1, 1),
	SHELL_CMD_ARG(panid, NULL, "PAN ID configuration", cmd_panid, 1, 1),
	SHELL_CMD_ARG(partitionid, NULL, "Partition configuration", cmd_partitionid, 1, 2),
	SHELL_CMD_ARG(pollperiod, NULL, "Polling configuration", cmd_pollperiod, 1, 1),
	SHELL_CMD_ARG(radio, NULL, "Radio configuration", cmd_radio, 1, 1),
	SHELL_CMD_ARG(rloc16, NULL, "Get RLOC16", cmd_rloc16, 1, 0),
	SHELL_CMD_ARG(state, NULL, "Current role", cmd_state, 1, 1),
	SHELL_CMD_ARG(thread, NULL, "Role management", cmd_thread, 2, 0),
	SHELL_CMD_ARG(udp, &udp_cmds, "UDP subcommands", NULL, 1, 0),
	SHELL_CMD_ARG(version, NULL, "Get version", cmd_version, 1, 1),
	SHELL_CMD_ARG(test_message, NULL, "Test message API", cmd_test_message, 1, 0),
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
		      "Test SRP client service remove API", cmd_test_srp_client_service_remove, 2,
		      0),
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
	SHELL_CMD_ARG(test_dns_client_service, NULL,
		      "Service instance resolution, args: <instance>"
		      "<service> <server>",
		      cmd_dns_client_service, 4, 0),
	SHELL_CMD_ARG(test_dns_client_servicehost, NULL,
		      "Service instance resolution, args:"
		      " <instance> <service> <server>",
		      cmd_dns_client_service, 4, 0),
	SHELL_CMD_ARG(test_dns_client_browse, NULL, "Service browsing, args <service> <server>",
		      cmd_dns_client_browse, 3, 0),
	SHELL_CMD_ARG(test_vendor_data, NULL,
		      "Vendor data, args: <vendor-name> <vendor-model>"
		      " <vendor-sw-version>",
		      cmd_test_vendor_data, 4, 0),
	SHELL_CMD_ARG(test_net_diag, NULL,
		      "Network diag, args: <get|reset> <IPv6-address>"
		      " <tlv-type ...>",
		      cmd_test_net_diag, 4, 255),
	SHELL_CMD_ARG(test_mesh_diag, NULL,
		      "Mesh diag topology discovery, args: <start|cancel>"
		      "[ip6] [children]",
		      cmd_test_mesh_diag, 2, 2),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(ot, &ot_cmds,
		       "OpenThread subcommands\n"
		       "Use \"ot help\" to get the list of subcommands",
		       cmd_ot, 2, 255);

void ot_shell_server_restarted(void)
{
	ot_cli_is_initialized = false;
}

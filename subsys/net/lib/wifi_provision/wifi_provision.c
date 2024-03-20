/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/dhcpv4_server.h>
#include <net/wifi_mgmt_ext.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <net/wifi_credentials.h>
#include <zephyr/smf.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/dns_sd.h>
#include <net/wifi_provision.h>

#include <zephyr/net/http/client.h>
#include <zephyr/net/http/parser.h>

#include "pb_decode.h"
#include "pb_encode.h"
#include "proto/common.pb.h"

LOG_MODULE_REGISTER(wifi_provision, CONFIG_WIFI_PROVISION_LOG_LEVEL);

/* HTTP responses for demonstration */
#define RESPONSE_200 "HTTP/1.1 200 OK\r\n"
#define RESPONSE_400 "HTTP/1.1 400 Bad Request\r\n\r\n"
#define RESPONSE_403 "HTTP/1.1 403 Forbidden\r\n\r\n"
#define RESPONSE_404 "HTTP/1.1 404 Not Found\r\n\r\n"
#define RESPONSE_405 "HTTP/1.1 405 Method Not Allowed\r\n\r\n"
#define RESPONSE_500 "HTTP/1.1 500 Internal Server Error\r\n\r\n"

/* Zephyr NET management events that this module subscribes to. */
#define NET_MGMT_WIFI (NET_EVENT_WIFI_AP_ENABLE_RESULT		| \
		       NET_EVENT_WIFI_AP_DISABLE_RESULT		| \
		       NET_EVENT_WIFI_AP_STA_CONNECTED		| \
		       NET_EVENT_WIFI_SCAN_DONE			| \
		       NET_EVENT_WIFI_SCAN_RESULT		| \
		       NET_EVENT_WIFI_CONNECT_RESULT		| \
		       NET_EVENT_WIFI_AP_STA_DISCONNECTED)

static struct net_mgmt_event_callback net_l2_mgmt_cb;

/* Forward declarations */
static void state_transition_work_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(state_transition_work, state_transition_work_fn);
static int ap_enable(void);

/* Externs */
extern char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len, char *buf, int buflen);

/* Register service */
DNS_SD_REGISTER_TCP_SERVICE(wifi_provision_sd, CONFIG_NET_HOSTNAME, "_http", "local", DNS_SD_EMPTY_TXT, CONFIG_WIFI_PROVISION_TCP_PORT);

/* Declare workqueue */
K_THREAD_STACK_DEFINE(stack_area, 8192);
static struct k_work_q queue;

/* Internal variables */
static ScanResults scan_results = ScanResults_init_zero;
static uint8_t scan_result_buffer[1024];
static size_t scan_result_buffer_len;
static const struct smf_state state[];

/* Semaphore used to block wifi_provision_start() until provisioning has completed. */
static K_SEM_DEFINE(wifi_provision_sem, 0, 1);

/* Variable used to indicated that we have received and stored credentials, used to break
 * out of socket recv.
 */
static bool credentials_stored;

/* Local reference to callers handler, used to send events to the application. */
static wifi_provision_evt_handler_t handler_cb;

/* Zephyr State Machine Framework variables */
static enum module_state {
	UNPROVISIONED,
	PROVISIONING,
	PROVISIONED,
	FINISHED,
	RESET
} state_next;

/* User defined state object used with SMF.
 * Used to transfer data between state changes.
 */
static struct s_object {
	/* This must be first */
	struct smf_ctx ctx;

	char ssid[50];
	char psk[50];
} s_obj;

static struct http_req {
	struct http_parser parser;
	int socket;
	int accepted;
	bool received_all;
	enum http_method method;
	const char *url;
	size_t url_len;
	const char *body;
	size_t body_len;
} request;

static const unsigned char server_certificate[] = {
	#include "server_certificate.pem.inc"
	(0x00)
};

static const unsigned char server_private_key[] = {
	#include "server_private_key.pem.inc"
	(0x00)
};

/* Function to notify application */
static void notify_app(enum wifi_provision_evt_type type)
{
	if (handler_cb) {
		struct wifi_provision_evt evt = {
			.type = type,
		};

		handler_cb(&evt);
	}
}

/* Functions used to schedule state transitions. State transitions are offloaded to a separate
 * workqueue to avoid races with Zephyr NET management. Time consuming tasks can safely be offloaded
 * to this workqueue.
 */
static void state_transition_work_fn(struct k_work *work)
{
	smf_set_state(SMF_CTX(&s_obj), &state[state_next]);
}

static void state_set(enum module_state new_state, k_timeout_t delay)
{
	state_next = new_state;
	k_work_reschedule_for_queue(&queue, &state_transition_work, delay);
}

static void handle_wifi_scan_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;

	if (scan_results.results_count < ARRAY_SIZE(scan_results.results)) {

		/* SSID */
		strncpy(scan_results.results[scan_results.results_count].ssid, entry->ssid, sizeof(entry->ssid));
		scan_results.results[scan_results.results_count].ssid[sizeof(entry->ssid) - 1] = '\0';

		/* BSSID (MAC) */
		net_sprint_ll_addr_buf(entry->mac, WIFI_MAC_ADDR_LEN,
				       scan_results.results[scan_results.results_count].bssid,
				       sizeof(scan_results.results[scan_results.results_count].bssid));
		scan_results.results[scan_results.results_count].bssid[sizeof(entry->mac) - 1] = '\0';

		/* Band */
		scan_results.results[scan_results.results_count].band =
			(entry->band == WIFI_FREQ_BAND_2_4_GHZ) ? Band_BAND_2_4_GHZ :
			(entry->band == WIFI_FREQ_BAND_5_GHZ) ? Band_BAND_5_GHZ :
			(entry->band == WIFI_FREQ_BAND_6_GHZ) ? Band_BAND_6_GHZ :
			Band_BAND_UNSPECIFIED;

		/* Channel */
		scan_results.results[scan_results.results_count].channel = entry->channel;

		/* Auth mode - defaults to AuthMode_WPA_WPA2_PSK. */
			scan_results.results[scan_results.results_count].authMode =
			(entry->security == WIFI_SECURITY_TYPE_NONE) ? AuthMode_OPEN :
			(entry->security == WIFI_SECURITY_TYPE_PSK) ? AuthMode_WPA_WPA2_PSK :
			(entry->security == WIFI_SECURITY_TYPE_PSK_SHA256) ? AuthMode_WPA2_PSK :
			(entry->security == WIFI_SECURITY_TYPE_SAE) ? AuthMode_WPA3_PSK :
			AuthMode_WPA_WPA2_PSK;

		/* Signal strength */
		scan_results.results[scan_results.results_count].rssi = entry->rssi;

		scan_results.results_count++;
	}
}

static void dhcp_server_start(void)
{
	int ret;
	struct in_addr base_addr = { 0 };
	struct net_if *iface = net_if_get_first_wifi();

	/* Set base address for DHCPv4 server based on statically assigned IPv4 address. */
	if (inet_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &base_addr) != 1) {
		LOG_ERR("Failed to set IP address");
		notify_app(WIFI_PROVISION_EVT_FATAL_ERROR);
		return;
	}

	/* Increment base address by 1 to avoid collision with the servers base address. */
	uint32_t base_addr_int = ntohl(base_addr.s_addr);
	base_addr_int += 1;
	base_addr.s_addr = htonl(base_addr_int);

	ret = net_dhcpv4_server_start(iface, &base_addr);
	if (ret) {
		LOG_ERR("Failed to start DHCPv4 server, error: %d", ret);
		notify_app(WIFI_PROVISION_EVT_FATAL_ERROR);
		return;
	}

	LOG_DBG("DHCPv4 server started");
}

static void net_mgmt_wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_AP_ENABLE_RESULT:
		LOG_DBG("NET_EVENT_WIFI_AP_ENABLE_RESULT");

		/* Start DHCP server */
		dhcp_server_start();
		break;
	case NET_EVENT_WIFI_AP_DISABLE_RESULT:
		LOG_DBG("NET_EVENT_WIFI_AP_DISABLE_RESULT");

		state_set(FINISHED, K_SECONDS(1));
		break;
	case NET_EVENT_WIFI_AP_STA_CONNECTED:
		LOG_DBG("NET_EVENT_WIFI_AP_STA_CONNECTED");

		notify_app(WIFI_PROVISION_EVT_CLIENT_CONNECTED);
		break;
	case NET_EVENT_WIFI_AP_STA_DISCONNECTED:
		LOG_DBG("NET_EVENT_WIFI_AP_STA_DISCONNECTED");

		notify_app(WIFI_PROVISION_EVT_CLIENT_DISCONNECTED);
		break;
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		LOG_DBG("NET_EVENT_WIFI_SCAN_DONE");

		/* Scan results cached, waiting for client to request available networks and
		 * provide Wi-Fi credentials.
		 */
		state_set(PROVISIONING, K_SECONDS(1));
		break;
	default:
		break;
	}
}

static int wifi_scan(void)
{
	int ret;
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_scan_params params = { 0 };

	LOG_DBG("Scaning for Wi-Fi networks...");

	/* Clear scan result buffer */
	memset(scan_result_buffer, 0, sizeof(scan_result_buffer));

	ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params, sizeof(params));
	if (ret) {
		LOG_ERR("Failed to start Wi-Fi scan, error: %d", ret);
		return -ENOEXEC;
	}

	return 0;
}

static int ap_enable(void)
{
	int ret;
	struct net_if *iface = net_if_get_first_wifi();
	static struct wifi_connect_req_params params = { 0 };

	params.timeout = SYS_FOREVER_MS;
	params.band = WIFI_FREQ_BAND_UNKNOWN;
	params.channel = WIFI_CHANNEL_ANY;
	params.security = WIFI_SECURITY_TYPE_NONE;
	params.mfp = WIFI_MFP_OPTIONAL;
	params.ssid = CONFIG_WIFI_PROVISION_SSID;
	params.ssid_length = strlen(CONFIG_WIFI_PROVISION_SSID);

	ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &params,
		       sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("Failed to enable AP, error: %d", ret);
		return -ENOEXEC;
	}

	return 0;
}

static int ap_disable(void)
{
	int ret;
	struct net_if *iface = net_if_get_first_wifi();

	ret = net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to disable AP, error: %d", ret);
		return -ENOEXEC;
	}

	return 0;
}

/* Update this function to parse using NanoPB. */
static int parse_and_store_credentials(const char *body, size_t body_len)
{
	int ret;
	WifiConfig credentials = WifiConfig_init_zero;

	pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)body, body_len);

	if (!pb_decode(&stream, WifiConfig_fields, &credentials)) {
		LOG_ERR("Decoding credentials failed");
		return -1;
	}

	LOG_DBG("Received Wi-Fi credentials: %s, %s, sectype: %d, channel: %d, band: %d",
		credentials.ssid,
		credentials.passphrase,
		credentials.authMode,
		credentials.channel,
		credentials.band);

	enum wifi_security_type sec_type =
		credentials.authMode == AuthMode_WPA_WPA2_PSK ? WIFI_SECURITY_TYPE_PSK :
		credentials.authMode == AuthMode_WPA2_PSK ? WIFI_SECURITY_TYPE_PSK_SHA256 :
		credentials.authMode == AuthMode_WPA3_PSK ? WIFI_SECURITY_TYPE_SAE :
		WIFI_SECURITY_TYPE_NONE;

	/* Preferred band is set using a flag. */
	uint32_t flag = (credentials.band == Band_BAND_2_4_GHZ) ? WIFI_CREDENTIALS_FLAG_2_4GHz :
			(credentials.band == Band_BAND_5_GHZ) ? WIFI_CREDENTIALS_FLAG_5GHz : 0;

	ret = wifi_credentials_set_personal(credentials.ssid, strlen(credentials.ssid),
					    sec_type, NULL, 0,
					    credentials.passphrase, strlen(credentials.passphrase),
					    flag, credentials.channel);
	if (ret) {
		LOG_ERR("Storing credentials failed, error: %d", ret);
		return ret;
	}

	notify_app(WIFI_PROVISION_EVT_CREDENTIALS_RECEIVED);

	return 0;
}

/* Zephyr State Machine Framework functions. */

/* Scan and cache Wi-Fi results in unprovisioned state. */
static void unprovisioned_entry(void *o)
{
	ARG_UNUSED(o);

	int ret = wifi_scan();

	if (ret) {
		LOG_ERR("wifi_scan, error: %d", ret);
		notify_app(WIFI_PROVISION_EVT_FATAL_ERROR);
		return;
	}
}

static void unprovisioned_exit(void *o)
{
	ARG_UNUSED(o);

	LOG_DBG("Scanning for Wi-Fi networks completed, preparing protobuf payload");

	/* If we have received all scan results, copy them to a buffer and notify the application. */
	pb_ostream_t stream = pb_ostream_from_buffer(scan_result_buffer, sizeof(scan_result_buffer));
	if (!pb_encode(&stream, ScanResults_fields, &scan_results)) {
		LOG_ERR("Encoding scan results failed");
		notify_app(WIFI_PROVISION_EVT_FATAL_ERROR);
		return;
	}

	LOG_DBG("Protobuf payload prepared, scan results encoded, size: %d", stream.bytes_written);

	scan_result_buffer_len = stream.bytes_written;
}

/* Scan for available Wi-Fi networks when we enter the provisioning state. */
static void provisioning_entry(void *o)
{
	ARG_UNUSED(o);

	LOG_DBG("Enabling AP mode to allow client to connect and provide Wi-Fi credentials.");
	LOG_DBG("Waiting for Wi-Fi credentials...");

	int ret = ap_enable();

	if (ret) {
		LOG_ERR("ap_enable, error: %d", ret);
		notify_app(WIFI_PROVISION_EVT_FATAL_ERROR);
		return;
	}

	/* Notify application that provisioning has started. */
	notify_app(WIFI_PROVISION_EVT_STARTED);
}

/*Wi-Fi credentials received, provisioned has completed, cleanup and disable AP mode. */
static void provisioned_entry(void *o)
{
	ARG_UNUSED(o);

	LOG_DBG("Credentials received, cleaning up...");

	ARG_UNUSED(o);

	int ret;
	struct net_if *iface = net_if_get_first_wifi();

	ret = net_dhcpv4_server_stop(iface);
	if (ret) {
		LOG_ERR("Failed to stop DHCPv4 server, error: %d", ret);
		notify_app(WIFI_PROVISION_EVT_FATAL_ERROR);
		return;
	}

	ret = ap_disable();
	if (ret) {
		LOG_ERR("ap_disable, error: %d", ret);
		notify_app(WIFI_PROVISION_EVT_FATAL_ERROR);
		return;
	}
}

/* We have received Wi-Fi credentials and is considered provisioned, disable AP mode. */
static void finished_entry(void *o)
{
	ARG_UNUSED(o);

	notify_app(WIFI_PROVISION_EVT_COMPLETED);

	/* Block init until softAP mode is disabled. */
	k_sem_give(&wifi_provision_sem);
}

/* Delete Wi-Fi credentials upon exit of the provisioning state. */
static void reset_entry(void *o)
{
	int ret;

	LOG_DBG("Exiting unprovisioned state, cleaning up and deleting stored Wi-Fi credentials");
	LOG_DBG("Deleting stored credentials...");

	ret = wifi_credentials_delete_all();
	if (ret) {
		LOG_ERR("wifi_credentials_delete_all, error: %d", ret);
		notify_app(WIFI_PROVISION_EVT_FATAL_ERROR);
		return;
	}

	/* Request reboot of the firmware to reset the device firmware and re-enter
	 * provisioning. Ideally we would bring the iface down/up but this is currently not
	 * supported.
	 */

	LOG_DBG("Wi-Fi credentials deleted, request reboot to re-enter provisioning (softAP mode)");

	notify_app(WIFI_PROVISION_EVT_REBOOT);
}

/* Construct state table */
static const struct smf_state state[] = {
	[UNPROVISIONED] = SMF_CREATE_STATE(unprovisioned_entry, NULL, unprovisioned_exit),
	[PROVISIONING] = SMF_CREATE_STATE(provisioning_entry, NULL, NULL),
	[PROVISIONED] = SMF_CREATE_STATE(provisioned_entry, NULL, NULL),
	[FINISHED] = SMF_CREATE_STATE(finished_entry, NULL, NULL),
	[RESET] = SMF_CREATE_STATE(reset_entry, NULL, NULL),
};

static struct http_parser_settings parser_settings;

static int send_response(struct http_req *request, char *response, size_t len)
{
	ssize_t out_len;

	while (len) {
		out_len = send(request->accepted, response, len, 0);
		if (out_len < 0) {
			LOG_ERR("send, error: %d", -errno);
			return -errno;
		}

		len -= out_len;
	}

	return 0;
}

/* Handle HTTP request */
static int handle_http_request(struct http_req *request)
{
	int ret;
	char response[2048] = { 0 };

	if ((strncmp(request->url, "/prov/networks", request->url_len) == 0)) {
		/* Wi-Fi scan requested, return the cached scan results. */

		ret = snprintk(response, sizeof(response),
			"%sContent-Type: application/x-protobuf\r\nContent-Length: %d\r\n\r\n",
			RESPONSE_200, scan_result_buffer_len);
		if ((ret < 0) || (ret >= sizeof(response))) {
			LOG_DBG("snprintk, error: %d", ret);
			return ret;
		}

		/* Send headers */
		ret = send_response(request, response, strlen(response));
		if (ret) {
			LOG_ERR("send_response (headers), error: %d", ret);
			return ret;
		}

		/* Send payload */
		ret = send_response(request, scan_result_buffer, scan_result_buffer_len);
		if (ret) {
			LOG_ERR("send_response (payload), error: %d", ret);
			return ret;
		}

	} else if ((strncmp(request->url, "/prov/configure", request->url_len) == 0)) {
		/* Wi-Fi provisioning requested, parse the body and store the credentials. */

		ret = parse_and_store_credentials(request->body, request->body_len);
		if (ret) {
			LOG_ERR("parse_and_store_credentials, error: %d", ret);
			return ret;
		}

		ret = snprintk(response, sizeof(response), "%sContent-Length: %d\r\n\r\n",
			       RESPONSE_200, 0);
		if ((ret < 0) || (ret >= sizeof(response))) {
			LOG_DBG("snprintk, error: %d", ret);
			return ret;
		}

		ret = send_response(request, response, strlen(response));
		if (ret) {
			LOG_ERR("send_response, error: %d", ret);
			return ret;
		}

		credentials_stored = true;

		state_set(PROVISIONED, K_SECONDS(1));
	} else {
		LOG_DBG("Unrecognized HTTP resource, ignoring...");
	}

	return 0;
}

static int process_tcp(sa_family_t family)
{
	int client;
	struct sockaddr_in6 client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	char addr_str[INET6_ADDRSTRLEN];
	int received;
	int ret = 0;
	char buf[2048];
	size_t offset = 0;
	size_t total_received = 0;

	client = accept(request.socket, (struct sockaddr *)&client_addr, &client_addr_len);
	if (client < 0) {
		LOG_ERR("Error in accept %d, try again", -errno);
		return -errno;
	}

	request.accepted = client;

	net_addr_ntop(client_addr.sin6_family, &client_addr.sin6_addr, addr_str, sizeof(addr_str));
	LOG_DBG("[%d] Connection from %s accepted", request.accepted, addr_str);

	http_parser_init(&request.parser, HTTP_REQUEST);

	while (true) {
		received = recv(request.accepted, buf + offset, sizeof(buf) - offset, 0);
		if (received == 0) {
			/* Connection closed */
			ret = -ECONNRESET;
			LOG_DBG("[%d] Connection closed by peer", request.accepted);
			goto socket_close;
		} else if (received < 0) {
			/* Socket error */
			ret = -errno;
			LOG_ERR("[%d] Connection error %d", request.accepted, ret);
			goto socket_close;
		}

		/* Parse the received data as HTTP request */
		(void)http_parser_execute(&request.parser,
					  &parser_settings, buf + offset, received);

		total_received += received;
		offset += received;

		if (offset >= sizeof(buf)) {
			offset = 0;
		}

		/* If the HTTP request has been completely received, stop receiving data and
		 * proceed to process the request.
		 */
		if (request.received_all) {
			handle_http_request(&request);
			break;
		}
	};

socket_close:
	LOG_DBG("Closing listening socket: %d", request.accepted);
	(void)close(request.accepted);

	return ret;
}

static int setup_server(int *sock, struct sockaddr *bind_addr, socklen_t bind_addrlen)
{
	int ret;
	int enable = 1;

	*sock = socket(bind_addr->sa_family, SOCK_STREAM, IPPROTO_TLS_1_2);
	if (*sock < 0) {
		LOG_ERR("Failed to create socket: %d", -errno);
		return -errno;
	}

	sec_tag_t sec_tag_list[] = {
		CONFIG_WIFI_PROVISION_CERTIFICATE_SEC_TAG,
	};

	ret = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set security tag list %d", -errno);
		return -errno;
	}

	ret = setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	if (ret) {
		LOG_ERR("Failed to set SO_REUSEADDR %d", -errno);
		return -errno;
	}

	ret = bind(*sock, bind_addr, bind_addrlen);
	if (ret < 0) {
		LOG_ERR("Failed to bind socket %d", -errno);
		return -errno;
	}

	ret = listen(*sock, 1);
	if (ret < 0) {
		LOG_ERR("Failed to listen on socket %d", -errno);
		return -errno;
	}

	return ret;
}

/* Processing incoming IPv4 clients */
static int process_tcp4(void)
{
	int ret;
	struct sockaddr_in addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(CONFIG_WIFI_PROVISION_TCP_PORT),
	};

	ret = setup_server(&request.socket, (struct sockaddr *)&addr4, sizeof(addr4));
	if (ret < 0) {
		LOG_ERR("Failed to create IPv4 socket %d", ret);
		return ret;
	}

	LOG_DBG("Waiting for IPv4 HTTP connections on port %d", CONFIG_WIFI_PROVISION_TCP_PORT);

	while (true) {

		/* Process incoming IPv4 clients */
		ret = process_tcp(AF_INET);
		if (ret < 0) {
			LOG_ERR("Failed to process TCP %d", ret);
			return ret;
		}

		if (credentials_stored) {
			LOG_DBG("Credentials stored, closing server socket");
			close(request.socket);
			break;
		}
	}

	return 0;
}

/* HTTP parser callbacks */
static int on_body(struct http_parser *parser, const char *at, size_t length)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->body = at;
	req->body_len = length;

	LOG_DBG("on_body: %d", parser->method);
	LOG_DBG("> %.*s", length, at);

	return 0;
}

static int on_headers_complete(struct http_parser *parser)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->method = parser->method;

	LOG_DBG("on_headers_complete, method: %s", http_method_str(parser->method));

	return 0;
}

static int on_message_begin(struct http_parser *parser)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->received_all = false;

	LOG_DBG("on_message_begin, method: %d", parser->method);

	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->received_all = true;

	LOG_DBG("on_message_complete, method: %d", parser->method);

	return 0;
}

static int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct http_req *req = CONTAINER_OF(parser, struct http_req, parser);

	req->url = at;
	req->url_len = length;

	LOG_DBG("on_url, method: %d", parser->method);
	LOG_DBG("> %.*s", length, at);

	return 0;
}

int wifi_provision_start(const wifi_provision_evt_handler_t handler)
{
	int ret;

	/* Initialize and start dedicated workqueue.
	 * This workqueue is used to offload tasks and state transitions.
	 */
	k_work_queue_init(&queue);
	k_work_queue_start(&queue, stack_area,
			   K_THREAD_STACK_SIZEOF(stack_area),
			   K_HIGHEST_APPLICATION_THREAD_PRIO,
			   NULL);

	/* Set library callback handler */
	handler_cb = handler;

	if (!wifi_credentials_is_empty()) {
		LOG_DBG("Stored Wi-Fi credentials found, already provisioned");
		smf_set_initial(SMF_CTX(&s_obj), &state[FINISHED]);
		return 0;
	}

	/* Provision self-signed server certificate */
	ret = tls_credential_add(CONFIG_WIFI_PROVISION_CERTIFICATE_SEC_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 server_certificate,
				 sizeof(server_certificate));
	if (ret == -EEXIST) {
		LOG_DBG("Public certificate already exists, sec tag: %d",
			CONFIG_WIFI_PROVISION_CERTIFICATE_SEC_TAG);
	} else if (ret < 0) {
		LOG_ERR("Failed to register public certificate: %d", ret);
		return ret;
	}

	ret = tls_credential_add(CONFIG_WIFI_PROVISION_CERTIFICATE_SEC_TAG,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 server_certificate,
				 sizeof(server_certificate));
	if (ret == -EEXIST) {
		LOG_DBG("Public certificate already exists, sec tag: %d",
			CONFIG_WIFI_PROVISION_CERTIFICATE_SEC_TAG);
	} else if (ret < 0) {
		LOG_ERR("Failed to register public certificate: %d", ret);
		return ret;
	}

	ret = tls_credential_add(CONFIG_WIFI_PROVISION_CERTIFICATE_SEC_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 server_private_key, sizeof(server_private_key));

	if (ret == -EEXIST) {
		LOG_DBG("Private key already exists, sec tag: %d",
			CONFIG_WIFI_PROVISION_CERTIFICATE_SEC_TAG);
	} else if (ret < 0) {
		LOG_ERR("Failed to register private key: %d", ret);
		return ret;
	}

	LOG_DBG("Self-signed server certificate provisioned");

	net_mgmt_init_event_callback(&net_l2_mgmt_cb,
				     net_mgmt_wifi_event_handler,
				     NET_MGMT_WIFI);

	net_mgmt_add_event_callback(&net_l2_mgmt_cb);

	smf_set_initial(SMF_CTX(&s_obj), &state[UNPROVISIONED]);

	http_parser_settings_init(&parser_settings);

	parser_settings.on_body = on_body;
	parser_settings.on_headers_complete = on_headers_complete;
	parser_settings.on_message_begin = on_message_begin;
	parser_settings.on_message_complete = on_message_complete;
	parser_settings.on_url = on_url;

	/* Start processing incoming IPv4 clients */
	ret = process_tcp4();
	if (ret < 0) {
		LOG_ERR("Failed to start TCP server %d", ret);
		return ret;
	}

	k_sem_take(&wifi_provision_sem, K_FOREVER);
	return 0;
}

int wifi_provision_reset(void)
{
	LOG_DBG("Resetting Wi-Fi provision state machine");
	state_set(RESET, K_SECONDS(1));
	return 0;
}

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/poll.h>
#else
#include <zephyr/net/socket.h>
#endif /* CONFIG_POSIX_API */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/random/random.h>
#include <zephyr/net/coap_client.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/tls_credentials.h>

/* CONFIG_COAP_SAMPLE_DTLS_SEC_TAG and CONFIG_COAP_SAMPLE_DTLS_HANDSHAKE_TIMEOUT_MAX_MS
 * are defined in Kconfig under "if COAP_SAMPLE_DTLS", so they don't exist at all when
 * CONFIG_COAP_SAMPLE_DTLS is disabled. Provide fallback values so the code referencing
 * them below compiles unconditionally; the IS_ENABLED(CONFIG_COAP_SAMPLE_DTLS) runtime
 * check ensures they are never actually used in that case.
 */
#ifndef CONFIG_COAP_SAMPLE_DTLS_SEC_TAG
#define CONFIG_COAP_SAMPLE_DTLS_SEC_TAG 0
#endif
#ifndef CONFIG_COAP_SAMPLE_DTLS_HANDSHAKE_TIMEOUT_MAX_MS
#define CONFIG_COAP_SAMPLE_DTLS_HANDSHAKE_TIMEOUT_MAX_MS 0
#endif

LOG_MODULE_REGISTER(coap_client_sample, CONFIG_COAP_CLIENT_SAMPLE_LOG_LEVEL);

/* Macro used to subscribe to specific Zephyr NET management events. */
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

#define MAX_CONSECUTIVE_BUSY_RETRIES 5

/* Any IP family, and DNS to resolve the server host name */
#define READY_MASK (CONN_MGR_READY_IP_ANY | CONN_MGR_READY_DNS)

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()					\
	LOG_ERR("Fatal error! Rebooting the device.");	\
	LOG_PANIC();					\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

static void server_addr_set_ipv4(struct sockaddr_storage *server, struct addrinfo *result)
{
	struct sockaddr_in *server4 = (struct sockaddr_in *)server;
	char addr_str[NET_IPV6_ADDR_LEN];

	server4->sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_COAP_SAMPLE_SERVER_PORT);

	inet_ntop(AF_INET, &server4->sin_addr.s_addr, addr_str, sizeof(addr_str));
	LOG_INF("IPv4 Address found %s", addr_str);
}

static void server_addr_set_ipv6(struct sockaddr_storage *server, struct addrinfo *result)
{
	struct sockaddr_in6 *server6 = (struct sockaddr_in6 *)server;
	char addr_str[NET_IPV6_ADDR_LEN];

	memcpy(&server6->sin6_addr,
	       &((struct sockaddr_in6 *)result->ai_addr)->sin6_addr,
	       sizeof(server6->sin6_addr));
	server6->sin6_family = AF_INET6;
	server6->sin6_port = htons(CONFIG_COAP_SAMPLE_SERVER_PORT);

	inet_ntop(AF_INET6, &server6->sin6_addr, addr_str, sizeof(addr_str));
	LOG_INF("IPv6 Address found %s", addr_str);
}

/* Resolves the server for the address families that are ready */
static int server_resolve(struct addrinfo **result)
{
	int err;
	struct addrinfo hints = {
		.ai_family = conn_mgr_get_ready_addr_family(),
		.ai_socktype = SOCK_DGRAM
	};

	err = getaddrinfo(CONFIG_COAP_SAMPLE_SERVER_HOSTNAME, NULL, &hints, result);
	if (err) {
		LOG_ERR("getaddrinfo, error: %d", err);
		return err;
	}

	return 0;
}

static int server_addr_set(struct sockaddr_storage *server, struct addrinfo *result)
{
	/* Resolve either IPv4 or IPv6 */
	switch (result->ai_family) {
	case AF_INET:
		server_addr_set_ipv4(server, result);
		return 0;
	case AF_INET6:
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			server_addr_set_ipv6(server, result);
			return 0;
		}
		__fallthrough;
	default:
		LOG_ERR("Unexpected address family: %d", result->ai_family);
		return -EAFNOSUPPORT;
	}
}

static void response_cb(const struct coap_client_response_data *data, void *user_data)
{
	if (data->result_code >= 0) {
		LOG_INF("CoAP response: code: 0x%x, payload: %s",
			data->result_code, data->payload);
	} else {
		LOG_INF("Response received with error code: %d", data->result_code);
	}
}

static int credentials_add(void)
{
	int err;

	if (IS_ENABLED(CONFIG_COAP_SAMPLE_DTLS)) {
		static const char ca_cert[] = {
#if defined(CONFIG_COAP_SAMPLE_DTLS)
			#include "coap_sample_ca_cert.inc"
			'\0'
#endif
		};
		static const char client_cert[] = {
#if defined(CONFIG_COAP_SAMPLE_DTLS)
			#include "coap_sample_client_cert.inc"
			'\0'
#endif
		};
		static const char client_key[] = {
#if defined(CONFIG_COAP_SAMPLE_DTLS)
			#include "coap_sample_client_key.inc"
			'\0'
#endif
		};

		err = tls_credential_add(CONFIG_COAP_SAMPLE_DTLS_SEC_TAG,
					 TLS_CREDENTIAL_CA_CERTIFICATE,
					 ca_cert, sizeof(ca_cert));
		if ((err < 0) && (err != -EEXIST)) {
			LOG_ERR("Failed to register CA certificate: %d", err);
			FATAL_ERROR();
			return err;
		}

		err = tls_credential_add(CONFIG_COAP_SAMPLE_DTLS_SEC_TAG,
					 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
					 client_cert, sizeof(client_cert));
		if ((err < 0) && (err != -EEXIST)) {
			LOG_ERR("Failed to register client certificate: %d", err);
			FATAL_ERROR();
			return err;
		}

		err = tls_credential_add(CONFIG_COAP_SAMPLE_DTLS_SEC_TAG,
					 TLS_CREDENTIAL_PRIVATE_KEY,
					 client_key, sizeof(client_key));
		if ((err < 0) && (err != -EEXIST)) {
			LOG_ERR("Failed to register private key: %d", err);
			FATAL_ERROR();
			return err;
		}
	}

	return 0;
}

/* Opens a socket to the server, performing the DTLS handshake if enabled.
 * Returns the socket on success, or a negative error code on a recoverable failure.
 */
static int server_connect(struct sockaddr_storage *server)
{
	int err, sock;

	sock = socket(server->ss_family, SOCK_DGRAM,
		      IS_ENABLED(CONFIG_COAP_SAMPLE_DTLS) ? IPPROTO_DTLS_1_2 : IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create CoAP socket: %d.", -errno);
		FATAL_ERROR();
		return -errno;
	}

	if (IS_ENABLED(CONFIG_COAP_SAMPLE_DTLS)) {
		sec_tag_t sec_tag_list[] = { CONFIG_COAP_SAMPLE_DTLS_SEC_TAG };

		err = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
				 sec_tag_list, sizeof(sec_tag_list));
		if (err < 0) {
			LOG_ERR("Failed to set TLS_SEC_TAG_LIST: %d", -errno);
			(void)zsock_close(sock);
			FATAL_ERROR();
			return -errno;
		}

		uint32_t handshake_timeout_ms = CONFIG_COAP_SAMPLE_DTLS_HANDSHAKE_TIMEOUT_MAX_MS;

		err = setsockopt(sock, SOL_TLS, TLS_DTLS_HANDSHAKE_TIMEOUT_MAX,
				 &handshake_timeout_ms, sizeof(handshake_timeout_ms));
		if (err < 0) {
			LOG_ERR("Failed to set TLS_DTLS_HANDSHAKE_TIMEOUT_MAX: %d", -errno);
			(void)zsock_close(sock);
			FATAL_ERROR();
			return -errno;
		}

		/* Set the expected server hostname for SNI and server certificate
		 * CN/SAN verification.
		 */
		err = setsockopt(sock, SOL_TLS, TLS_HOSTNAME,
				 CONFIG_COAP_SAMPLE_SERVER_HOSTNAME,
				 sizeof(CONFIG_COAP_SAMPLE_SERVER_HOSTNAME) - 1);
		if (err < 0) {
			LOG_ERR("Failed to set TLS_HOSTNAME: %d", -errno);
			(void)zsock_close(sock);
			FATAL_ERROR();
			return -errno;
		}

		/* Explicitly connect to trigger the DTLS handshake now so we can log
		 * the outcome and negotiated ciphersuite before handing the socket to
		 * the CoAP client.
		 */
		LOG_INF("Performing DTLS handshake with %s:%d (mutual TLS)...",
			CONFIG_COAP_SAMPLE_SERVER_HOSTNAME,
			CONFIG_COAP_SAMPLE_SERVER_PORT);

		err = zsock_connect(sock, (struct sockaddr *)server,
				    server->ss_family == AF_INET6 ?
					    sizeof(struct sockaddr_in6) :
					    sizeof(struct sockaddr_in));
		if (err < 0) {
			err = -errno;
			LOG_ERR("DTLS handshake failed: %d", err);
			(void)zsock_close(sock);
			return err;
		}

		int cipher_id;
		socklen_t cipher_id_len = sizeof(cipher_id);

		err = getsockopt(sock, SOL_TLS, TLS_CIPHERSUITE_USED, &cipher_id, &cipher_id_len);
		if (err == 0) {
			LOG_INF("DTLS handshake complete, ciphersuite: 0x%04x", cipher_id);
		} else {
			LOG_INF("DTLS handshake complete (ciphersuite unknown, getsockopt err %d)",
				-errno);
		}
	}

	return sock;
}

/* Resolves the server and connects to the first address that accepts a connection */
static int server_resolve_and_connect(struct sockaddr_storage *server)
{
	struct addrinfo *result;
	struct addrinfo *ai;
	int sock = -ENOENT;
	int err;

	err = server_resolve(&result);
	if (err) {
		LOG_ERR("Failed to resolve server name");
		return err;
	}

	for (ai = result; ai != NULL && sock < 0; ai = ai->ai_next) {
		if (server_addr_set(server, ai) == 0) {
			sock = server_connect(server);
		}
	}

	freeaddrinfo(result);

	return sock;
}

static void server_disconnect(struct coap_client *coap_client, int *sock)
{
	coap_client_cancel_requests(coap_client);
	(void)zsock_close(*sock);
	*sock = -1;
}

/* Only returns on fatal errors. Recoverable errors close the socket and reconnect. */
static int periodic_coap_request_loop(void)
{
	int err;
	int sock = -1;
	int consecutive_busy_retries = 0;
	uint32_t last_seen = 0;
	struct sockaddr_storage server = { 0 };
	struct coap_client coap_client = { 0 };
	struct coap_client_request req = {
		.method = COAP_METHOD_GET,
		.confirmable = true,
		.fmt = COAP_CONTENT_FORMAT_TEXT_PLAIN,
		.payload = NULL,
		.cb = response_cb,
		.len = 0,
		.path = CONFIG_COAP_SAMPLE_RESOURCE,
	};

	err = credentials_add();
	if (err) {
		return err;
	}

	err = coap_client_init(&coap_client, NULL);
	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
		FATAL_ERROR();
		return err;
	}

	LOG_INF("Waiting for network...");
	(void)conn_mgr_wait_ready(READY_MASK, NULL, K_FOREVER);
	LOG_INF("Network ready");

	while (true) {
		if (sock < 0) {
			/* At once the first time, then after the delay or a network change */
			err = conn_mgr_wait_ready(READY_MASK, &last_seen,
						  K_SECONDS(CONFIG_SAMPLE_RECONNECT_DELAY_SECONDS));
			if (err == -EAGAIN) {
				/* The network is not ready. Keep waiting. */
				continue;
			} else if (err != 0) {
				return err;
			}

			sock = server_resolve_and_connect(&server);
			if (sock < 0) {
				LOG_WRN("Failed to connect to server: %d, retrying in %d seconds",
					sock, CONFIG_SAMPLE_RECONNECT_DELAY_SECONDS);
				continue;
			}
		}

		/* Send request */
		err = coap_client_req(&coap_client, sock, (struct sockaddr *)&server, &req, NULL);
		if (err == -EAGAIN &&
		    ++consecutive_busy_retries < MAX_CONSECUTIVE_BUSY_RETRIES) {
			LOG_WRN("CoAP client busy, retrying later");
			k_sleep(K_SECONDS(CONFIG_COAP_SAMPLE_REQUEST_INTERVAL_SECONDS));
			continue;
		}

		consecutive_busy_retries = 0;

		if (err) {
			LOG_ERR("Failed to send request: %d, reconnecting", err);
			server_disconnect(&coap_client, &sock);
			continue;
		}

		LOG_INF("CoAP GET request sent to %s:%d, resource: %s",
			CONFIG_COAP_SAMPLE_SERVER_HOSTNAME,
			CONFIG_COAP_SAMPLE_SERVER_PORT,
			CONFIG_COAP_SAMPLE_RESOURCE);

		k_sleep(K_SECONDS(CONFIG_COAP_SAMPLE_REQUEST_INTERVAL_SECONDS));
	}
}

static void connectivity_event_handler(uint64_t event, struct net_if *iface, void *info,
				       size_t info_length, void *user_data)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		FATAL_ERROR();
		return;
	}
}

/* Registered statically to not miss events raised before main() */
NET_MGMT_REGISTER_EVENT_HANDLER(conn_events, CONN_LAYER_EVENT_MASK, connectivity_event_handler,
				NULL);

int main(void)
{
	int err;

	LOG_INF("The CoAP client sample started");

	/* Bring all network interfaces up.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	LOG_INF("Bringing network interface up and connecting to the network");

	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_up, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	err = conn_mgr_all_if_connect(true);
	if (err) {
		LOG_ERR("conn_mgr_all_if_connect, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	err = periodic_coap_request_loop();
	if (err) {
		LOG_ERR("periodic_coap_request_loop, error: %d", err);
	}

	LOG_INF("Bringing network interfaces down");
	(void)conn_mgr_all_if_disconnect(true);
	(void)conn_mgr_all_if_down(true);

	return err;
}

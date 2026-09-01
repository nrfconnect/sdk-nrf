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

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

#define MAX_CONSECUTIVE_BUSY_RETRIES 5

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()					\
	LOG_ERR("Fatal error! Rebooting the device.");	\
	LOG_PANIC();					\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

/* Variable used to indicate if network is connected. */
static bool is_connected;

/* Mutex and conditional variable used to signal network connectivity. */
K_MUTEX_DEFINE(network_connected_lock);
K_CONDVAR_DEFINE(network_connected);

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

static int server_resolve(struct sockaddr_storage *server)
{
	int err;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM
	};

	err = getaddrinfo(CONFIG_COAP_SAMPLE_SERVER_HOSTNAME, NULL, &hints, &result);
	if (err) {
		LOG_ERR("getaddrinfo, error: %d", err);
		return err;
	}

	if (result == NULL) {
		LOG_ERR("Address not found");
		return -ENOENT;
	}

	/* Resolve either IPv4 or IPv6 */
	switch (result->ai_family) {
	case AF_INET:
		server_addr_set_ipv4(server, result);
		break;
	case AF_INET6:
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			server_addr_set_ipv6(server, result);
			break;
		}
		__fallthrough;
	default:
		LOG_ERR("Unexpected address family: %d", result->ai_family);
		freeaddrinfo(result);
		return -EAFNOSUPPORT;
	}

	/* Free the address. */
	freeaddrinfo(result);

	return 0;
}

static void wait_for_network(void)
{
	k_mutex_lock(&network_connected_lock, K_FOREVER);

	if (!is_connected) {
		LOG_INF("Waiting for network connectivity");
		k_condvar_wait(&network_connected, &network_connected_lock, K_FOREVER);
	}

	k_mutex_unlock(&network_connected_lock);
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

static int periodic_coap_request_loop(void)
{
	int err, sock;
	int consecutive_busy_retries = 0;
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

	err = server_resolve(&server);
	if (err) {
		LOG_ERR("Failed to resolve server name");
		return err;
	}

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
			return err;
		}

		err = tls_credential_add(CONFIG_COAP_SAMPLE_DTLS_SEC_TAG,
					 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
					 client_cert, sizeof(client_cert));
		if ((err < 0) && (err != -EEXIST)) {
			LOG_ERR("Failed to register client certificate: %d", err);
			return err;
		}

		err = tls_credential_add(CONFIG_COAP_SAMPLE_DTLS_SEC_TAG,
					 TLS_CREDENTIAL_PRIVATE_KEY,
					 client_key, sizeof(client_key));
		if ((err < 0) && (err != -EEXIST)) {
			LOG_ERR("Failed to register private key: %d", err);
			return err;
		}
	}

	sock = socket(server.ss_family, SOCK_DGRAM,
		      IS_ENABLED(CONFIG_COAP_SAMPLE_DTLS) ? IPPROTO_DTLS_1_2 : IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create CoAP socket: %d.", -errno);
		return -errno;
	}

	if (IS_ENABLED(CONFIG_COAP_SAMPLE_DTLS)) {
		sec_tag_t sec_tag_list[] = { CONFIG_COAP_SAMPLE_DTLS_SEC_TAG };

		err = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
				 sec_tag_list, sizeof(sec_tag_list));
		if (err < 0) {
			LOG_ERR("Failed to set TLS_SEC_TAG_LIST: %d", -errno);
			(void)zsock_close(sock);
			return -errno;
		}

		uint32_t handshake_timeout_ms = CONFIG_COAP_SAMPLE_DTLS_HANDSHAKE_TIMEOUT_MAX_MS;

		err = setsockopt(sock, SOL_TLS, TLS_DTLS_HANDSHAKE_TIMEOUT_MAX,
				 &handshake_timeout_ms, sizeof(handshake_timeout_ms));
		if (err < 0) {
			LOG_ERR("Failed to set TLS_DTLS_HANDSHAKE_TIMEOUT_MAX: %d", -errno);
			(void)zsock_close(sock);
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
			return -errno;
		}

		/* Explicitly connect to trigger the DTLS handshake now so we can log
		 * the outcome and negotiated ciphersuite before handing the socket to
		 * the CoAP client.
		 */
		LOG_INF("Performing DTLS handshake with %s:%d (mutual TLS)...",
			CONFIG_COAP_SAMPLE_SERVER_HOSTNAME,
			CONFIG_COAP_SAMPLE_SERVER_PORT);

		err = zsock_connect(sock, (struct sockaddr *)&server,
				    server.ss_family == AF_INET6 ?
					    sizeof(struct sockaddr_in6) :
					    sizeof(struct sockaddr_in));
		if (err < 0) {
			LOG_ERR("DTLS handshake failed: %d", -errno);
			(void)zsock_close(sock);
			return -errno;
		}

		int cipher_id;
		socklen_t cipher_id_len = sizeof(cipher_id);

		err = getsockopt(sock, SOL_TLS, TLS_CIPHERSUITE_USED, &cipher_id, &cipher_id_len);
		if (err == 0) {
			LOG_INF("DTLS handshake complete, ciphersuite: 0x%04x", cipher_id);
		} else {
			LOG_INF("DTLS handshake complete (ciphersuite unknown, getsockopt err %d)",
				err);
		}
	}

	err = coap_client_init(&coap_client, NULL);
	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
		return err;
	}

	while (true) {
		wait_for_network();

		/* Send request */
		err = coap_client_req(&coap_client, sock, (struct sockaddr *)&server, &req, NULL);
		if (err) {
			if (err == -EAGAIN) {
				consecutive_busy_retries++;
				if (consecutive_busy_retries >= MAX_CONSECUTIVE_BUSY_RETRIES) {
					LOG_ERR("CoAP client busy after %d consecutive retries",
						consecutive_busy_retries);
					return err;
				}

				LOG_WRN("CoAP client busy, retrying later");
				k_sleep(K_SECONDS(CONFIG_COAP_SAMPLE_REQUEST_INTERVAL_SECONDS));
				continue;
			}

			LOG_ERR("Failed to send request: %d", err);
			return err;
		}

		consecutive_busy_retries = 0;

		LOG_INF("CoAP GET request sent to %s:%d, resource: %s",
			CONFIG_COAP_SAMPLE_SERVER_HOSTNAME,
			CONFIG_COAP_SAMPLE_SERVER_PORT,
			CONFIG_COAP_SAMPLE_RESOURCE);

		k_sleep(K_SECONDS(CONFIG_COAP_SAMPLE_REQUEST_INTERVAL_SECONDS));
	}
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint64_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established");
		k_mutex_lock(&network_connected_lock, K_FOREVER);
		is_connected = true;
		k_condvar_signal(&network_connected);
		k_mutex_unlock(&network_connected_lock);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		k_mutex_lock(&network_connected_lock, K_FOREVER);
		is_connected = false;
		k_mutex_unlock(&network_connected_lock);
		break;
	default:
		/* Don't care */
		return;
	}
}
static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
						uint64_t event,
						struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		FATAL_ERROR();
		return;
	}
}

int main(void)
{
	int err;

	LOG_INF("The CoAP client sample started");

	/* Setup handler for Zephyr NET Connection Manager events and Connectivity layer. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

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

	/* Resend connection status if the sample is built for NATIVE_SIM.
	 * This is necessary because the network interface is automatically brought up
	 * at SYS_INIT() before main() is called.
	 * This means that NET_EVENT_L4_CONNECTED fires before the
	 * appropriate handler l4_event_handler() is registered.
	 */
	if (IS_ENABLED(CONFIG_BOARD_NATIVE_SIM)) {
		conn_mgr_mon_resend_status();
	}

	wait_for_network();

	err = periodic_coap_request_loop();
	if (err) {
		LOG_ERR("periodic_coap_request_loop, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	return 0;
}

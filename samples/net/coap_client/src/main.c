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
#include <zephyr/random/random.h>
#include <zephyr/net/coap_client.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/tls_credentials.h>
#include <net/ready_resolve.h>

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

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()					\
	LOG_ERR("Fatal error! Rebooting the device.");	\
	LOG_PANIC();					\
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

/* Zephyr NET management event callback structure. */
static struct net_mgmt_event_callback conn_cb;

/* Track failure of the current request. */
static volatile bool request_failed;

static void response_cb(const struct coap_client_response_data *data, void *user_data)
{
	if (data->result_code >= 0) {
		LOG_INF("CoAP response: code: 0x%x, payload: %s",
			data->result_code, data->payload);
	} else {
		LOG_INF("Response received with error code: %d", data->result_code);
		/* Don't count cancellation as failure */
		if (data->result_code != -ECANCELED) {
			request_failed = true;
		}
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

/* Opens a socket and connects it, performing the DTLS handshake if enabled */
static int server_connect(const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	int err, sock;

	sock = socket(addr->sa_family, SOCK_DGRAM,
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

		LOG_INF("Performing DTLS handshake (mutual TLS)...");
	}

	/* For DTLS, connecting performs the handshake. For plain UDP it only sets the peer. */
	err = connect(sock, (const struct sockaddr *)addr, addrlen);
	if (err < 0) {
		err = -errno;
		LOG_ERR("Connect failed: %d", err);
		(void)zsock_close(sock);
		return err;
	}

	if (IS_ENABLED(CONFIG_COAP_SAMPLE_DTLS)) {
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

/* Connects to the next resolved address that accepts a connection */
static int server_connect_next(struct ready_resolve *host, struct net_sockaddr_storage *server)
{
	const struct net_sockaddr *addr;
	net_socklen_t addrlen;
	int sock = -1;

	while (sock < 0 && ready_resolve_next(host, &addr, &addrlen) == 0) {
		sock = server_connect(addr, addrlen);
	}

	if (sock >= 0) {
		/* coap_client_req() needs the peer address */
		memcpy(server, addr, addrlen);
	}

	return sock;
}

/* Connects to the next server address. Plain UDP connect() always succeeds, so addresses are
 * kept across failed requests and the host is resolved again only once all were tried.
 */
static int server_connect_retry(struct ready_resolve *host, struct net_sockaddr_storage *server)
{
	const struct zsock_addrinfo hints = {
		.ai_socktype = NET_SOCK_DGRAM,
	};
	int sock = server_connect_next(host, server);
	int err;

	if (sock < 0) {
		LOG_INF("Connecting to %s", CONFIG_COAP_SAMPLE_SERVER_HOSTNAME);
	}

	while (sock < 0) {
		err = ready_resolve(host, CONFIG_COAP_SAMPLE_SERVER_HOSTNAME, NULL, &hints,
				    CONFIG_COAP_SAMPLE_SERVER_PORT);
		if (err == 0) {
			sock = server_connect_next(host, server);
			if (sock >= 0) {
				break;
			}
		}

		/* -ENETDOWN: waiting for the network to come up */
		if (err != -ENETDOWN) {
			LOG_WRN("Failed to connect to server, retrying");
		}
		ready_resolve_wait_change(host, K_SECONDS(CONFIG_SAMPLE_RECONNECT_DELAY_SECONDS));
	}

	return sock;
}

static void server_disconnect(struct coap_client *coap_client, int *sock)
{
	coap_client_cancel_requests(coap_client);
	(void)zsock_close(*sock);
	*sock = -1;
	request_failed = false;
}

/* Only returns on fatal errors. Recoverable errors close the socket and reconnect. */
static int periodic_coap_request_loop(void)
{
	int err;
	int sock = -1;
	int consecutive_busy_retries = 0;
	struct coap_client coap_client = { 0 };
	struct ready_resolve host = { 0 };
	struct net_sockaddr_storage server;
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

	while (true) {
		if (sock >= 0 && request_failed) {
			LOG_WRN("Request failed, reconnecting");
			server_disconnect(&coap_client, &sock);
		}

		if (sock < 0) {
			sock = server_connect_retry(&host, &server);
		}

		/* Send request */
		err = coap_client_req(&coap_client, sock, (struct net_sockaddr *)&server, &req,
				      NULL);
		if (err == -EAGAIN &&
		    ++consecutive_busy_retries < MAX_CONSECUTIVE_BUSY_RETRIES) {
			LOG_WRN("CoAP client busy, retrying later");
		} else if (err) {
			consecutive_busy_retries = 0;
			LOG_ERR("Failed to send request: %d, reconnecting", err);
			server_disconnect(&coap_client, &sock);
			continue;
		} else {
			consecutive_busy_retries = 0;
			LOG_INF("CoAP GET request sent to %s:%d, resource: %s",
				CONFIG_COAP_SAMPLE_SERVER_HOSTNAME,
				CONFIG_COAP_SAMPLE_SERVER_PORT,
				CONFIG_COAP_SAMPLE_RESOURCE);
		}

		k_sleep(K_SECONDS(CONFIG_COAP_SAMPLE_REQUEST_INTERVAL_SECONDS));
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

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
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

	err = periodic_coap_request_loop();
	if (err) {
		LOG_ERR("periodic_coap_request_loop, error: %d", err);
	}

	LOG_INF("Bringing network interfaces down");
	(void)conn_mgr_all_if_disconnect(true);
	(void)conn_mgr_all_if_down(true);

	return err;
}

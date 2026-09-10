/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/tls_credentials.h>

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#endif

#if CONFIG_MODEM_KEY_MGMT
#include <modem/modem_key_mgmt.h>
#endif

#define HTTPS_PORT		"443"
#define HTTP_HEAD		\
				"HEAD / HTTP/1.1\r\n"	\
				"Host: " CONFIG_HTTPS_HOSTNAME ":" HTTPS_PORT "\r\n"		\
				"Connection: close\r\n\r\n"

#define HTTP_HEAD_LEN		(sizeof(HTTP_HEAD) - 1)
#define HTTP_HDR_END		"\r\n\r\n"

#define RECV_BUF_SIZE		2048
#define TLS_SEC_TAG		42
#define IP_FAMILY_EVENT_IPV4	BIT(0)
#define IP_FAMILY_EVENT_IPV6	BIT(1)

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK		(NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED |	\
				 NET_EVENT_L4_IPV4_CONNECTED | NET_EVENT_L4_IPV4_DISCONNECTED |	\
				 NET_EVENT_L4_IPV6_CONNECTED | NET_EVENT_L4_IPV6_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK	(NET_EVENT_CONN_IF_FATAL_ERROR)

static const char send_buf[] = HTTP_HEAD;
static char recv_buf[RECV_BUF_SIZE];
static K_SEM_DEFINE(network_connected_sem, 0, 1);
static K_EVENT_DEFINE(ip_family_event);
static ATOMIC_DEFINE(network_state, 2);
/* Certificate for `example.com` */
static const char cert[] = {
	#include "example_com_ca.pem.inc"

	/* Null terminate certificate if running Mbed TLS on the application core.
	 * Required by TLS credentials API.
	 */
	IF_ENABLED(CONFIG_TLS_CREDENTIALS, (0x00))
};

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

enum network_state_flag {
	NETWORK_STATE_IPV4_READY,
	NETWORK_STATE_IPV6_READY,
};

BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

/* Provision certificate to modem */
int cert_provision(void)
{
	int err;

	printk("Provisioning certificate\n");

#if CONFIG_MODEM_KEY_MGMT
	bool exists;
	int mismatch;

	/* It may be sufficient for you application to check whether the correct
	 * certificate is provisioned with a given tag directly using modem_key_mgmt_cmp().
	 * Here, for the sake of the completeness, we check that a certificate exists
	 * before comparing it with what we expect it to be.
	 */
	err = modem_key_mgmt_exists(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		mismatch = modem_key_mgmt_cmp(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert,
					      sizeof(cert));
		if (!mismatch) {
			printk("Certificate match\n");
			return 0;
		}

		printk("Certificate mismatch\n");
		err = modem_key_mgmt_delete(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n", err);
		}
	}

	printk("Provisioning certificate to the modem\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert,
				   sizeof(cert));
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}
#else /* CONFIG_MODEM_KEY_MGMT */
	err = tls_credential_add(TLS_SEC_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 cert,
				 sizeof(cert));
	if (err == -EEXIST) {
		printk("CA certificate already exists, sec tag: %d\n", TLS_SEC_TAG);
	} else if (err < 0) {
		printk("Failed to register CA certificate: %d\n", err);
		return err;
	}
#endif /* !CONFIG_MODEM_KEY_MGMT */

	return 0;
}


/* Setup TLS options on a given socket */
int tls_setup(int fd)
{
	int err;
	int verify;

	/* Security tag that we have provisioned the certificate with */
	const sec_tag_t tls_sec_tag[] = {
		TLS_SEC_TAG,
	};

	/* Set up TLS peer verification */
	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		printk("Failed to setup peer verification, err %d\n", errno);
		return err;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag, sizeof(tls_sec_tag));
	if (err) {
		printk("Failed to setup TLS sec tag, err %d\n", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME,
			CONFIG_HTTPS_HOSTNAME,
			sizeof(CONFIG_HTTPS_HOSTNAME) - 1);
	if (err) {
		printk("Failed to setup TLS hostname, err %d\n", errno);
		return err;
	}
	return 0;
}

static void on_net_event_l4_disconnected(void)
{
	printk("Disconnected from the network\n");
}

static void on_net_event_l4_connected(void)
{
	k_sem_give(&network_connected_sem);
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint64_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		printk("Network connectivity established and IP address assigned\n");
		on_net_event_l4_connected();
		break;
	case NET_EVENT_L4_DISCONNECTED:
		printk("Network connectivity lost\n");
		on_net_event_l4_disconnected();
		break;
	case NET_EVENT_L4_IPV4_CONNECTED:
		atomic_set_bit(network_state, NETWORK_STATE_IPV4_READY);
		k_event_post(&ip_family_event, IP_FAMILY_EVENT_IPV4);
		printk("IPv4 connectivity established\n");
		break;
	case NET_EVENT_L4_IPV4_DISCONNECTED:
		printk("IPv4 connectivity lost\n");
		atomic_clear_bit(network_state, NETWORK_STATE_IPV4_READY);
		k_event_clear(&ip_family_event, IP_FAMILY_EVENT_IPV4);
		break;
	case NET_EVENT_L4_IPV6_CONNECTED:
		atomic_set_bit(network_state, NETWORK_STATE_IPV6_READY);
		k_event_post(&ip_family_event, IP_FAMILY_EVENT_IPV6);
		printk("IPv6 connectivity established\n");
		break;
	case NET_EVENT_L4_IPV6_DISCONNECTED:
		printk("IPv6 connectivity lost\n");
		atomic_clear_bit(network_state, NETWORK_STATE_IPV6_READY);
		k_event_clear(&ip_family_event, IP_FAMILY_EVENT_IPV6);
		break;
	default:
		break;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint64_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		printk("Fatal error received from the connectivity layer\n");
		return;
	}
}

static bool ipv4_ready(void)
{
	return atomic_test_bit(network_state, NETWORK_STATE_IPV4_READY);
}

static bool ipv6_ready(void)
{
	return atomic_test_bit(network_state, NETWORK_STATE_IPV6_READY);
}

static bool family_ready(int family)
{
	if (family == AF_INET) {
		return ipv4_ready();
	}

	if (family == AF_INET6) {
		return ipv6_ready();
	}

	return false;
}

static int wait_for_family_ready_event(int family)
{
	const uint32_t family_event = (family == AF_INET) ? IP_FAMILY_EVENT_IPV4 :
					IP_FAMILY_EVENT_IPV6;
	int events;

	printk("Waiting for local %s address event\n", family == AF_INET ? "IPv4" : "IPv6");
	events = k_event_wait(&ip_family_event, family_event, false, K_FOREVER);
	if (events == 0) {
		printk("Failed to wait for local %s address event\n",
		       family == AF_INET ? "IPv4" : "IPv6");
		return -EIO;
	}

	if (!family_ready(family)) {
		printk("Local %s address is not ready\n", family == AF_INET ? "IPv4" : "IPv6");
		return -EIO;
	}

	printk("Local %s address is now ready\n", family == AF_INET ? "IPv4" : "IPv6");

	return 0;
}

static int connect_to_family(struct addrinfo *res, int family, int *last_err)
{
	int fd;
	int err;
	uint16_t peer_port;
	char peer_addr[INET6_ADDRSTRLEN];

	for (struct addrinfo *entry = res; entry != NULL; entry = entry->ai_next) {
		const void *addr_ptr;

		if (entry->ai_family != family) {
			continue;
		}

		if (entry->ai_family == AF_INET6) {
			addr_ptr = &((const struct sockaddr_in6 *)entry->ai_addr)->sin6_addr;
			peer_port = ntohs(((const struct sockaddr_in6 *)entry->ai_addr)->sin6_port);
		} else {
			addr_ptr = &((const struct sockaddr_in *)entry->ai_addr)->sin_addr;
			peer_port = ntohs(((const struct sockaddr_in *)entry->ai_addr)->sin_port);
		}

		if (inet_ntop(entry->ai_family, addr_ptr, peer_addr, sizeof(peer_addr)) == NULL) {
			(void)snprintf(peer_addr, sizeof(peer_addr), "<inet_ntop err %d>", errno);
		}

		printk("Resolved %s (%s)\n", peer_addr, net_family2str(entry->ai_family));

		if (IS_ENABLED(CONFIG_SAMPLE_TFM_MBEDTLS)) {
			fd = socket(entry->ai_family, SOCK_STREAM | SOCK_NATIVE_TLS,
				    IPPROTO_TLS_1_2);
		} else {
			fd = socket(entry->ai_family, SOCK_STREAM, IPPROTO_TLS_1_2);
		}
		if (fd == -1) {
			*last_err = errno;
			printk("Failed to open socket for %s\n", peer_addr);
			continue;
		}

		err = tls_setup(fd);
		if (err) {
			*last_err = errno;
			(void)close(fd);
			continue;
		}

		printk("Connecting to %s:%d (%s)\n", CONFIG_HTTPS_HOSTNAME, peer_port, peer_addr);
		err = connect(fd, entry->ai_addr, entry->ai_addrlen);
		if (err) {
			*last_err = errno;
			printk("connect() failed for %s, err: %d. Will try another IP.\n",
			       peer_addr, *last_err);
			(void)close(fd);
			continue;
		}

		return fd;
	}

	return -1;
}

static int connect_to_server(struct addrinfo *res, int *last_err)
{
	int first_family = AF_UNSPEC;
	int second_family = AF_UNSPEC;
	int fd = -1;

	for (struct addrinfo *entry = res; entry != NULL; entry = entry->ai_next) {
		if (IS_ENABLED(CONFIG_NET_IPV4) && entry->ai_family == AF_INET) {
			first_family = AF_INET;
		} else if (IS_ENABLED(CONFIG_NET_IPV6) && entry->ai_family == AF_INET6) {
			second_family = AF_INET6;
		}
	}

	if (first_family == AF_INET) {
		/* If IPv4 is resolved and compiled in but not ready, then swap first and second
		 * families to start with IPv6 if it is resolved and compiled in, otherwise, keep
		 * IPv4 and wait for IP address.
		 */
		if (!ipv4_ready() && second_family == AF_INET6) {
			first_family = AF_INET6;
			second_family = AF_INET;
		}
	} else {
		/* If IPv4 is not resolved or compiled in, select second family. Don't set AF_UNSPEC
		 * directly, because second_family can be resolved to AF_UNSPEC if IPv6 is not
		 * compiled in.
		 */
		first_family = second_family;
	}

	/* This happens if resolved families and compiled families don't match. For example, if IPv6
	 * is resolved but not compiled in.
	 */
	if (first_family == AF_UNSPEC) {
		printk("No supported address families found\n");
		return -1;
	}

	/* It is possible that the first family is not ready yet if resolved families don't match
	 * to device family, which device got first. Therefore we need to wait for the other family
	 * to be ready.
	 */
	if (!family_ready(first_family)) {
		int err;

		err = wait_for_family_ready_event(first_family);
		if (err != 0) {
			printk("Failed to wait for the first family ready event, err %d\n", err);
			return -1;
		}
	}

	fd = connect_to_family(res, first_family, last_err);
	if (fd >= 0) {
		return fd;
	}

	/* Try the second family. */
	if (second_family == AF_UNSPEC) {
		return fd;
	}

	if (!family_ready(second_family)) {
		int err;

		err = wait_for_family_ready_event(second_family);
		if (err != 0) {
			printk("Failed to wait for the second family ready event, err %d\n", err);
			return -1;
		}
	}

	fd = connect_to_family(res, second_family, last_err);
	return fd;
}

static void send_http_request(void)
{
	int err;
	int fd = -1;
	char *p;
	int bytes;
	size_t off;
	struct addrinfo *res = NULL;
	struct addrinfo hints = {
		.ai_flags = AI_NUMERICSERV, /* Let getaddrinfo() set port */
		.ai_socktype = SOCK_STREAM,
	};
	int connect_errno = 0;

	printk("Looking up %s\n", CONFIG_HTTPS_HOSTNAME);

	err = getaddrinfo(CONFIG_HTTPS_HOSTNAME, HTTPS_PORT, &hints, &res);
	if (err) {
		connect_errno = errno;
		printk("getaddrinfo() failed, err %d\n", errno);
		goto clean_up;
	}

	fd = connect_to_server(res, &connect_errno);
	if (fd < 0) {
		printk("Failed to connect to any resolved address for %s, last err: %d\n",
		       CONFIG_HTTPS_HOSTNAME, connect_errno);
		goto clean_up;
	}

	off = 0;
	do {
		bytes = send(fd, &send_buf[off], HTTP_HEAD_LEN - off, 0);
		if (bytes < 0) {
			printk("send() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;
	} while (off < HTTP_HEAD_LEN);

	printk("Sent %d bytes\n", off);

	off = 0;
	do {
		bytes = recv(fd, &recv_buf[off], RECV_BUF_SIZE - off, 0);
		if (bytes < 0) {
			printk("recv() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;
	} while (bytes != 0 /* peer closed connection */);

	printk("Received %d bytes\n", off);

	/* Make sure recv_buf is NULL terminated (for safe use with strstr) */
	if (off < sizeof(recv_buf)) {
		recv_buf[off] = '\0';
	} else {
		recv_buf[sizeof(recv_buf) - 1] = '\0';
	}

	/* Print HTTP response */
	p = strstr(recv_buf, "\r\n");
	if (p) {
		off = p - recv_buf;
		recv_buf[off + 1] = '\0';
		printk("\n>\t %s\n\n", recv_buf);
	}

	printk("Finished, closing socket.\n");

clean_up:
	if (res != NULL) {
		freeaddrinfo(res);
	}
	if (fd >= 0) {
		(void)close(fd);
	}
}

int main(void)
{
	int err;

	printk("HTTPS client sample started\n\r");

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	printk("Bringing network interface up\n");

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	err = conn_mgr_all_if_up(true);
	if (err) {
		printk("conn_mgr_all_if_up, error: %d\n", err);
		return err;
	}

	 /* Provision certificates before connecting to the network */
	err = cert_provision();
	if (err) {
		return 0;
	}

	printk("Connecting to the network\n");

	err = conn_mgr_all_if_connect(true);
	if (err) {
		printk("conn_mgr_all_if_connect, error: %d\n", err);
		return 0;
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

	k_sem_take(&network_connected_sem, K_FOREVER);

	send_http_request();

	/* A small delay for the TCP connection teardown */
	k_sleep(K_SECONDS(1));

	/* The HTTP transaction is done, take the network connection down */
	err = conn_mgr_all_if_disconnect(true);
	if (err) {
		printk("conn_mgr_all_if_disconnect, error: %d\n", err);
	}

	err = conn_mgr_all_if_down(true);
	if (err) {
		printk("conn_mgr_all_if_down, error: %d\n", err);
	}

	return 0;
}

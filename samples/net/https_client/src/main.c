/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/sys/atomic.h>
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

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK		(NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED | \
				 NET_EVENT_L4_IPV4_CONNECTED | NET_EVENT_L4_IPV4_DISCONNECTED | \
				 NET_EVENT_L4_IPV6_CONNECTED | NET_EVENT_L4_IPV6_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK	(NET_EVENT_CONN_IF_FATAL_ERROR)

static const char send_buf[] = HTTP_HEAD;
static char recv_buf[RECV_BUF_SIZE];
static K_SEM_DEFINE(network_connected_sem, 0, 1);
static K_SEM_DEFINE(ip_family_state_sem, 0, 1);
static ATOMIC_DEFINE(network_state, 3);
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

BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

enum network_state_flag {
	NETWORK_STATE_L4_CONNECTED = 0,
	NETWORK_STATE_IPV4_READY,
	NETWORK_STATE_IPV6_READY,
};

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
	atomic_clear_bit(network_state, NETWORK_STATE_L4_CONNECTED);
	atomic_clear_bit(network_state, NETWORK_STATE_IPV4_READY);
	atomic_clear_bit(network_state, NETWORK_STATE_IPV6_READY);
}

static void on_net_event_l4_connected(void)
{
	atomic_set_bit(network_state, NETWORK_STATE_L4_CONNECTED);
	k_sem_give(&network_connected_sem);
}

static bool is_ip_family_ready(int family)
{
	if (family == AF_INET) {
		return atomic_test_bit(network_state, NETWORK_STATE_IPV4_READY);
	}

	if (family == AF_INET6) {
		return atomic_test_bit(network_state, NETWORK_STATE_IPV6_READY);
	}

	return false;
}

static void wait_for_ip_family(int family)
{
	const char *family_str = (family == AF_INET6) ? "IPv6" : "IPv4";

	printk("Waiting for local %s connectivity\n", family_str);
	k_sem_take(&ip_family_state_sem, K_FOREVER);
}

static struct addrinfo *select_matching_addr(struct addrinfo *res, int *missing_family)
{
	int first_missing_family = AF_UNSPEC;
	bool ipv4_dns_found = false;
	bool ipv6_dns_found = false;
	bool ipv4_ready = is_ip_family_ready(AF_INET);
	bool ipv6_ready = is_ip_family_ready(AF_INET6);

	for (struct addrinfo *entry = res; entry != NULL; entry = entry->ai_next) {
		if ((entry->ai_family != AF_INET) && (entry->ai_family != AF_INET6)) {
			continue;
		}

		if (entry->ai_family == AF_INET) {
			ipv4_dns_found = true;
		} else {
			ipv6_dns_found = true;
		}

		if (is_ip_family_ready(entry->ai_family)) {
			return entry;
		}

		if (first_missing_family == AF_UNSPEC) {
			if ((entry->ai_family == AF_INET && IS_ENABLED(CONFIG_NET_IPV4)) ||
			    (entry->ai_family == AF_INET6 && IS_ENABLED(CONFIG_NET_IPV6))) {
				first_missing_family = entry->ai_family;
			}
		}
	}

	if (ipv4_dns_found || ipv6_dns_found) {
		printk("Resolved families/local readiness mismatch: "
		       "DNS(v4=%d v6=%d), local(v4=%d v6=%d)\n",
		       ipv4_dns_found, ipv6_dns_found, ipv4_ready, ipv6_ready);
	}

	*missing_family = first_missing_family;

	return NULL;
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
		printk("IPv4 connectivity established\n");
		atomic_set_bit(network_state, NETWORK_STATE_IPV4_READY);
		k_sem_give(&ip_family_state_sem);
		break;
	case NET_EVENT_L4_IPV4_DISCONNECTED:
		printk("IPv4 connectivity lost\n");
		atomic_clear_bit(network_state, NETWORK_STATE_IPV4_READY);
		break;
	case NET_EVENT_L4_IPV6_CONNECTED:
		printk("IPv6 connectivity established\n");
		atomic_set_bit(network_state, NETWORK_STATE_IPV6_READY);
		k_sem_give(&ip_family_state_sem);
		break;
	case NET_EVENT_L4_IPV6_DISCONNECTED:
		printk("IPv6 connectivity lost\n");
		atomic_clear_bit(network_state, NETWORK_STATE_IPV6_READY);
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

static void send_http_request(void)
{
	int err;
	int fd = -1;
	char *p;
	int bytes;
	size_t off;
	struct addrinfo *res;
	struct addrinfo *selected = NULL;
	struct addrinfo hints = {
		.ai_flags = AI_NUMERICSERV, /* Let getaddrinfo() set port */
		.ai_socktype = SOCK_STREAM,
	};
	char peer_addr[INET6_ADDRSTRLEN];

	printk("Looking up %s\n", CONFIG_HTTPS_HOSTNAME);

	err = getaddrinfo(CONFIG_HTTPS_HOSTNAME, HTTPS_PORT, &hints, &res);
	if (err) {
		printk("getaddrinfo() failed, err %d\n", errno);
		return;
	}

	const void *peer_addr_ptr = NULL;
	uint16_t peer_port = 0U;

	while (selected == NULL) {
		int missing_family = AF_UNSPEC;

		selected = select_matching_addr(res, &missing_family);
		if (selected != NULL) {
			break;
		}

		if (missing_family == AF_INET || missing_family == AF_INET6) {
			printk("No resolved address matches currently ready local family\n");
			wait_for_ip_family(missing_family);
			continue;
		}

		break;
	}

	if (selected == NULL) {
		printk("No usable resolved IP address for %s\n", CONFIG_HTTPS_HOSTNAME);
		goto clean_up;
	}

	switch (selected->ai_family) {
	case AF_INET: {
		const struct sockaddr_in *addr4 = (const struct sockaddr_in *)selected->ai_addr;

		peer_addr_ptr = &addr4->sin_addr;
		peer_port = ntohs(addr4->sin_port);
		break;
	}
	case AF_INET6: {
		const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *)selected->ai_addr;

		peer_addr_ptr = &addr6->sin6_addr;
		peer_port = ntohs(addr6->sin6_port);
		break;
	}
	default:
		printk("Unsupported address family: %d\n", selected->ai_family);
		goto clean_up;
	}

	if (!inet_ntop(selected->ai_family, peer_addr_ptr, peer_addr, sizeof(peer_addr))) {
		printk("inet_ntop() failed, err %d\n", errno);
		goto clean_up;
	}
	printk("Resolved %s (%s)\n", peer_addr, net_family2str(selected->ai_family));

	if (IS_ENABLED(CONFIG_SAMPLE_TFM_MBEDTLS)) {
		fd = socket(selected->ai_family, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
	} else {
		fd = socket(selected->ai_family, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (fd == -1) {
		printk("Failed to open socket!\n");
		goto clean_up;
	}

	/* Setup TLS socket options */
	err = tls_setup(fd);
	if (err) {
		goto clean_up;
	}

	printk("Connecting to %s:%d\n", CONFIG_HTTPS_HOSTNAME, peer_port);
	err = connect(fd, selected->ai_addr, selected->ai_addrlen);
	if (err) {
		printk("connect() failed, err: %d\n", errno);
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
	freeaddrinfo(res);
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

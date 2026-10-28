/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/tls_credentials.h>
#include <net/ready_resolve.h>

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/sys/socket.h>
#endif

#if CONFIG_MODEM_KEY_MGMT
#include <modem/modem_key_mgmt.h>
#endif

#define HTTPS_PORT		443
#define HTTP_HEAD		\
				"HEAD / HTTP/1.1\r\n"	\
				"Host: " CONFIG_HTTPS_HOSTNAME ":" STRINGIFY(HTTPS_PORT) "\r\n" \
				"Connection: close\r\n\r\n"

#define HTTP_HEAD_LEN		(sizeof(HTTP_HEAD) - 1)
#define HTTP_HDR_END		"\r\n\r\n"

#define RECV_BUF_SIZE		2048
#define TLS_SEC_TAG		42
#define RETRY_DELAY		K_SECONDS(10)

/* Macro used to subscribe to specific Zephyr NET management events. */
#define CONN_LAYER_EVENT_MASK	(NET_EVENT_CONN_IF_FATAL_ERROR)

static const char send_buf[] = HTTP_HEAD;
static char recv_buf[RECV_BUF_SIZE];
/* Certificate for `example.com` */
static const char cert[] = {
	#include "example_com_ca.pem.inc"

	/* Null terminate certificate if running Mbed TLS on the application core.
	 * Required by TLS credentials API.
	 */
	IF_ENABLED(CONFIG_TLS_CREDENTIALS, (0x00))
};

/* Zephyr NET management event callback structure. */
static struct net_mgmt_event_callback conn_cb;

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

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint64_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		printk("Fatal error received from the connectivity layer\n");
		return;
	}
}

static int server_connect(const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	int fd;
	int err;

	if (IS_ENABLED(CONFIG_SAMPLE_TFM_MBEDTLS)) {
		fd = socket(addr->sa_family, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
	} else {
		fd = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (fd == -1) {
		err = -errno;
		printk("Failed to open socket, err %d\n", err);
		return err;
	}

	err = tls_setup(fd);
	if (err) {
		err = -errno;
		(void)close(fd);
		return err;
	}

	err = connect(fd, (const struct sockaddr *)addr, addrlen);
	if (err) {
		err = -errno;
		printk("connect() failed, err %d\n", err);
		(void)close(fd);
		return err;
	}

	return fd;
}

/* Connects to the first reachable server address, retrying until it succeeds */
static int server_connect_retry(void)
{
	const struct zsock_addrinfo hints = {
		.ai_socktype = NET_SOCK_STREAM,
	};
	struct ready_resolve host = {0};
	const struct net_sockaddr *addr;
	net_socklen_t addrlen;
	int fd = -1;
	int err;

	printk("Connecting to %s\n", CONFIG_HTTPS_HOSTNAME);

	while (true) {
		err = ready_resolve(&host, CONFIG_HTTPS_HOSTNAME, NULL, &hints, HTTPS_PORT);
		while (err == 0 && fd < 0 && ready_resolve_next(&host, &addr, &addrlen) == 0) {
			fd = server_connect(addr, addrlen);
		}
		if (fd >= 0) {
			break;
		}

		/* -ENETDOWN: waiting for the network to come up */
		if (err != -ENETDOWN) {
			printk("Failed to connect to %s, retrying\n", CONFIG_HTTPS_HOSTNAME);
		}
		ready_resolve_wait_change(&host, RETRY_DELAY);
	}

	ready_resolve_free(&host);

	return fd;
}

static void send_http_request(void)
{
	int fd;
	char *p;
	int bytes;
	size_t off;

	fd = server_connect_retry();
	if (fd < 0) {
		printk("Failed to connect, err %d\n", fd);
		return;
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
	(void)close(fd);
}

int main(void)
{
	int err;

	printk("HTTPS client sample started\n\r");

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

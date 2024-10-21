/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <zephyr/net/socket_ncs.h>
#include <zephyr/net/tls_credentials.h>

#include "download_client_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(download_client, CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL);

#define SIN6(A) ((struct sockaddr_in6 *)(A))
#define SIN(A) ((struct sockaddr_in *)(A))

#define HOSTNAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE

static const char *str_family(int family)
{
	switch (family) {
	case AF_INET:
		return "IPv4";
	case AF_INET6:
		return "IPv6";
	default:
		__ASSERT(false, "Unsupported family");
		return NULL;
	}
}

int client_socket_send_timeout_set(int fd, int timeout_ms)
{
	int err;

	if (timeout_ms <= 0) {
		return 0;
	}

	struct timeval timeo = {
		.tv_sec = (timeout_ms / 1000),
		.tv_usec = (timeout_ms % 1000) * 1000,
	};

	err = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -errno;
	}

	return 0;
}

static int socket_sectag_set(int fd, const int * const sec_tag_list, uint8_t sec_tag_count)
{
	int err;
	int verify;

	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, errno %d", errno);
		return -errno;
	}

	LOG_INF("Setting up TLS credentials, sec tag count %u", sec_tag_count);
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			 sizeof(sec_tag_t) * sec_tag_count);
	if (err) {
		LOG_ERR("Failed to setup socket security tag list, errno %d", errno);
		return -errno;
	}

	return 0;
}

static int socket_tls_hostname_set(int fd, const char * const host)
{
	__ASSERT_NO_MSG(host);

	int err;

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, host,
			 strlen(host));
	if (err) {
		LOG_ERR("Failed to setup TLS hostname (%s), errno %d",
			host, errno);
		return -errno;
	}

	return 0;
}

static int socket_pdn_id_set(int fd, int pdn_id)
{
	int err;

	LOG_INF("Binding to PDN ID: %d", pdn_id);
	err = setsockopt(fd, SOL_SOCKET, SO_BINDTOPDN, &pdn_id, sizeof(pdn_id));
	if (err) {
		LOG_ERR("Failed to bind socket to PDN ID %d, err %d",
			pdn_id, errno);
		return -ENETDOWN;
	}

	return 0;
}

static bool is_ip_address(const char *hostname)
{
	struct sockaddr sa;

	if (zsock_inet_pton(AF_INET, hostname, sa.data) == 1) {
		return true;
	} else if (zsock_inet_pton(AF_INET6, hostname, sa.data) == 1) {
		return true;
	}

	return false;
}

int client_socket_host_lookup(const char * const hostname, uint8_t pdn_id, struct sockaddr *sa)
{
	int err;
	char pdnserv[4];
	char *servname = NULL;
	//char hostname[HOSTNAME_SIZE];
	struct addrinfo *ai;
	struct addrinfo hints = {};

	printk("host lookup %s, pdn id %d\n", hostname, pdn_id);

	if (pdn_id) {
		hints.ai_flags = AI_PDNSERV;
		(void)snprintf(pdnserv, sizeof(pdnserv), "%d", pdn_id);
		servname = pdnserv;
	}

#if CONFIG_NET_IPV6
	printk("IPv6\n");
	hints.ai_family = AF_INET6;
	err = getaddrinfo(hostname, servname, &hints, &ai);
	if (err) {
		LOG_DBG("Failed to resolve hostname %s on %s, err %d",
			hostname, str_family(hints.ai_family), err);
	} else {
		goto out;
	}
#endif

#if CONFIG_NET_IPV4
	printk("IPv4\n");
	hints.ai_family = AF_INET;
	err = getaddrinfo(hostname, servname, &hints, &ai);
	if (err) {
		printk("Failed to resolve hostname %s on %s, err %d\n",
			hostname, str_family(hints.ai_family), err);
	} else {
		goto out;
	}
#endif

	printk("Failed to resolve hostname %s\n", hostname);
	return -EHOSTUNREACH;
out:
	printk("End of host lookup\n");
	k_sleep(K_SECONDS(1));
	memcpy(sa, ai->ai_addr, ai->ai_addrlen);
	freeaddrinfo(ai);

	return 0;
}

int client_socket_configure_and_connect(
	int *fd, int proto, int type, uint16_t port, struct sockaddr *remote_addr,
	const char *hostname, struct download_client_host_cfg *host_cfg)
{
	int err;
	socklen_t addrlen;

	switch (remote_addr->sa_family) {
	case AF_INET6:
		SIN6(remote_addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);
		break;
	case AF_INET:
		SIN(remote_addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);
		break;
	default:
		err = -EAFNOSUPPORT;
		goto cleanup;
	}

	printk("family: %d, type: %d, proto: %d\n",
		remote_addr->sa_family, type, proto);

	*fd = socket(remote_addr->sa_family, type, proto);
	if (*fd < 0) {
		err = -errno;
		LOG_ERR("Failed to create socket, errno %d", -err);
		goto cleanup;
	}

	LOG_DBG("Socket opened, fd %d", *fd);

	if (host_cfg->pdn_id) {
		err = socket_pdn_id_set(*fd, host_cfg->pdn_id);
		if (err) {
			goto cleanup;
		}
	}

	if ((proto == IPPROTO_TLS_1_2 || proto == IPPROTO_DTLS_1_2) &&
	    (host_cfg->sec_tag_list != NULL) && (host_cfg->sec_tag_count > 0)) {
		err = socket_sectag_set(*fd, host_cfg->sec_tag_list,
					host_cfg->sec_tag_count);
		if (err) {
			goto cleanup;
		}

		if (proto == IPPROTO_TLS_1_2 &&
		    !is_ip_address(hostname)) { //TODO former dlc->host_cfg.set_tls_hostname, is this ok?
			err = socket_tls_hostname_set(*fd, hostname);
			if (err) {
				err = -errno;
				goto cleanup;
			}
		}

		if (proto == IPPROTO_DTLS_1_2 && IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_CID)) {
			/* Enable connection ID */
			uint32_t dtls_cid = TLS_DTLS_CID_ENABLED;

			err = setsockopt(*fd, SOL_TLS, TLS_DTLS_CID, &dtls_cid,
					 sizeof(dtls_cid));
			if (err) {
				err = -errno;
				LOG_ERR("Failed to enable TLS_DTLS_CID: %d", err);
				/* Not fatal, so continue */
			}
		}
	}

	if (IS_ENABLED(CONFIG_LOG)) {
		char ip_addr_str[NET_IPV6_ADDR_LEN];
		void *sin_addr;

		if (remote_addr->sa_family == AF_INET6) {
			sin_addr = &((struct sockaddr_in6 *)remote_addr)->sin6_addr;
		} else {
			sin_addr = &((struct sockaddr_in *)remote_addr)->sin_addr;
		}
		inet_ntop(remote_addr->sa_family, sin_addr,
			  ip_addr_str, sizeof(ip_addr_str));
		LOG_INF("Connecting to %s", ip_addr_str);
	}
	LOG_DBG("fd %d, addrlen %d, fam %s, port %d",
		*fd, addrlen, str_family(remote_addr->sa_family), port);

	err = connect(*fd, remote_addr, addrlen);
	if (err) {
		err = -errno;
		LOG_ERR("Unable to connect, errno %d", -err);
		/* Make sure that ECONNRESET is not returned as it has a special meaning
		 * in the download client API
		 */
		if (err == -ECONNRESET) {
			err = -ECONNREFUSED;
		}
	}

cleanup:
	if (err) {
		client_socket_close(fd);
	}

	return err;
}

int client_socket_close(int *fd)
{
	int err = 0;

	if (*fd != -1) {
		err = close(*fd);
		if (err && errno != EBADF) {
			err = -errno;
			LOG_ERR("Failed to close socket, errno %d", -err);
		}

		LOG_DBG("Socket closed, fd %d", *fd);
		*fd = -1;
	}

	return err;
}

int client_socket_send(int fd, void *buf, size_t len)
{
	int sent;
	size_t off = 0;

	while (len) {
		sent = send(fd, (uint8_t *)buf + off, len, 0);
		if (sent < 0) {
			return -errno;
		}

		off += sent;
		len -= sent;
	}

	return 0;
}

int client_socket_recv_timeout_set(int fd, int timeout_ms)
{
	int err;
	struct timeval timeo;

	if (timeout_ms <= 0) {
		return 0;
	}

	if (fd == -1) {
		return -EINVAL;
	}

	timeo.tv_sec = (timeout_ms / 1000);
	timeo.tv_usec = (timeout_ms % 1000) * 1000;

	err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -errno;
	}

	return 0;
}

ssize_t client_socket_recv(int fd, void *buf, size_t len)
{
	int err = 0;

	if (fd == -1) {
		return -EINVAL;
	}

	err = recv(fd, buf, len, 0);
	if (err < 0) {
		return -errno;
	}

	return err;
}

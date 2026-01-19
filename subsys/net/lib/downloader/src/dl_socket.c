/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/socket.h>
#include <zephyr/net/socket_ncs.h>
#include <zephyr/net/tls_credentials.h>
#include <stdio.h>
#include <stdbool.h>

#include "dl_socket.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(downloader, CONFIG_DOWNLOADER_LOG_LEVEL);

#define SIN6(A) ((struct net_sockaddr_in6 *)(A))
#define SIN(A)	((struct net_sockaddr_in *)(A))

#define MS_TO_TIMEVAL(ms) \
	.tv_sec = (ms / 1000), \
	.tv_usec = (ms % 1000) * 1000 \

static const char *str_family(int family)
{
	switch (family) {
	case NET_AF_UNSPEC:
		return "Unspec";
	case NET_AF_INET:
		return "IPv4";
	case NET_AF_INET6:
		return "IPv6";
	default:
		__ASSERT(false, "Unsupported family");
		return "Unknown";
	}
}

static int socket_sectag_set(int fd, const int *const sec_tag_list, uint8_t sec_tag_count)
{
	int err;
	int verify;

	verify = ZSOCK_TLS_PEER_VERIFY_REQUIRED;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &verify,
			       sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, errno %d", errno);
		return -errno;
	}

	LOG_INF("Setting up TLS credentials, sec tag count %u", sec_tag_count);
	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, sec_tag_list,
			       sizeof(sec_tag_t) * sec_tag_count);
	if (err) {
		LOG_ERR("Failed to setup socket security tag list, errno %d", errno);
		return -errno;
	}

	return 0;
}

static int socket_ZSOCK_TLS_HOSTNAME_set(int fd, const char *const hostname)
{
	__ASSERT_NO_MSG(hostname);

	int err;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, hostname,
			       strlen(hostname));
	if (err) {
		LOG_ERR("Failed to setup TLS hostname (%s), errno %d", hostname, errno);
		return -errno;
	}

	return 0;
}

static int socket_pdn_id_set(int fd, uint32_t pdn_id)
{
	int err;

	LOG_INF("Binding to PDN ID: %d", pdn_id);
	err = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, SO_BINDTOPDN, &pdn_id,
			       sizeof(pdn_id));
	if (err) {
		LOG_ERR("Failed to bind socket to PDN ID %d, err %d", pdn_id, errno);
		return -ENETDOWN;
	}

	return 0;
}

static int socket_dtls_cid_enable(int fd)
{
	int err;
	uint32_t dtls_cid = ZSOCK_TLS_DTLS_CID_ENABLED;

	err = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_DTLS_CID, &dtls_cid,
			       sizeof(dtls_cid));
	if (err) {
		err = -errno;
		LOG_ERR("Failed to enable ZSOCK_TLS_DTLS_CID: %d", err);
		/* Not fatal, so continue */
	}

	return err;
}

static bool is_ip_address(const char *hostname)
{
	struct net_sockaddr sa;

	if (zsock_inet_pton(NET_AF_INET, hostname, sa.data) == 1) {
		return true;
	} else if (zsock_inet_pton(NET_AF_INET6, hostname, sa.data) == 1) {
		return true;
	}

	return false;
}

static int dl_socket_host_lookup(const char *const hostname, uint32_t pdn_id,
				 struct net_sockaddr *sa, int family)
{
	int err;
	char pdnserv[4];
	char *servname = NULL;
	struct zsock_addrinfo *ai;
	struct zsock_addrinfo hints = {
		.ai_family = family,
	};

#if !defined(CONFIG_NET_IPV6)
	if (family == NET_AF_INET6) {
		return -EINVAL;
	}
#endif

	LOG_DBG("host lookup %s, pdn id %d, family %d", hostname, pdn_id, family);

	if (pdn_id) {
		hints.ai_flags = AI_PDNSERV;
		(void)snprintf(pdnserv, sizeof(pdnserv), "%d", pdn_id);
		servname = pdnserv;
	}

	err = zsock_getaddrinfo(hostname, servname, &hints, &ai);
	if (err) {
		/* We expect this to fail on IPv6 sometimes */
		LOG_INF("Failed to resolve hostname %s on %s, err %d", hostname,
			str_family(hints.ai_family), err);
		return -EHOSTUNREACH;
	}

	memcpy(sa, ai->ai_addr, ai->ai_addrlen);
	zsock_freeaddrinfo(ai);

	return 0;
}

static int dl_socket_create_and_connect(int *fd, int proto, int type, uint16_t port,
					struct net_sockaddr *remote_addr, const char *hostname,
					struct downloader_host_cfg *dl_host_cfg)
{
	int err;
	net_socklen_t addrlen;

	switch (remote_addr->sa_family) {
	case NET_AF_INET6:
		SIN6(remote_addr)->sin6_port = net_htons(port);
		addrlen = sizeof(struct net_sockaddr_in6);
		break;
	case NET_AF_INET:
		SIN(remote_addr)->sin_port = net_htons(port);
		addrlen = sizeof(struct net_sockaddr_in);
		break;
	default:
		err = -EAFNOSUPPORT;
		goto cleanup;
	}

	LOG_DBG("family: %d, type: %d, proto: %d", remote_addr->sa_family, type, proto);

	*fd = zsock_socket(remote_addr->sa_family, type, proto);
	if (*fd < 0) {
		err = -errno;
		LOG_ERR("Failed to create socket, errno %d", -err);
		goto cleanup;
	}

	LOG_DBG("Socket opened, fd %d", *fd);

	if (dl_host_cfg->pdn_id) {
		err = socket_pdn_id_set(*fd, dl_host_cfg->pdn_id);
		if (err) {
			goto cleanup;
		}
	}

	if ((proto == NET_IPPROTO_TLS_1_2 || proto == NET_IPPROTO_DTLS_1_2) &&
	    (dl_host_cfg->sec_tag_list != NULL) && (dl_host_cfg->sec_tag_count > 0)) {
		err = socket_sectag_set(*fd, dl_host_cfg->sec_tag_list, dl_host_cfg->sec_tag_count);
		if (err) {
			goto cleanup;
		}

		if (proto == NET_IPPROTO_TLS_1_2 && !is_ip_address(hostname)) {
			err = socket_ZSOCK_TLS_HOSTNAME_set(*fd, hostname);
			if (err) {
				goto cleanup;
			}
		}

		if (proto == NET_IPPROTO_DTLS_1_2 && dl_host_cfg->cid) {
			LOG_DBG("enabling CID");
			err = socket_dtls_cid_enable(*fd);
			if (err) {
				goto cleanup;
			}
		}
	}

	if (IS_ENABLED(CONFIG_LOG)) {
		char ip_addr_str[NET_IPV6_ADDR_LEN];
		void *sin_addr;

		if (remote_addr->sa_family == NET_AF_INET6) {
			sin_addr = &((struct net_sockaddr_in6 *)remote_addr)->sin6_addr;
		} else {
			sin_addr = &((struct net_sockaddr_in *)remote_addr)->sin_addr;
		}
		zsock_inet_ntop(remote_addr->sa_family, sin_addr, ip_addr_str,
				sizeof(ip_addr_str));
		LOG_INF("Connecting to %s", ip_addr_str);
	}
	LOG_DBG("fd %d, addrlen %d, fam %s, port %d", *fd, addrlen,
		str_family(remote_addr->sa_family), port);

	err = zsock_connect(*fd, remote_addr, addrlen);
	if (err) {
		err = -errno;
		/* Make sure that ECONNRESET is not returned as it has a special meaning
		 * in the downloader API
		 */
		if (err == -ECONNRESET) {
			err = -ECONNREFUSED;
		}
	}

cleanup:
	if (err) {
		dl_socket_close(fd);
	}

	return err;
}

int dl_socket_configure_and_connect(int *fd, int proto, int type, uint16_t port,
				    struct net_sockaddr *remote_addr, const char *hostname,
				    struct downloader_host_cfg *dl_host_cfg)
{
	int err = -1;
	int fam;

	if (remote_addr->sa_family) {
		goto connect;
	}

	fam = dl_host_cfg->family ? dl_host_cfg->family : NET_AF_INET6;

	err = dl_socket_host_lookup(hostname, dl_host_cfg->pdn_id, remote_addr, fam);
	if (!err) {
		goto connect;
	} else if (dl_host_cfg->family) {
		LOG_ERR("Host lookup failed for hostname %s, err %d", hostname, err);
		return err;
	}

	LOG_INF("Host lookup failed for hostname %s on IPv6 (err %d), attempting IPv4",
		hostname, err);


fallback_ipv4:
	err = dl_socket_host_lookup(hostname, dl_host_cfg->pdn_id, remote_addr, NET_AF_INET);
	if (err) {
		LOG_ERR("Host lookup failed for hostname %s, err %d", hostname, err);
		return err;
	}

connect:
	err = dl_socket_create_and_connect(fd, proto, type, port, remote_addr, hostname,
					   dl_host_cfg);
	if (err) {
		if (remote_addr->sa_family == NET_AF_INET6) {
			LOG_INF("Failed to connect on IPv6 (err %d), attempting IPv4", err);
			goto fallback_ipv4;
		}

		LOG_ERR("Failed to connect, err %d", err);
		return err;
	}

	return 0;
}

int dl_socket_close(int *fd)
{
	int err = 0;

	if (*fd >= 0) {
		err = zsock_close(*fd);
		if (err && errno != EBADF) {
			err = -errno;
			LOG_ERR("Failed to close socket, errno %d", -err);
		}

		LOG_DBG("Socket closed, fd %d", *fd);
		*fd = -1;
	}

	return err;
}

int dl_socket_send(int fd, void *buf, size_t len)
{
	int sent;
	size_t off = 0;

	while (len) {
		sent = zsock_send(fd, (uint8_t *)buf + off, len, 0);
		if (sent < 0) {
			return -errno;
		}

		off += sent;
		len -= sent;
	}

	return 0;
}

int dl_socket_send_timeout_set(int fd, uint32_t timeout_ms)
{
	int err;
	struct timeval timeo = {
		MS_TO_TIMEVAL(timeout_ms),
	};

	if (timeout_ms <= 0) {
		return 0;
	}

	err = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_SNDTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -errno;
	}

	return 0;
}

int dl_socket_recv_timeout_set(int fd, uint32_t timeout_ms)
{
	int err;
	struct timeval timeo = {
		MS_TO_TIMEVAL(timeout_ms),
	};

	if (fd == -1) {
		return -EINVAL;
	}

	err = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -errno;
	}

	return 0;
}

ssize_t dl_socket_recv(int fd, void *buf, size_t len)
{
	int err = 0;

	if (fd == -1) {
		return -EINVAL;
	}

	err = zsock_recv(fd, buf, len, 0);
	if (err < 0) {
		return -errno;
	}

	return err;
}

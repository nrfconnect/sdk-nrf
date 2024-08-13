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

#include <zephyr/logging/log.h>
#include "download_client_internal.h"

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

static int socket_recv_timeout_set(int fd, int timeout_ms)
{
	int err;

	if (timeout_ms <= 0) {
		return 0;
	}

	if (fd == -1) {
		return -1;
	}

	struct timeval timeo = {
		.tv_sec = (timeout_ms / 1000),
		.tv_usec = (timeout_ms % 1000) * 1000,
	};

	err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (err) {
		LOG_WRN("Failed to set socket timeout, errno %d", errno);
		return -1;
	}

	return 0;
}

static int socket_send_timeout_set(int fd, int timeout_ms)
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
		return -1;
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

static int socket_tls_hostname_set(int fd, const char * const hostname)
{
	__ASSERT_NO_MSG(hostname);

	char parsed_host[HOSTNAME_SIZE];
	int err;

	err = url_parse_host(hostname, parsed_host, sizeof(parsed_host));
	if (err) {
		LOG_ERR("Failed to parse host, err %d", err);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, parsed_host,
			 strlen(parsed_host));
	if (err) {
		LOG_ERR("Failed to setup TLS hostname (%s), errno %d",
			parsed_host, errno);
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


static int host_lookup(const char *host, int family, uint8_t pdn_id,
		       struct sockaddr *sa)
{
	int err;
	char pdnserv[4];
	char hostname[HOSTNAME_SIZE];
	struct addrinfo *ai;

	struct addrinfo hints = {
		.ai_family = family,
	};

	/* Extract the hostname, without protocol or port */
	err = url_parse_host(host, hostname, sizeof(hostname));
	if (err) {
		return err;
	}

	if (pdn_id) {
		hints.ai_flags = AI_PDNSERV;
		(void)snprintf(pdnserv, sizeof(pdnserv), "%d", pdn_id);
		err = getaddrinfo(hostname, pdnserv, &hints, &ai);
	} else {
		err = getaddrinfo(hostname, NULL, &hints, &ai);
	}

	if (err) {
		LOG_DBG("Failed to resolve hostname %s on %s",
			hostname, str_family(family));
		return -EHOSTUNREACH;
	}

	memcpy(sa, ai->ai_addr, ai->ai_addrlen);
	freeaddrinfo(ai);

	return 0;
}

int client_socket_configure_and_connect(struct download_client *dlc, int type, uint16_t port)
{
	int err;
	socklen_t addrlen;

	err = -ENOTSUP;
	/* Attempt IPv6 connection if configured, fallback to IPv4 on error */
	if ((dlc->config.family == AF_UNSPEC) || (dlc->config.family == AF_INET6)) {
		err = host_lookup(dlc->host, AF_INET6, dlc->config.pdn_id, &dlc->remote_addr);
		/* err is checked later */
	}

	if (((dlc->config.family == AF_UNSPEC) && err) || (dlc->config.family == AF_INET)) {
		err = host_lookup(dlc->host, AF_INET, dlc->config.pdn_id, &dlc->remote_addr);
		/* err is checked later */
	}

	if (err) {
		LOG_ERR("DNS lookup failed %s", dlc->host);
		return err;
	}

	switch (dlc->remote_addr.sa_family) {
	case AF_INET6:
		SIN6(&dlc->remote_addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);
		break;
	case AF_INET:
		SIN(&dlc->remote_addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);
		break;
	default:
		err = -EAFNOSUPPORT;
		goto cleanup;
	}

	LOG_DBG("family: %d, type: %d, proto: %d",
		dlc->remote_addr.sa_family, type, dlc->proto);

	dlc->fd = socket(dlc->remote_addr.sa_family, type, dlc->proto);
	if (dlc->fd < 0) {
		err = -errno;
		LOG_ERR("Failed to create socket, errno %d", -err);
		goto cleanup;
	}

	if (dlc->config.pdn_id) {
		err = socket_pdn_id_set(dlc->fd, dlc->config.pdn_id);
		if (err) {
			goto cleanup;
		}
	}

	if ((dlc->proto == IPPROTO_TLS_1_2 || dlc->proto == IPPROTO_DTLS_1_2) &&
	    (dlc->config.sec_tag_list != NULL) && (dlc->config.sec_tag_count > 0)) {
		err = socket_sectag_set(dlc->fd, dlc->config.sec_tag_list, dlc->config.sec_tag_count);
		if (err) {
			goto cleanup;
		}

		if (dlc->config.set_tls_hostname) {
			err = socket_tls_hostname_set(dlc->fd, dlc->host);
			if (err) {
				err = -errno;
				goto cleanup;
			}
		}

		if (dlc->proto == IPPROTO_DTLS_1_2 && IS_ENABLED(CONFIG_DOWNLOAD_CLIENT_CID)) {
			/* Enable connection ID */
			uint32_t dtls_cid = TLS_DTLS_CID_ENABLED;

			err = setsockopt(dlc->fd, SOL_TLS, TLS_DTLS_CID, &dtls_cid,
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

		if (dlc->remote_addr.sa_family == AF_INET6) {
			sin_addr = &((struct sockaddr_in6 *)&dlc->remote_addr)->sin6_addr;
		} else {
			sin_addr = &((struct sockaddr_in *)&dlc->remote_addr)->sin_addr;
		}
		inet_ntop(dlc->remote_addr.sa_family, sin_addr, ip_addr_str, sizeof(ip_addr_str));
		LOG_INF("Connecting to %s", ip_addr_str);
	}
	LOG_DBG("fd %d, addrlen %d, fam %s, port %d",
		dlc->fd, addrlen, str_family(dlc->remote_addr.sa_family), port);

	err = connect(dlc->fd, &dlc->remote_addr, addrlen);
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
		if (dlc->fd != -1) {
			close(dlc->fd);
			dlc->fd = -1;
		}
	}

	return err;
}

int client_socket_send(const struct download_client *dlc, size_t len, int timeout)
{
	int err;
	int sent;
	size_t off = 0;

	err = socket_send_timeout_set(dlc->fd, timeout);
	if (err) {
		return -errno;
	}

	while (len) {
		sent = send(dlc->fd, dlc->config.buf + off, len, 0);
		if (sent < 0) {
			return -errno;
		}

		off += sent;
		len -= sent;
	}

	return 0;
}

ssize_t client_socket_recv(struct download_client *dlc)
{
	int err, timeout = 0;

	switch (dlc->proto) {
	case IPPROTO_TCP:
	case IPPROTO_TLS_1_2:
		timeout = CONFIG_DOWNLOAD_CLIENT_TCP_SOCK_TIMEO_MS;
		break;
	case IPPROTO_UDP:
	case IPPROTO_DTLS_1_2:
		if (IS_ENABLED(CONFIG_COAP)) {
			timeout = coap_get_recv_timeout(dlc);
			if (timeout == 0) {
				errno = ETIMEDOUT;
				return -1;
			}
			break;
		}
	default:
		LOG_ERR("unhandled proto");
		return -1;
	}

	err = socket_recv_timeout_set(dlc->fd, timeout);
	if (err) {
		return -1;
	}

	return recv(dlc->fd, dlc->config.buf + dlc->offset, dlc->config.buf_size - dlc->offset, 0);
}

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#if defined(CONFIG_POSIX_API)
#include <posix/arpa/inet.h>
#include <posix/unistd.h>
#include <posix/netdb.h>
#include <posix/sys/socket.h>
#else
#include <net/socket.h>
#endif
#include <modem/lte_lc.h>
#include <net/tls_credentials.h>
#include <modem/modem_key_mgmt.h>
#include <net/multicell_location.h>

#include "location_service.h"

#include <logging/log.h>

#define TLS_SEC_TAG	CONFIG_MULTICELL_LOCATION_TLS_SEC_TAG
#define HTTPS_PORT	CONFIG_MULTICELL_LOCATION_HTTPS_PORT
#define HOSTNAME	CONFIG_MULTICELL_LOCATION_HOSTNAME

LOG_MODULE_REGISTER(multicell_location, CONFIG_MULTICELL_LOCATION_LOG_LEVEL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_MULTICELL_LOCATION_SERVICE_NONE),
	     "A location service must be enabled");

static char http_request[CONFIG_MULTICELL_LOCATION_SEND_BUF_SIZE];
static char recv_buf[CONFIG_MULTICELL_LOCATION_RECV_BUF_SIZE];

static int tls_setup(int fd)
{
	int err;
	int verify = TLS_PEER_VERIFY_REQUIRED;
	/* Security tag that we have provisioned the certificate to */
	const sec_tag_t tls_sec_tag[] = {
		CONFIG_MULTICELL_LOCATION_TLS_SEC_TAG,
	};

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, err %d", errno);
		return -errno;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag,
			 sizeof(tls_sec_tag));
	if (err) {
		LOG_ERR("Failed to setup TLS sec tag, error: %d", errno);
		return -errno;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, HOSTNAME, sizeof(HOSTNAME) - 1);
	if (err < 0) {
		LOG_ERR("Failed to set hostname option, errno: %d", errno);
		return -errno;
	}

	return 0;
}

static int execute_http_request(const char *request, size_t request_len)
{
	int err, fd, bytes;
	size_t offset = 0;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

	err = getaddrinfo(location_service_get_hostname(), NULL, &hints, &res);
	if (err) {
		LOG_ERR("getaddrinfo() failed, error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_MULTICELL_LOCATION_LOG_LEVEL_DBG)) {
		char ip[INET6_ADDRSTRLEN];

		inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr,
			  ip, sizeof(ip));
		LOG_DBG("IP address: %s", log_strdup(ip));
	}

	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(HTTPS_PORT);

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	if (fd == -1) {
		LOG_ERR("Failed to open socket, errno: %d", errno);
		err = -errno;
		goto clean_up;
	}

	/* Setup TLS socket options */
	err = tls_setup(fd);
	if (err) {
		goto clean_up;
	}

	if (CONFIG_MULTICELL_LOCATION_SEND_TIMEOUT > 0) {
		struct timeval timeout = {
			.tv_sec = CONFIG_MULTICELL_LOCATION_SEND_TIMEOUT,
		};

		err = setsockopt(fd,
				 SOL_SOCKET,
				 SO_SNDTIMEO,
				 &timeout,
				 sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to setup socket send timeout, errno %d", errno);
			err = -errno;
			goto clean_up;
		}
	}

	if (CONFIG_MULTICELL_LOCATION_RECV_TIMEOUT > 0) {
		struct timeval timeout = {
			.tv_sec = CONFIG_MULTICELL_LOCATION_RECV_TIMEOUT,
		};

		err = setsockopt(fd,
				 SOL_SOCKET,
				 SO_RCVTIMEO,
				 &timeout,
				 sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to setup socket receive timeout, errno %d", errno);
			err = -errno;
			goto clean_up;
		}
	}

	err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
	if (err) {
		LOG_ERR("connect() failed, errno: %d", errno);
		err = -errno;
		goto clean_up;
	}

	do {
		bytes = send(fd, &request[offset], request_len - offset, 0);
		if (bytes < 0) {
			LOG_ERR("send() failed, errno: %d", errno);
			err = -errno;
			goto clean_up;
		}

		offset += bytes;
	} while (offset < request_len);

	LOG_DBG("Sent %d bytes", offset);

	offset = 0;

	do {
		bytes = recv(fd, &recv_buf[offset], sizeof(recv_buf) - offset - 1, 0);
		if (bytes < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == ETIMEDOUT)) {
				LOG_WRN("Receive timeout, possibly incomplete data received");

				/* It's been observed that some services seemingly doesn't
				 * close the connection as expected, causing recv() to never
				 * return 0. In these cases we may have received all data,
				 * so we choose to break here to continue parsing while
				 * propagating the timeout information.
				 */
				err = -ETIMEDOUT;
				break;
			}

			LOG_ERR("recv() failed, errno: %d", errno);

			err = -errno;
			goto clean_up;
		} else {
			LOG_DBG("Received HTTP response chunk of %d bytes", bytes);
		}

		offset += bytes;
	} while (bytes != 0);

	recv_buf[offset] = '\0';

	LOG_DBG("Received %d bytes", offset);

	if (offset > 0) {
		LOG_DBG("HTTP response:\n%s\n", log_strdup(recv_buf));
	}

	LOG_DBG("Closing socket");

	/* Propagate timeout information */
	if (err != -ETIMEDOUT) {
		err = 0;
	}

clean_up:
	freeaddrinfo(res);
	(void)close(fd);

	return err;
}

int multicell_location_get(const struct lte_lc_cells_info *cell_data,
			   const char * const device_id,
			   struct multicell_location *location)
{
	int err;

	if ((cell_data == NULL) || (location == NULL)) {
		return -EINVAL;
	}

	if (cell_data->current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		LOG_WRN("Invalid cell ID, device may not be connected to a network");
		return -ENOENT;
	}

	if (cell_data->ncells_count > CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS) {
		LOG_WRN("Found %d neighbor cells, but %d cells will be used in location request",
			cell_data->ncells_count, CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS);
		LOG_WRN("Increase CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS to use more cells");
	}

	if (IS_ENABLED(CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD)) {
		return location_service_get_cell_location(cell_data, device_id, recv_buf,
							  sizeof(recv_buf), location);
	} else {
		err = location_service_generate_request(cell_data, device_id,
							http_request, sizeof(http_request));
		if (err) {
			LOG_ERR("Failed to generate HTTP request, error: %d", err);
			return err;
		}

		LOG_DBG("Generated request:\n%s", log_strdup(http_request));

		err = execute_http_request(http_request, strlen(http_request));
		if (err == -ETIMEDOUT) {
			LOG_WRN("Data reception timed out, parsing potentially incomplete data");
		} else if (err) {
			LOG_ERR("HTTP request failed, error: %d", err);
			return err;
		}

		err = location_service_parse_response(recv_buf, location);
		if (err) {
			LOG_ERR("Failed to parse HTTP response");
			return -ENOMSG;
		}
	}

	return 0;
}

int multicell_location_provision_certificate(bool overwrite)
{
	int err;
	bool exists;
	const char *certificate = location_service_get_certificate();

	if (certificate == NULL) {
		LOG_ERR("No certificate was provided by the location service");
		return -EFAULT;
	}

	err = modem_key_mgmt_exists(TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists);
	if (err) {
		LOG_ERR("Failed to check for certificates err %d", err);
		return err;
	}

	if (exists && overwrite) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			LOG_ERR("Failed to delete existing certificate, err %d", err);
		}
	} else if (exists && !overwrite) {
		LOG_INF("A certificate is already provisioned to sec tag %d",
			TLS_SEC_TAG);
		return 0;
	}

	LOG_INF("Provisioning certificate");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   certificate, strlen(certificate));
	if (err) {
		LOG_ERR("Failed to provision certificate, err %d", err);
		return err;
	}

	return 0;
}

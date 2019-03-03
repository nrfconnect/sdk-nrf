/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <string.h>
#include <download_client.h>
#include <net/socket.h>
#include <zephyr/types.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(http_dfu);


#define REQUEST_TEMPLATE "GET %s HTTP/1.1\r\n"\
	"Host: %s\r\n"\
	"Connection: keep-alive\r\n"\
	"Range: bytes=%d-%d\r\n\r\n"


static int resolve_and_connect(const char *const host, const char *const port,
					u32_t family, u32_t proto)
{
	int fd;

	if (host == NULL) {
		return -1;
	}

	struct addrinfo *addrinf = NULL;
	struct addrinfo hints = {
	    .ai_family = family,
	    .ai_socktype = SOCK_STREAM,
	    .ai_protocol = proto,
	};

	LOG_INF("Requesting getaddrinfo() for %s", host);

	/* DNS resolve the port. */
	int rc = getaddrinfo(host, port, &hints, &addrinf);

	if (rc < 0 || (addrinf == NULL)) {
		LOG_ERR("getaddrinfo() failed, err %d", errno);
		return -1;
	}

	struct addrinfo *addr = addrinf;
	struct sockaddr *remoteaddr;

	int addrlen = (family == AF_INET6)
			  ? sizeof(struct sockaddr_in6)
			  : sizeof(struct sockaddr_in);

	/* Open a socket based on the local address. */
	fd = socket(family, SOCK_STREAM, proto);
	if (fd >= 0) {
		/* Look for IPv4 address of the broker. */
		while (addr != NULL) {
			remoteaddr = addr->ai_addr;

			LOG_INF("Resolved address family %d\n",
				addr->ai_family);

			if (remoteaddr->sa_family == family) {
				((struct sockaddr_in *)remoteaddr)->sin_port =
					htons(80);

				LOG_HEXDUMP_INF((const uint8_t *)remoteaddr,
					addr->ai_addrlen, "Resolved addr");

				/** TODO:
				 *  Need to set security setting for HTTPS.
				 */
				rc = connect(fd, remoteaddr, addrlen);
				if (rc == 0) {
					break;
				}
			}
			addr = addr->ai_next;
		}
	}

	freeaddrinfo(addrinf);

	if (rc < 0) {
		close(fd);
		fd = -1;
	}

	return fd;
}

static void rxdata_flush(struct download_client * const client)
{
	if (client == NULL || client->host == NULL ||
		client->callback == NULL ||
		client->status == DOWNLOAD_CLIENT_STATUS_IDLE) {
		return;
	}

	int flush_len = recv(client->fd, client->resp_buf,
				CONFIG_NRF_DOWNLOAD_MAX_RESPONSE_SIZE, 0);

	LOG_INF("flush(): len %d\n", flush_len);
	if (flush_len == -1) {
		if (errno == EAGAIN) {
			LOG_ERR("flused %d\n", flush_len);
		} else {
			/* Something is wrong on the socket! */
			client->callback(client, DOWNLOAD_CLIENT_EVT_ERROR,
					errno);
		}
	}

	memset(client->resp_buf, 0, CONFIG_NRF_DOWNLOAD_MAX_RESPONSE_SIZE);
}

static int fragment_request(struct download_client * const client, bool flush,
				bool connnection_close)
{
	if (client == NULL || client->host == NULL ||
	    client->callback == NULL || client->resource == NULL) {
		LOG_ERR("request(): Invalid client object!");
		return -1;
	}

	if (flush == true) {
		rxdata_flush(client);
	}

	if (connnection_close == true) {
		LOG_DBG("request(): connection resume.");
		(void)close(client->fd);
		client->fd = -1;
		client->fd = resolve_and_connect(client->host, NULL, AF_INET,
					 IPPROTO_TCP);
		if (client->fd < 0) {
			LOG_ERR("frequest(): connect() failed, err %d",  errno);
			client->callback(client, DOWNLOAD_CLIENT_EVT_ERROR,
					ECONNRESET);
			return -1;
		}
	}

	memset(client->req_buf, 0, CONFIG_NRF_DOWNLOAD_MAX_REQUEST_SIZE);

	int request_len = snprintf(client->req_buf,
				CONFIG_NRF_DOWNLOAD_MAX_REQUEST_SIZE,
				REQUEST_TEMPLATE, client->resource,
				client->host, client->download_size,
				(client->download_size +
				CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE - 1));

	LOG_INF("request(), request length %d, state = %d\n",
		request_len, client->status);

	if ((request_len > 0) &&
		(request_len < CONFIG_NRF_DOWNLOAD_MAX_REQUEST_SIZE)) {

		client->status = DOWNLOAD_CLIENT_STATUS_DOWNLOAD_INPROGRESS;
		LOG_INF("Request: %s", client->req_buf);

		int written = 0;

		while ((written != request_len)) {
			written = send(client->fd, &client->req_buf[written],
						 (request_len - written), 0);
			if (written <= 0) {
				/** Could not send the whole of request.
				 *  Cannot continue.
				 */
				return -1;
			}
		}

	} else {
		LOG_ERR("Cannot create request, buffer too small!");
		return -1;
	}

	return 0;
}

static void request_and_notify(struct download_client * const client,
				bool flush, bool connnection_close)
{

	if (-1 == fragment_request(client, flush, connnection_close)) {
		client->status = DOWNLOAD_CLIENT_ERROR;
		client->callback(client, DOWNLOAD_CLIENT_EVT_ERROR,
					ECONNRESET);
	}
}

int download_client_init(struct download_client * const client)
{
	LOG_INF("init()\n");

	if (client == NULL || client->host == NULL ||
		client->callback == NULL ||
		client->resource == NULL) {
		LOG_ERR("init(): Invalid client object!");
		return -1;
	}

	client->fd = -1;
	client->status = DOWNLOAD_CLIENT_STATUS_IDLE;

	return 0;
}

int download_client_connect(struct download_client * const client)
{
	int fd;

	if (client == NULL || client->host == NULL ||
	    client->callback == NULL) {
		LOG_ERR("connect(): Invalid client object!");
		return -1;
	}

	if ((client->fd != -1) &&
		(client->status == DOWNLOAD_CLIENT_STATUS_CONNECTED)) {

		LOG_ERR("connect(): already connected, fd %d",
			client->fd);
		return 0;
	}

	/* TODO: Parse the post for name, port and protocol. */
	fd = resolve_and_connect(client->host, NULL, AF_INET, IPPROTO_TCP);
	if (fd < 0) {
		LOG_ERR("connect(): resolve_and_connect() failed, err %d",
			errno);
		return -1;
	}


	client->fd = fd;
	client->status = DOWNLOAD_CLIENT_STATUS_CONNECTED;

	LOG_INF("connect(): Success! State %d, fd %d",
		client->status, client->fd);

	return 0;
}

void download_client_disconnect(struct download_client * const client)
{
	if (client == NULL || client->fd < 0) {
		LOG_ERR("disconnect(): Invalid client object!");
		return;
	}

	close(client->fd);
	client->fd = -1;
	client->status = DOWNLOAD_CLIENT_STATUS_IDLE;
}

int download_client_start(struct download_client * const client)
{
	if (client == NULL || client->fd < 0 ||
		(client->status != DOWNLOAD_CLIENT_STATUS_CONNECTED)) {
		LOG_ERR("download(): Invalid client object/state!");
		return -1;
	}

	client->object_size = -1;
	return fragment_request(client, false, false);
}

void download_client_process(struct download_client * const client)
{
	int len;

	if (client == NULL || client->fd < 0 ||
		client->status != DOWNLOAD_CLIENT_STATUS_DOWNLOAD_INPROGRESS) {
		LOG_ERR("process(): Invalid client object/state!");
		return;
	}

	memset(client->resp_buf, 0, sizeof(client->resp_buf));

	len = recv(client->fd, client->resp_buf, sizeof(client->resp_buf),
			MSG_PEEK);
	LOG_DBG("process(), fd = %d, state = %d, length = %d, "
		"errno %d\n", client->fd, client->status, len, errno);

	if (len == -1) {
		if (errno != EAGAIN) {
			LOG_ERR("recv err errno %d!", errno);
			client->status = DOWNLOAD_CLIENT_ERROR;
			client->callback(client, DOWNLOAD_CLIENT_EVT_ERROR,
				ENOTCONN);
		}
		return;
	}
	if (len == 0) {
		LOG_ERR("recv returned 0, peer closed connection!");
		client->status = DOWNLOAD_CLIENT_ERROR;
		client->callback(client, DOWNLOAD_CLIENT_EVT_ERROR, ECONNRESET);
		return;
	}

	LOG_INF("Received response of size %d", len);

	int payload_size = 0;
	int total_size = 0;

	char *content_range_header = strstr(client->resp_buf,
		"Content-Range: bytes");

	if (content_range_header == NULL) {
		/* Return and wait for the header to arrive. */
		LOG_DBG("Wait for Content-Range header.");
		return;
	}

	content_range_header += strlen("Content-Range: bytes");
	char *range_str = strstr(content_range_header, " ");

	if (range_str == NULL) {
		/* Return and wait for the header to arrive. */
		LOG_DBG("Wait for Content-Range header value.");
		return;
	}

	int download_offest = atoi(range_str+1);

	if (download_offest != client->download_size) {
		LOG_DBG("Start download_size %d, expected %d",
		download_offest, client->download_size);
		/* Returned range not as expected, cannot continue. */
		client->status = DOWNLOAD_CLIENT_ERROR;
		client->callback(client, DOWNLOAD_CLIENT_EVT_ERROR, EFAULT);
		return;
	}

	char *totalsize_str = strstr(content_range_header, "/");

	if (totalsize_str == NULL) {
		/* Return and wait for size to arrive. */
		LOG_DBG("Wait for Total length.");
		return;
	}

	total_size = atoi(totalsize_str + 1);
	LOG_DBG("Total size %d", total_size);

	char *content_length_header = strstr(client->resp_buf,
						"Content-Length: ");

	if (content_length_header == NULL) {
		/* Return and wait for the header to arrive. */
		LOG_DBG("Wait for Content-Length header.");
		return;
	}

	content_length_header += strlen("Content-Length: ");
	payload_size = atoi(content_length_header);

	if (total_size)	{
		if (client->object_size == -1) {
			client->object_size = total_size;
		} else  if (client->object_size != total_size) {
			LOG_ERR("Firmware size changed from %d to %d during "
				"download!", client->object_size, total_size);
			client->status = DOWNLOAD_CLIENT_ERROR;
			client->callback(client, DOWNLOAD_CLIENT_EVT_ERROR,
			EFAULT);
		}
	}

	/** Allow a full sized fragment, except the last one. The last one can
	 *  be smaller.
	 */
	if ((payload_size != CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE) &&
		(client->download_size + payload_size != client->object_size)) {
		LOG_DBG("Wait for payload.");
		return;
	}

	char *payload = strstr(client->resp_buf, "\r\n\r\n");
	bool connection_resume = false;

	if (payload == NULL) {
		LOG_DBG("Wait for payload.");
		return;
	}
	payload += strlen("\r\n\r\n");
	int expected_payload_size = len - (int)(payload - client->resp_buf);

	if (payload_size != expected_payload_size) {
		/* Wait for entire payload. */
		LOG_DBG("Expected payload %d, received %d\n", payload_size,
			expected_payload_size);
		return;
	}

	char *connnection_header = strstr(client->resp_buf, "Connection: ");

	if (connnection_header != NULL) {
		connnection_header += strlen("Connection: ");
		char *connnection_close = strstr(connnection_header, "close");

		if (connnection_close != NULL) {
			LOG_INF("Resume TCP connection\n");
			connection_resume = true;
		}
	}

	/** Parse the response, send the firmware to the modem.
	 *  And generate the right events.
	 */
	client->fragment = payload;
	client->fragment_size = payload_size;

	/** Continue download if application returns success,
	 *  else, halt.
	 */
	if (0 ==
	client->callback(client, DOWNLOAD_CLIENT_EVT_DOWNLOAD_FRAG, 0)) {

		client->download_size += payload_size;

		if (client->download_size == client->object_size) {
			client->status =
				DOWNLOAD_CLIENT_STATUS_DOWNLOAD_COMPLETE;
			client->callback(client,
				DOWNLOAD_CLIENT_EVT_DOWNLOAD_DONE,
				0);
			rxdata_flush(client);
		} else {
			request_and_notify(client, true, connection_resume);
		}
	} else {
		client->download_size -= payload_size;
		client->status = DOWNLOAD_CLIENT_STATUS_HALTED;
	}
}

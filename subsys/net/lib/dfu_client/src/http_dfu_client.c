/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <string.h>
#include <http_client.h>
#include <dfu_client.h>
#include <net/socket.h>
#include <zephyr/types.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(http_dfu);

#define REQUEST_TEMPLATE "GET /nordic-firmware-files/749fc958-b75b-4ca1-8161-bcf564530aac HTTP/1.1\r\n"\
	"Host: s3.amazonaws.com\r\n"\
	"Range: bytes=%d-%d\r\n\r\n"


int dfu_client_init(struct dfu_client_object *const dfu)
{
	LOG_INF("dfu_client_init()\n");

	if (dfu == NULL || dfu->host == NULL || dfu->callback == NULL) {
		return -1;
	}

	return 0;
}

int dfu_client_connect(struct dfu_client_object *const dfu)
{
	int fd;

	if (dfu == NULL || dfu->host == NULL || dfu->callback == NULL) {
		return -1;
	}

	/* TODO: Parse the post for name, port and protocol. */
	fd = httpc_connect(dfu->host, NULL, AF_INET, IPPROTO_TCP);
	if (fd < 0) {
		LOG_ERR("httpc_connect() failed, err %d", errno);
		return -1;
	}

	dfu->fd = fd;

	return 0;
}

void dfu_client_disconnect(struct dfu_client_object *const dfu)
{
	if (dfu == NULL || dfu->fd < 0) {
		return;
	}

	close(dfu->fd);
	dfu->fd = -1;
}

int dfu_client_download(struct dfu_client_object *const dfu)
{
	if (dfu == NULL || dfu->fd < 0) {
		return -1;
	}

	dfu->firmware_size = -1;

	int request_len = snprintf (dfu->req_buf, CONFIG_NRF_DFU_HTTP_MAX_REQUEST_SIZE, REQUEST_TEMPLATE, dfu->offset, dfu->offset+1023);

	if (request_len > 0) {
		return httpc_request(dfu->fd, dfu->req_buf, request_len);
	} else {
		return -1;
	}
}

void dfu_client_process(struct dfu_client_object *const dfu)
{
	int len;
	int sent;

	if (dfu == NULL || dfu->fd < 0) {
		return;
	}

	len = httpc_recv(dfu->fd, dfu->resp_buf, sizeof(dfu->resp_buf),MSG_PEEK);
	if (len ==  -1) {
		dfu->callback(dfu, DFU_CLIENT_EVT_ERROR, ENOTCONN);
		return;
	}
    if (len ==  0) {
		dfu->callback(dfu, DFU_CLIENT_EVT_ERROR, ECONNRESET);
		return;
	}

	LOG_INF("Received response of size %d", len);

	int payload_size = 0;
	int total_size = 0;

	char * content_range_header = strstr(dfu->resp_buf, "Content-Range: bytes");
	if (content_range_header != NULL) {
		content_range_header += strlen("Content-Range: bytes");
	    char * p_start_range = strstr(content_range_header, " ");

	    if (p_start_range != NULL) {
	    	int start_offset = atoi(p_start_range+1);
	    	LOG_DBG("Start offset %d, expected %d", start_offset, dfu->offset);
	    }
	    char * p_totalsize = strstr(content_range_header, "/");
	    if (p_totalsize != NULL) {
	    	total_size = atoi(p_totalsize+1);
	    	LOG_DBG("Total size %d", total_size);
	    }
	}

	char * content_length_header = strstr(dfu->resp_buf, "Content-Length: ");
	if (content_length_header != NULL) {
		content_length_header += strlen("Content-Length: ");
    	payload_size = atoi(content_length_header);
	}

	if (total_size)	{
		if (dfu->firmware_size == -1) {
			dfu->firmware_size = total_size;
		} else {
			if (dfu->firmware_size != total_size) {
				LOG_ERR("firmware size changed from %d to %d during downloads?!", dfu->firmware_size, total_size);
			}
		}
	}

	/* Allow a full sized fragment, except the last one which can be smaller. */
	if ((payload_size == CONFIG_NRF_DFU_HTTP_MAX_FRAGMENT_SIZE) ||
		(dfu->offset + payload_size == dfu->firmware_size))	{
		len = httpc_recv(dfu->fd, dfu->resp_buf, sizeof(dfu->resp_buf),0);
		if (len ==  -1) {
			dfu->callback(dfu, DFU_CLIENT_EVT_ERROR, 0);
			return;
		}
	}
	else {
		len = httpc_recv(dfu->fd, dfu->resp_buf, sizeof(dfu->resp_buf),0);
		dfu_client_download(dfu);
		return;
	}

	char * p_payload = strstr(dfu->resp_buf, "\r\n\r\n");

	if ((p_payload != NULL) && (payload_size)) {
		p_payload += strlen("\r\n\r\n");

		/* Parse the response, send the firmware to the modem. and
		 * generate the right events.
		 */
		dfu->fragment = p_payload;
		dfu->fragment_size = payload_size;
		if (dfu->offset == dfu->firmware_size) {
			if (dfu->callback(dfu, DFU_CLIENT_EVT_DOWNLOAD_DONE, 0) == 0) {
				// Continue download
				dfu_client_download(dfu);
			}
		}
		else {
			if (dfu->callback(dfu, DFU_CLIENT_EVT_DOWNLOAD_FRAG, 0) == 0) {
				// Continue download, else. Halt.
				dfu->offset += payload_size;
				dfu_client_download(dfu);
			}

		}
	}
}

void dfu_client_abort(struct dfu_client_object *dfu)
{
}

int dfu_client_apply(struct dfu_client_object *dfu)
{
	return 0;
}

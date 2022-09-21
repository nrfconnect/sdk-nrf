/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/parser.h>

#include <zephyr/logging/log.h>

#include <net/rest_client.h>

LOG_MODULE_REGISTER(rest_client, CONFIG_REST_CLIENT_LOG_LEVEL);

#define HTTP_PROTOCOL "HTTP/1.1"

static void rest_client_http_response_cb(struct http_response *rsp,
					  enum http_final_call final_data,
					  void *user_data)
{
	struct rest_client_resp_context *resp_ctx = NULL;

	if (user_data) {
		resp_ctx = (struct rest_client_resp_context *)user_data;
	}

	/* If the entire HTTP response is not received in a single "recv" call
	 * then this could be called multiple times, with a different value in
	 * rsp->body_start. Only set rest_ctx->response once, the first time,
	 * which will be the start of the body.
	 */
	if (resp_ctx) {
		if (!resp_ctx->response && rsp->body_found && rsp->body_frag_start) {
			resp_ctx->response = rsp->body_frag_start;
		}
		resp_ctx->total_response_len += rsp->data_len;
	}

	if (final_data == HTTP_DATA_MORE) {
		LOG_DBG("Partial data received(%zd bytes)", rsp->data_len);
	} else if (final_data == HTTP_DATA_FINAL) {
		if (!resp_ctx) {
			LOG_WRN("REST response context not provided");
			return;
		}
		resp_ctx->http_status_code = rsp->http_status_code;
		resp_ctx->response_len = rsp->processed;
		strcpy(resp_ctx->http_status_code_str, rsp->http_status);

		LOG_DBG("HTTP: All data received (content/total: %d/%d), status: %u %s",
			resp_ctx->response_len,
			resp_ctx->total_response_len,
			rsp->http_status_code,
			rsp->http_status);
	}
}

static int rest_client_sckt_tls_setup(int fd, const char *const tls_hostname,
				      const sec_tag_t sec_tag, int tls_peer_verify)
{
	int err;
	int verify = TLS_PEER_VERIFY_REQUIRED;
	const sec_tag_t tls_sec_tag[] = {
		sec_tag,
	};
	uint8_t cache;

	if (tls_peer_verify == TLS_PEER_VERIFY_REQUIRED ||
	    tls_peer_verify == TLS_PEER_VERIFY_OPTIONAL ||
	    tls_peer_verify == TLS_PEER_VERIFY_NONE) {
		verify = tls_peer_verify;
	}

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, error: %d", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag, sizeof(tls_sec_tag));
	if (err) {
		LOG_ERR("Failed to setup TLS sec tag, error: %d", errno);
		return err;
	}

	if (IS_ENABLED(CONFIG_REST_CLIENT_SCKT_TLS_SESSION_CACHE_IN_USE)) {
		cache = TLS_SESSION_CACHE_ENABLED;
	} else {
		cache = TLS_SESSION_CACHE_DISABLED;
	}

	err = setsockopt(fd, SOL_TLS, TLS_SESSION_CACHE, &cache, sizeof(cache));
	if (err) {
		LOG_ERR("Unable to set session cache, errno %d", errno);
		return err;
	}

	if (tls_hostname) {
		err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, tls_hostname, strlen(tls_hostname));
		if (err) {
			LOG_ERR("Failed to setup TLS hostname, error: %d", errno);
			return err;
		}
	}
	return 0;
}

static int rest_client_sckt_timeouts_set(int fd, int32_t timeout_ms)
{
	int err;
	struct timeval timeout = { 0 };

	if (timeout_ms != SYS_FOREVER_MS && timeout_ms > 0) {
		/* Send TO also affects TCP connect */
		timeout.tv_sec = timeout_ms / MSEC_PER_SEC;
		timeout.tv_usec = (timeout_ms % MSEC_PER_SEC) * USEC_PER_MSEC;
		err = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to set socket send timeout, error: %d", errno);
			return err;
		}

		err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (err) {
			LOG_ERR("Failed to set socket recv timeout, error: %d", errno);
			return err;
		}
	}
	return 0;
}

/* The time taken in this function is deducted from the given timeout value. */
static int rest_client_sckt_connect(int *const fd,
				    const char *const hostname,
				    const uint16_t port_num,
				    const sec_tag_t sec_tag,
				    int tls_peer_verify,
				    int32_t *timeout_ms)
{
	int ret;
	struct addrinfo *addr_info;
	char peer_addr[INET6_ADDRSTRLEN];
	char portstr[6] = { 0 };
	struct addrinfo hints = {
		.ai_flags = AI_NUMERICSERV, /* Let getaddrinfo() set port to addrinfo */
		.ai_family = AF_UNSPEC, /* Both IPv4 and IPv6 addresses accepted */
		.ai_socktype = SOCK_STREAM,
		.ai_next = NULL,
	};
	struct sockaddr *sa;
	int proto = 0;
	int64_t sckt_connect_start_time;
	int64_t time_used = 0;

	sckt_connect_start_time = k_uptime_get();

	/* Make sure fd is always initialized when this function is called */
	*fd = -1;

	snprintf(portstr, 6, "%d", port_num);

	LOG_DBG("Doing getaddrinfo() with connect addr %s port %s", hostname, portstr);

	ret = getaddrinfo(hostname, portstr, &hints, &addr_info);
	if (ret) {
		LOG_ERR("getaddrinfo() failed, error: %d", ret);
		return -EFAULT;
	}

	sa = addr_info->ai_addr;
	inet_ntop(sa->sa_family,
		  (void *)&((struct sockaddr_in *)sa)->sin_addr,
		  peer_addr,
		  INET6_ADDRSTRLEN);
	LOG_DBG("getaddrinfo() %s", peer_addr);

	if (*timeout_ms != SYS_FOREVER_MS) {
		/* Take time used for DNS query into account */
		time_used = k_uptime_get() - sckt_connect_start_time;

		/* Check if timeout has already elapsed */
		if (time_used >= *timeout_ms) {
			LOG_WRN("Timeout occurred during DNS query");
			return -ETIMEDOUT;
		}
	}

	proto = (sec_tag == REST_CLIENT_SEC_TAG_NO_SEC) ? IPPROTO_TCP : IPPROTO_TLS_1_2;
	*fd = socket(addr_info->ai_family, SOCK_STREAM, proto);
	if (*fd == -1) {
		LOG_ERR("Failed to open socket, error: %d", errno);
		ret = -ENOTCONN;
		goto clean_up;
	}

	if (sec_tag >= 0) {
		ret = rest_client_sckt_tls_setup(*fd, hostname, sec_tag, tls_peer_verify);
		if (ret) {
			ret = -EACCES;
			goto clean_up;
		}
	}

	ret = rest_client_sckt_timeouts_set(*fd, *timeout_ms - time_used);
	if (ret) {
		LOG_ERR("Failed to set socket timeouts, error: %d", errno);
		ret = -EINVAL;
		goto clean_up;
	}

	LOG_DBG("Connecting to %s port %s", hostname, portstr);

	ret = connect(*fd, addr_info->ai_addr, addr_info->ai_addrlen);
	if (ret) {
		LOG_ERR("Failed to connect socket, error: %d", errno);
		if (errno == ETIMEDOUT) {
			ret = -ETIMEDOUT;
		} else {
			ret = -ECONNREFUSED;
		}
		goto clean_up;
	}

	if (*timeout_ms != SYS_FOREVER_MS) {
		/* Take time used for socket connect into account */
		time_used = k_uptime_get() - sckt_connect_start_time;

		/* Check if timeout has already elapsed */
		if (time_used >= *timeout_ms) {
			LOG_WRN("Timeout occurred during socket connect");
			return -ETIMEDOUT;
		}
		*timeout_ms -= time_used;
	}

clean_up:

	freeaddrinfo(addr_info);
	if (ret) {
		if (*fd > -1) {
			(void)close(*fd);
			*fd = -1;
		}
	}

	return ret;
}

static void rest_client_close_connection(struct rest_client_req_context *const req_ctx,
					 struct rest_client_resp_context *const resp_ctx)
{
	int ret;

	if (!req_ctx->keep_alive) {
		ret = close(req_ctx->connect_socket);
		if (ret) {
			LOG_WRN("Failed to close socket, error: %d", errno);
		}
		req_ctx->connect_socket = REST_CLIENT_SCKT_CONNECT;
	} else {
		resp_ctx->used_socket_is_alive = true;
		LOG_INF("Socket with id: %d was kept alive and wasn't closed",
			req_ctx->connect_socket);
	}
}

static void rest_client_init_request(struct rest_client_req_context *const req_ctx,
				      struct http_request *const req)
{
	memset(req, 0, sizeof(struct http_request));

	req->host = req_ctx->host;
	req->protocol = HTTP_PROTOCOL;

	req->response = rest_client_http_response_cb;
	req->method = req_ctx->http_method;
}

static int rest_client_do_api_call(struct http_request *http_req,
				   struct rest_client_req_context *const req_ctx,
				   struct rest_client_resp_context *const resp_ctx)
{
	int err = 0;

	if (req_ctx->connect_socket < 0) {
		err = rest_client_sckt_connect(&req_ctx->connect_socket,
						http_req->host,
						req_ctx->port,
						req_ctx->sec_tag,
						req_ctx->tls_peer_verify,
						&req_ctx->timeout_ms);
		if (err) {
			return err;
		}
	}

	/* Assign the user provided receive buffer into the http request */
	http_req->recv_buf = req_ctx->resp_buff;
	http_req->recv_buf_len = req_ctx->resp_buff_len;

	memset(http_req->recv_buf, 0, http_req->recv_buf_len);

	/* Ensure receive buffer stays NULL terminated */
	--http_req->recv_buf_len;

	resp_ctx->response = NULL;
	resp_ctx->response_len = 0;
	resp_ctx->total_response_len = 0;
	resp_ctx->used_socket_id = req_ctx->connect_socket;
	resp_ctx->http_status_code_str[0] = '\0';

	err = http_client_req(req_ctx->connect_socket, http_req, req_ctx->timeout_ms, resp_ctx);
	if (err < 0) {
		LOG_ERR("http_client_req() error: %d", err);
	} else if (resp_ctx->total_response_len >= req_ctx->resp_buff_len) {
		/* 1 byte is reserved to NULL terminate the response */
		LOG_ERR("Receive buffer too small, %d bytes are required",
			resp_ctx->total_response_len + 1);
		err = -ENOBUFS;
	} else {
		err = 0;
	}

	return err;
}

void rest_client_request_defaults_set(struct rest_client_req_context *req_ctx)
{
	__ASSERT_NO_MSG(req_ctx != NULL);

	req_ctx->connect_socket = REST_CLIENT_SCKT_CONNECT;
	req_ctx->keep_alive = false;
	req_ctx->sec_tag = REST_CLIENT_SEC_TAG_NO_SEC;
	req_ctx->tls_peer_verify = REST_CLIENT_TLS_DEFAULT_PEER_VERIFY;
	req_ctx->http_method = HTTP_GET;
	req_ctx->timeout_ms = CONFIG_REST_CLIENT_REQUEST_TIMEOUT * MSEC_PER_SEC;
	if (req_ctx->timeout_ms == 0) {
		req_ctx->timeout_ms = SYS_FOREVER_MS;
	}
}

int rest_client_request(struct rest_client_req_context *req_ctx,
			struct rest_client_resp_context *resp_ctx)
{
	__ASSERT_NO_MSG(req_ctx != NULL);
	__ASSERT_NO_MSG(req_ctx->host != NULL);
	__ASSERT_NO_MSG(req_ctx->url != NULL);
	__ASSERT_NO_MSG(req_ctx->resp_buff != NULL);
	__ASSERT_NO_MSG(req_ctx->resp_buff_len > 0);

	struct http_request http_req;
	int ret;

	rest_client_init_request(req_ctx, &http_req);

	http_req.url = req_ctx->url;

	LOG_DBG("Requesting destination HOST: %s at port %d, URL: %s",
		req_ctx->host, req_ctx->port, http_req.url);

	http_req.header_fields = req_ctx->header_fields;

	if (req_ctx->body != NULL) {
		http_req.payload = req_ctx->body;
		http_req.payload_len =
			req_ctx->body_len ? req_ctx->body_len : strlen(http_req.payload);

		if (req_ctx->body_len) {
			LOG_HEXDUMP_DBG(req_ctx->body, req_ctx->body_len, "Payload:");
		} else {
			LOG_DBG("Payload: %s", http_req.payload);
		}
	}

	ret = rest_client_do_api_call(&http_req, req_ctx, resp_ctx);
	if (ret) {
		LOG_ERR("rest_client_do_api_call() failed, err %d", ret);
		goto clean_up;
	}

	if (!resp_ctx->response || !resp_ctx->response_len) {
		char *end_ptr = &req_ctx->resp_buff[resp_ctx->total_response_len];

		LOG_DBG("No data in a response body");
		/* Make it as zero length string */
		*end_ptr = '\0';
		resp_ctx->response = end_ptr;
		resp_ctx->response_len = 0;
	}
	LOG_DBG("API call response len: http status: %d, %u bytes", resp_ctx->http_status_code,
		resp_ctx->response_len);

clean_up:
	if (req_ctx->connect_socket != REST_CLIENT_SCKT_CONNECT) {
		/* Socket was not closed yet: */
		rest_client_close_connection(req_ctx, resp_ctx);
	}
	return ret;
}

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket_offload.h>

#include <zephyr/ztest.h>
#include <download_client.h>

#include "mock/socket.h"
#include "mock/dl_coap.h"

static struct download_client client;
static const struct sockaddr_in addr_coap_me_http = {
	.sin_family = AF_INET,
	.sin_port = htons(80),
	.sin_addr.s4_addr = {134, 102, 218, 18},
};
static const struct sockaddr_in addr_example_com = {
	.sin_family = AF_INET,
	.sin_port = htons(5683),
	.sin_addr.s4_addr = {93, 184, 216, 34},
};

struct pipe {
	struct download_client_evt data[10];
	uint8_t wr_idx;
	uint8_t rd_idx;
};

static struct pipe event_pipe;
K_SEM_DEFINE(pipe_sem, 0, 10);

static void pipe_reset(struct pipe *pipe)
{
	memset(pipe, 0, sizeof(struct pipe));
}

static int pipe_put(struct pipe *pipe, const struct download_client_evt *evt)
{
	if (k_sem_count_get(&pipe_sem) >= 10) {
		printk("Pipe is full! Please check size!");
		return -ENOSPC;
	}

	memcpy(&pipe->data[pipe->wr_idx], evt, sizeof(struct download_client_evt));

	pipe->wr_idx++;
	pipe->wr_idx = CLAMP(pipe->wr_idx, 0,
			     (sizeof(pipe->data) / sizeof(struct download_client_evt)) - 1);

	k_sem_give(&pipe_sem);

	return 0;
}

static int pipe_get(struct pipe *pipe, struct download_client_evt *evt, k_timeout_t timeo)
{
	int err;

	err = k_sem_take(&pipe_sem, timeo);
	if (err) {
		return err;
	}

	memcpy(evt, &pipe->data[pipe->rd_idx], sizeof(struct download_client_evt));

	pipe->rd_idx++;
	pipe->rd_idx = CLAMP(pipe->rd_idx, 0,
			     (sizeof(pipe->data) / sizeof(struct download_client_evt)) - 1);

	return 0;
}

static int download_client_callback(const struct download_client_evt *event)
{
	if (event == NULL) {
		return -EINVAL;
	}

	printk("event: %d\n", event->id);
	pipe_put(&event_pipe, event);

	return 0;
}

static void mock_return_values(const char *func, int32_t *val, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		z_ztest_returns_value(func, val[i]);
	}
}

static struct download_client_evt wait_for_event(enum download_client_evt_id event,
						 k_timeout_t timeout)
{
	struct download_client_evt evt;
	int err;

	while (true) {
		err = pipe_get(&event_pipe, &evt, timeout);
		zassert_ok(err);
		if (evt.id == event) {
			break;
		}
	}
	zassert_equal(evt.id, event);
	return evt;
}

static struct download_client_evt get_next_event(k_timeout_t timeout)
{
	struct download_client_evt evt;
	int err;

	err = pipe_get(&event_pipe, &evt, timeout);
	zassert_ok(err);
	return evt;
}

static struct download_client_cfg config = {
	.pdn_id = 0,
	.frag_size_override = 0,
};

static void init(void)
{
	static bool initialized;
	int err;

	if (!initialized) {
		memset(&client, 0, sizeof(struct download_client));
		err = download_client_init(&client, download_client_callback);
		zassert_ok(err, NULL);
		initialized = true;
	}
	pipe_reset(&event_pipe);
}

static void dl_coap_start(void)
{
	static const char host[] = "coap://example.com";
	int err;

	init();
	err = download_client_get(&client, host, &config, "no.file", 0);
	zassert_ok(err, NULL);
}

static void de_init(struct download_client *client)
{
	wait_for_event(DOWNLOAD_CLIENT_EVT_CLOSED, K_SECONDS(1));
}

ZTEST_SUITE(download_client, NULL, NULL, NULL, NULL, NULL);

ZTEST(download_client, test_download_simple)
{
	int32_t recvfrom_params[] = {25, 25, 25};
	int32_t sendto_params[] = {20, 20, 20};

	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_returns_value(mock_socket_offload_connect, 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, K_SECONDS(1));

	de_init(&client);
}

ZTEST(download_client, test_connection_failed)
{
	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_returns_value(mock_socket_offload_connect, ENETUNREACH);

	dl_coap_init(75, 20);

	dl_coap_start();

	struct download_client_evt evt = get_next_event(K_FOREVER);

	zassert_equal(evt.id, DOWNLOAD_CLIENT_EVT_ERROR);
	zassert_equal(evt.error, -ENETUNREACH);

	de_init(&client);
}

ZTEST(download_client, test_getaddr_failed)
{
	int err;
	struct download_client_evt evt;

	init();

	err = download_client_get(&client, "http://unknown_domain.com/file..bin", &config, NULL, 0);
	zassert_ok(err, NULL);
	evt = get_next_event(K_FOREVER);

	zassert_equal(evt.id, DOWNLOAD_CLIENT_EVT_ERROR);
	zassert_equal(evt.error, -EHOSTUNREACH);

	de_init(&client);
}

ZTEST(download_client, test_default_proto_http)
{
	int err;
	struct download_client_evt evt;

	init();
	ztest_expect_data(mock_socket_offload_connect, addr, &addr_coap_me_http);
	ztest_returns_value(mock_socket_offload_connect, EHOSTUNREACH);
	err = download_client_get(&client, "coap.me/file..bin", &config, NULL, 0);
	zassert_ok(err, NULL);
	evt = get_next_event(K_FOREVER);

	zassert_equal(evt.id, DOWNLOAD_CLIENT_EVT_ERROR);
	zassert_equal(evt.error, -EHOSTUNREACH);

	de_init(&client);
}

ZTEST(download_client, test_wrong_address)
{
	int err;
	struct download_client_evt evt;

	init();

	/* No host or config */
	err = download_client_get(&client, NULL, NULL, NULL, 0);
	zassert_equal(err, -EINVAL);

	/* Host, but no config */
	err = download_client_get(&client, "host", NULL, NULL, 0);
	zassert_equal(err, -EINVAL);

	/* Try twice to see client does not go into unusable mode */
	err = download_client_get(&client, "host", NULL, NULL, 0);
	zassert_equal(err, -EINVAL);

	/* Correct host+config buf fails to resolve */
	err = download_client_get(&client, "host", &config, NULL, 0);
	zassert_equal(err, 0);
	evt = wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR, K_SECONDS(1));
	zassert_equal(evt.error, -EHOSTUNREACH);
	de_init(&client);

	/* IP that fails to convert */
	err = download_client_get(&client, "0.0.a", &config, NULL, 0);
	zassert_equal(err, 0);
	evt = wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR, K_SECONDS(1));
	zassert_equal(evt.error, -EHOSTUNREACH);
	de_init(&client);

	/* Unsupported protocol */
	err = download_client_get(&client, "telnet://192.0.0.1/file.bin", &config, NULL, 0);
	zassert_equal(err, 0);
	evt = wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR, K_SECONDS(1));
	zassert_equal(evt.error, -EPROTONOSUPPORT);
	de_init(&client);

}

ZTEST(download_client, test_download_reconnect_on_socket_error)
{
	int32_t recvfrom_params[] = {25, -1, 25, 25};
	int32_t sendto_params[] = {20, 20, 20, 20};
	struct download_client_evt evt;

	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_returns_value(mock_socket_offload_connect, 0);
	ztest_returns_value(mock_socket_offload_connect, 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	evt = wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR, K_SECONDS(1));
	zassert_equal(evt.error, -ECONNRESET);
	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, K_SECONDS(1));

	de_init(&client);
}

ZTEST(download_client, test_download_reconnect_on_peer_close)
{
	int32_t recvfrom_params[] = {25, 0, 25, 25};
	int32_t sendto_params[] = {20, 20, 20, 20};
	struct download_client_evt evt;

	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_returns_value(mock_socket_offload_connect, 0);
	ztest_returns_value(mock_socket_offload_connect, 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	evt = get_next_event(K_SECONDS(1));
	zassert_equal(evt.id, DOWNLOAD_CLIENT_EVT_ERROR);
	zassert_equal(evt.error, -ECONNRESET);
	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	evt = get_next_event(K_SECONDS(1));
	zassert_equal(evt.id, DOWNLOAD_CLIENT_EVT_DONE);

	de_init(&client);
}

ZTEST(download_client, test_download_ignore_duplicate_block)
{
	int32_t recvfrom_params[] = {25, 25, 25, 25};
	int32_t sendto_params[] = {20, 20, 20};
	int32_t coap_parse_retv[] = {0, 0, 1, 0};
	struct download_client_evt evt;

	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_returns_value(mock_socket_offload_connect, 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));
	mock_return_values("coap_parse", coap_parse_retv, ARRAY_SIZE(coap_parse_retv));
	override_return_values.func_coap_parse = true;

	dl_coap_start();

	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	wait_for_event(DOWNLOAD_CLIENT_EVT_FRAGMENT, K_SECONDS(1));
	evt = get_next_event(K_SECONDS(1));
	zassert_equal(evt.id, DOWNLOAD_CLIENT_EVT_DONE);

	de_init(&client);
}

ZTEST(download_client, test_download_abort_on_invalid_block)
{
	int32_t recvfrom_params[] = {25, 25, 25};
	int32_t sendto_params[] = {20, 20, 20};
	int32_t coap_parse_retv[] = {0, 0, -1};

	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_returns_value(mock_socket_offload_connect, 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));
	mock_return_values("coap_parse", coap_parse_retv, ARRAY_SIZE(coap_parse_retv));
	override_return_values.func_coap_parse = true;

	dl_coap_start();

	wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR, K_SECONDS(1));

	de_init(&client);
}

ZTEST(download_client, test_get)
{
	int err;
	int32_t recvfrom_params[] = {25, 25, 25};
	int32_t sendto_params[] = {20, 20, 20};

	ztest_expect_data(mock_socket_offload_connect, addr, &addr_example_com);
	ztest_returns_value(mock_socket_offload_connect, 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	err = download_client_get(&client, "coap://example.com/large", &config, NULL, 0);
	zassert_ok(err, NULL);
	wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, K_SECONDS(1));
	wait_for_event(DOWNLOAD_CLIENT_EVT_CLOSED, K_SECONDS(1));
}

#define TEST_SOCKET_PRIO 40
NET_SOCKET_REGISTER(mock_socket, TEST_SOCKET_PRIO, AF_UNSPEC, mock_socket_is_supported,
		    mock_socket_create);
NET_DEVICE_OFFLOAD_INIT(mock_socket, "mock_socket", mock_nrf_modem_lib_socket_offload_init, NULL,
			&mock_socket_iface_data, NULL, 0, &mock_if_api, 1280);

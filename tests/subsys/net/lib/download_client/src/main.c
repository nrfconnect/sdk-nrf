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

K_PIPE_DEFINE(event_pipe, 10, _Alignof(struct download_client_evt));
static struct download_client client;

static int download_client_callback(const struct download_client_evt *event)
{
	size_t written;

	if (event == NULL) {
		return -EINVAL;
	}

	printk("event: %d\n", event->id);
	k_pipe_put(&event_pipe, (void *)event, sizeof(*event), &written, sizeof(*event), K_FOREVER);

	return 0;
}

static void mock_return_values(const char *func, int32_t *val, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		z_ztest_returns_value(func, val[i]);
	}
}

static bool wait_for_event(enum download_client_evt_id event, uint8_t seconds)
{
	size_t read;
	struct download_client_evt evt;
	int err;

	while (true) {
		err = k_pipe_get(&event_pipe, &evt, sizeof(evt), &read, sizeof(evt),
				 K_SECONDS(seconds));
		if (err) {
			return true;
		}
		if (evt.id == event) {
			return false;
		}
	}
}

static struct download_client_evt get_next_event(void)
{
	size_t read;
	struct download_client_evt evt;
	int err;

	err = k_pipe_get(&event_pipe, &evt, sizeof(evt), &read, sizeof(evt), K_FOREVER);
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
	k_pipe_flush(&event_pipe);
}

static void dl_coap_start(void)
{
	static const char host[] = "coap://10.1.0.10";
	int err;

	init();
	err = download_client_get(&client, host, &config, "no.file", 0);
	zassert_ok(err, NULL);
}

static void de_init(struct download_client *client)
{
	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_CLOSED, 10), "");
}

ZTEST_SUITE(download_client, NULL, NULL, NULL, NULL, NULL);

ZTEST(download_client, test_download_simple)
{
	int32_t recvfrom_params[] = {25, 25, 25};
	int32_t sendto_params[] = {20, 20, 20};

	z_ztest_returns_value("mock_socket_offload_connect", 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");

	de_init(&client);
}

ZTEST(download_client, test_connection_failed)
{
	z_ztest_returns_value("mock_socket_offload_connect", ENETUNREACH);

	dl_coap_init(75, 20);

	dl_coap_start();

	struct download_client_evt evt = get_next_event();

	zassert_equal(evt.id, DOWNLOAD_CLIENT_EVT_ERROR);
	zassert_equal(evt.error, -ENETUNREACH);

	de_init(&client);
}

ZTEST(download_client, test_wrong_address)
{
	int err;

	init();

	err = download_client_get(&client, NULL, NULL, NULL, 0);
	zassert_equal(err, -EINVAL);

	err = download_client_get(&client, "host", NULL, NULL, 0);
	zassert_equal(err, -EINVAL);

	err = download_client_get(&client, "host", NULL, NULL, 0);
	zassert_equal(err, -EINVAL);

	err = download_client_get(&client, "host", &config, NULL, 0);
	zassert_equal(err, 0);
	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR, 10), "");
	de_init(&client);

	err = download_client_get(&client, "0.0.a", &config, NULL, 0);
	zassert_equal(err, 0);
	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR, 10), "");
	de_init(&client);

}

ZTEST(download_client, test_download_reconnect_on_socket_error)
{
	int32_t recvfrom_params[] = {25, -1, 25, 25};
	int32_t sendto_params[] = {20, 20, 20, 20};

	z_ztest_returns_value("mock_socket_offload_connect", 0);
	z_ztest_returns_value("mock_socket_offload_connect", 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");

	de_init(&client);
}

ZTEST(download_client, test_download_reconnect_on_peer_close)
{
	int32_t recvfrom_params[] = {25, 0, 25, 25};
	int32_t sendto_params[] = {20, 20, 20, 20};

	z_ztest_returns_value("mock_socket_offload_connect", 0);
	z_ztest_returns_value("mock_socket_offload_connect", 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");

	de_init(&client);
}

ZTEST(download_client, test_download_ignore_duplicate_block)
{
	int32_t recvfrom_params[] = {25, 25, 25, 25};
	int32_t sendto_params[] = {20, 20, 20};
	int32_t coap_parse_retv[] = {0, 0, 1, 0};

	z_ztest_returns_value("mock_socket_offload_connect", 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));
	mock_return_values("coap_parse", coap_parse_retv, ARRAY_SIZE(coap_parse_retv));
	override_return_values.func_coap_parse = true;

	dl_coap_start();

	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");

	de_init(&client);
}

ZTEST(download_client, test_download_abort_on_invalid_block)
{
	int32_t recvfrom_params[] = {25, 25, 25};
	int32_t sendto_params[] = {20, 20, 20};
	int32_t coap_parse_retv[] = {0, 0, -1};

	z_ztest_returns_value("mock_socket_offload_connect", 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));
	mock_return_values("coap_parse", coap_parse_retv, ARRAY_SIZE(coap_parse_retv));
	override_return_values.func_coap_parse = true;

	dl_coap_start();

	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR, 10), "Download must be on error");

	de_init(&client);
}

ZTEST(download_client, test_get)
{
	int err;
	int32_t recvfrom_params[] = {25, 25, 25};
	int32_t sendto_params[] = {20, 20, 20};

	z_ztest_returns_value("mock_socket_offload_connect", 0);

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	err = download_client_get(&client, "coap://192.168.1.2/large", &config, NULL, 0);
	zassert_ok(err, NULL);
	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");
	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_CLOSED, 10), "Socket must have closed");
}

#define TEST_SOCKET_PRIO 40
NET_SOCKET_REGISTER(mock_socket, TEST_SOCKET_PRIO, AF_UNSPEC, mock_socket_is_supported,
		    mock_socket_create);
NET_DEVICE_OFFLOAD_INIT(mock_socket, "mock_socket", mock_nrf_modem_lib_socket_offload_init, NULL,
			&mock_socket_iface_data, NULL, 0, &mock_if_api, 1280);

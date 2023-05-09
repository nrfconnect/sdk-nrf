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

static enum download_client_evt_id last_event = -1;
static struct download_client client;

static int download_client_callback(const struct download_client_evt *event)
{
	if (event == NULL) {
		return -EINVAL;
	}

	last_event = event->id;

	return 0;
}

static enum download_client_evt_id get_last_download_client_event(bool reset)
{
	enum download_client_evt_id event;

	event = last_event;
	if (reset) {
		last_event = -1;
	}

	return event;
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
	int i;

	for (i = 0; i < (200 * seconds); i++) {
		if (get_last_download_client_event(false) == event) {
			break;
		}

		k_sleep(K_MSEC(10));
	}

	return get_last_download_client_event(true) != event;
}

static struct download_client_cfg config = {
	.pdn_id = 0,
	.frag_size_override = 0,
};

static void dl_coap_start(void)
{
	static const char host[] = "coap://10.1.0.10";
	static bool initialized;
	int err;

	if (!initialized) {
		memset(&client, 0, sizeof(struct download_client));
		err = download_client_init(&client, download_client_callback);
		zassert_ok(err, NULL);
		initialized = true;
	}

	err = download_client_set_host(&client, host, &config);
	zassert_ok(err, NULL);

	err = download_client_start(&client, "no.file", 0);
	zassert_ok(err, NULL);
}

static void de_init(struct download_client *client)
{
	int err;

	err = download_client_disconnect(client);
	zassert_ok(err, NULL);

	wait_for_event(DOWNLOAD_CLIENT_EVT_CLOSED, 10);
}

static void test_download_simple(void)
{
	int32_t recvfrom_params[] = { 25, 25, 25 };
	int32_t sendto_params[] = { 20, 20, 20 };

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");

	de_init(&client);
}

static void test_download_reconnect_on_socket_error(void)
{
	int32_t recvfrom_params[] = { 25, -1, 25, 25 };
	int32_t sendto_params[] = { 20, 20, 20, 20 };

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");

	de_init(&client);
}

static void test_download_reconnect_on_peer_close(void)
{
	int32_t recvfrom_params[] = { 25, 0, 25, 25 };
	int32_t sendto_params[] = { 20, 20, 20, 20 };

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	dl_coap_start();

	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");

	de_init(&client);
}

static void test_download_ignore_duplicate_block(void)
{
	int32_t recvfrom_params[] = { 25, 25, 25, 25 };
	int32_t sendto_params[] = { 20, 20, 20 };
	int32_t coap_parse_retv[] = { 0, 0, 1, 0 };

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

static void test_download_abort_on_invalid_block(void)
{
	int32_t recvfrom_params[] = { 25, 25, 25 };
	int32_t sendto_params[] = { 20, 20, 20 };
	int32_t coap_parse_retv[] = { 0, 0, -1 };

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

static void test_get(void)
{
	int err;
	int32_t recvfrom_params[] = { 25, 25, 25 };
	int32_t sendto_params[] = { 20, 20, 20 };

	dl_coap_init(75, 20);

	mock_return_values("mock_socket_offload_recvfrom", recvfrom_params,
			   ARRAY_SIZE(recvfrom_params));
	mock_return_values("mock_socket_offload_sendto", sendto_params, ARRAY_SIZE(sendto_params));

	err = download_client_get(&client, "coap://192.168.1.2/large", &config, NULL, 0);
	zassert_ok(err, NULL);
	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE, 10), "Download must have finished");
	zassert_ok(wait_for_event(DOWNLOAD_CLIENT_EVT_CLOSED, 10), "Socket must have closed");
}

void test_main(void)
{
	ztest_test_suite(lib_fota_download_test, ztest_unit_test(test_download_simple),
			 ztest_unit_test(test_download_reconnect_on_socket_error),
			 ztest_unit_test(test_download_reconnect_on_peer_close),
			 ztest_unit_test(test_download_ignore_duplicate_block),
			 ztest_unit_test(test_download_abort_on_invalid_block),
			 ztest_unit_test(test_get));

	ztest_run_test_suite(lib_fota_download_test);
}

#define TEST_SOCKET_PRIO 40
NET_SOCKET_REGISTER(mock_socket, TEST_SOCKET_PRIO, AF_UNSPEC, mock_socket_is_supported,
		    mock_socket_create);
NET_DEVICE_OFFLOAD_INIT(mock_socket, "mock_socket", mock_nrf_modem_lib_socket_offload_init, NULL,
			&mock_socket_iface_data, NULL, 0, &mock_if_api, 1280);

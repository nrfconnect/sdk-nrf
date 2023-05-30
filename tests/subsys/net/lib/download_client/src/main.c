/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "stubs.h"
#include "socket_mock.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket_offload.h>
#include <download_client.h>

LOG_MODULE_REGISTER(download_client_test);

static struct download_client_evt last_event = { .id = -1 };

DEFINE_FFF_GLOBALS;

static int download_client_callback(const struct download_client_evt *event)
{
	if (event == NULL) {
		return -EINVAL;
	}

	memcpy(&last_event, event, sizeof(struct download_client_evt));

	return 0;
}

static int wait_for_event(enum download_client_evt_id event)
{
	int max_wait_cycles = 1000;

	while (max_wait_cycles-- > 0) {
		if (last_event.id == event) {
			return true;
		}

		k_sleep(K_MSEC(100));
	}

	return false;
}

static struct download_client_cfg config = {
	.pdn_id = 0,
	.frag_size_override = 0,
};

static void download_start(struct download_client *client)
{
	static const char host[] = "coap://10.1.0.10";
	int err;

	memset(client, 0, sizeof(struct download_client));
	err = download_client_init(client, download_client_callback);
	zassert_ok(err, NULL);

	err = download_client_set_host(client, host, &config);
	zassert_ok(err, NULL);

	err = download_client_start(client, "no.file", 0);
	zassert_ok(err, NULL);
}

static void download_get(struct download_client *client)
{
	static const char host[] = "coap://192.168.1.2/large";
	int err;

	memset(client, 0, sizeof(struct download_client));
	err = download_client_init(client, download_client_callback);
	zassert_ok(err, NULL);

	err = download_client_get(client, host, &config, NULL, 0);
	zassert_ok(err, NULL);
}

static void download_stop(struct download_client *client)
{
	int err;

	err = download_client_disconnect(client);
	zassert_ok(err, NULL);

	wait_for_event(DOWNLOAD_CLIENT_EVT_CLOSED);
}

static void my_suite_before(void *data)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();

	memset(&last_event, -1, sizeof(struct download_client_evt));
	test_socket_mock_teardown();
	test_stubs_teardown();
}

ZTEST_SUITE(download_client, NULL, NULL, my_suite_before, NULL, NULL);

ZTEST(download_client, test_download_ok)
{
	struct download_client client;
	size_t socket_recvfrom_ret_values[] = { 25, 25, 25 };
	size_t socket_sendto_ret_values[] = { 20, 20, 20 };

	coap_get_recv_timeout_fake.custom_fake = coap_get_recv_timeout_fake_default;
	coap_parse_fake.custom_fake = coap_parse_fake_default;
	coap_request_send_fake.custom_fake = coap_request_send_fake_default;

	test_set_ret_val_socket_offload_recvfrom(socket_recvfrom_ret_values,
						 ARRAY_SIZE(socket_recvfrom_ret_values));
	test_set_ret_val_socket_offload_sendto(socket_sendto_ret_values,
					       ARRAY_SIZE(socket_sendto_ret_values));

	download_start(&client);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE));
	zassert_equal(coap_get_recv_timeout_fake.call_count, 3);
	zassert_equal(coap_request_send_fake.call_count, 3);
	zassert_equal(coap_parse_fake.call_count, 3);

	download_stop(&client);
}

ZTEST(download_client, test_download_reconnect_on_socket_error_ok)
{
	struct download_client client;
	size_t socket_recvfrom_ret_values[] = { 25, -1, 25, 25 };
	size_t socket_sendto_ret_values[] = { 20, 20, 20, 20 };

	coap_get_recv_timeout_fake.custom_fake = coap_get_recv_timeout_fake_default;
	coap_parse_fake.custom_fake = coap_parse_fake_default;
	coap_request_send_fake.custom_fake = coap_request_send_fake_default;

	test_set_ret_val_socket_offload_recvfrom(socket_recvfrom_ret_values,
						 ARRAY_SIZE(socket_recvfrom_ret_values));
	test_set_ret_val_socket_offload_sendto(socket_sendto_ret_values,
					       ARRAY_SIZE(socket_sendto_ret_values));

	download_start(&client);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE));
	zassert_equal(coap_get_recv_timeout_fake.call_count, 4);
	zassert_equal(coap_request_send_fake.call_count, 4);
	zassert_equal(coap_parse_fake.call_count, 3);

	download_stop(&client);
}

ZTEST(download_client, test_download_reconnect_on_peer_close_ok)
{
	struct download_client client;
	size_t socket_recvfrom_ret_values[] = { 25, 0, 25, 25 };
	size_t socket_sendto_ret_values[] = { 20, 20, 20, 20 };

	coap_get_recv_timeout_fake.custom_fake = coap_get_recv_timeout_fake_default;
	coap_parse_fake.custom_fake = coap_parse_fake_default;
	coap_request_send_fake.custom_fake = coap_request_send_fake_default;

	test_set_ret_val_socket_offload_recvfrom(socket_recvfrom_ret_values,
						 ARRAY_SIZE(socket_recvfrom_ret_values));
	test_set_ret_val_socket_offload_sendto(socket_sendto_ret_values,
					       ARRAY_SIZE(socket_sendto_ret_values));

	download_start(&client);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE));
	zassert_equal(coap_get_recv_timeout_fake.call_count, 4);
	zassert_equal(coap_request_send_fake.call_count, 4);
	zassert_equal(coap_parse_fake.call_count, 3);

	download_stop(&client);
}

ZTEST(download_client, test_download_ignore_duplicate_block_ok)
{
	struct download_client client;
	size_t socket_recvfrom_ret_values[] = { 25, 25, 25, 25 };
	size_t socket_sendto_ret_values[] = { 20, 20, 20 };
	size_t coap_parse_retv[] = { 0, 0, 1, 0 };

	coap_get_recv_timeout_fake.custom_fake = coap_get_recv_timeout_fake_default;
	coap_parse_fake.custom_fake = coap_parse_fake_default;
	coap_request_send_fake.custom_fake = coap_request_send_fake_default;

	test_set_ret_val_socket_offload_recvfrom(socket_recvfrom_ret_values,
						 ARRAY_SIZE(socket_recvfrom_ret_values));
	test_set_ret_val_socket_offload_sendto(socket_sendto_ret_values,
					       ARRAY_SIZE(socket_sendto_ret_values));
	set_ret_val_coap_parse(coap_parse_retv, ARRAY_SIZE(coap_parse_retv));

	download_start(&client);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE));
	zassert_equal(coap_get_recv_timeout_fake.call_count, 4);
	zassert_equal(coap_request_send_fake.call_count, 3);
	zassert_equal(coap_parse_fake.call_count, 4);

	download_stop(&client);
}

ZTEST(download_client, test_download_abort_on_invalid_block_fail)
{
	struct download_client client;
	size_t socket_recvfrom_ret_values[] = { 25, 25, 25 };
	size_t socket_sendto_ret_values[] = { 20, 20, 20 };
	size_t coap_parse_retv[] = { 0, 0, -1 };

	coap_get_recv_timeout_fake.custom_fake = coap_get_recv_timeout_fake_default;
	coap_parse_fake.custom_fake = coap_parse_fake_default;
	coap_request_send_fake.custom_fake = coap_request_send_fake_default;

	test_set_ret_val_socket_offload_recvfrom(socket_recvfrom_ret_values,
						 ARRAY_SIZE(socket_recvfrom_ret_values));
	test_set_ret_val_socket_offload_sendto(socket_sendto_ret_values,
					       ARRAY_SIZE(socket_sendto_ret_values));
	set_ret_val_coap_parse(coap_parse_retv, ARRAY_SIZE(coap_parse_retv));

	download_start(&client);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR));
	zassert_equal(coap_get_recv_timeout_fake.call_count, 3);
	zassert_equal(coap_request_send_fake.call_count, 3);
	zassert_equal(coap_parse_fake.call_count, 3);

	download_stop(&client);
}

ZTEST(download_client, test_get)
{
	struct download_client client;
	size_t socket_recvfrom_ret_values[] = { 25, 25, 25 };
	size_t socket_sendto_ret_values[] = { 20, 20, 20 };

	coap_get_recv_timeout_fake.custom_fake = coap_get_recv_timeout_fake_default;
	coap_parse_fake.custom_fake = coap_parse_fake_default;
	coap_request_send_fake.custom_fake = coap_request_send_fake_default;

	test_set_ret_val_socket_offload_recvfrom(socket_recvfrom_ret_values,
						 ARRAY_SIZE(socket_recvfrom_ret_values));
	test_set_ret_val_socket_offload_sendto(socket_sendto_ret_values,
					       ARRAY_SIZE(socket_sendto_ret_values));

	download_get(&client);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE), "Download must have finished");
	zassert_equal(coap_get_recv_timeout_fake.call_count, 3);
	zassert_equal(coap_request_send_fake.call_count, 3);
	zassert_equal(coap_parse_fake.call_count, 3);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_CLOSED), "Socket must have closed");
}

ZTEST(download_client, test_download_recv_timeout_fail)
{
	struct download_client client;
	/* server does not respond, recv fails, send is still okay */
	size_t socket_recvfrom_ret_values[] = { 25, 25, -1, -1 };
	size_t socket_sendto_ret_values[] = { 20, 20, 20, 20 };
	/* first retransmission succeeds, second fails due to backoff max retries */
	size_t coap_initiate_retransmission_ret_values[] = { 0, 1 };

	coap_get_recv_timeout_fake.custom_fake = coap_get_recv_timeout_fake_default;
	coap_parse_fake.custom_fake = coap_parse_fake_default;
	coap_request_send_fake.custom_fake = coap_request_send_fake_default;
	coap_initiate_retransmission_fake.custom_fake = coap_initiate_retransmission_fake_default;

	test_set_ret_val_socket_offload_recvfrom(socket_recvfrom_ret_values,
						 ARRAY_SIZE(socket_recvfrom_ret_values));
	test_set_ret_val_socket_offload_sendto(socket_sendto_ret_values,
					       ARRAY_SIZE(socket_sendto_ret_values));
	test_set_ret_val_coap_initiate_retransmission(
		coap_initiate_retransmission_ret_values,
		ARRAY_SIZE(coap_initiate_retransmission_ret_values));

	download_start(&client);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_ERROR));
	zassert_equal(last_event.error, -ETIMEDOUT);
	zassert_equal(coap_get_recv_timeout_fake.call_count, 4);
	zassert_equal(coap_request_send_fake.call_count, 4);
	zassert_equal(coap_parse_fake.call_count, 2);
	zassert_equal(coap_initiate_retransmission_fake.call_count, 2);

	download_stop(&client);
}

ZTEST(download_client, test_download_send_timeout_reconnect_ok)
{
	struct download_client client;
	/* server does not respond, recv fails, send is still okay */
	size_t socket_recvfrom_ret_values[] = { 25, 25, 25 };
	size_t socket_sendto_ret_values[] = { 20, 20, -1, 20 };

	coap_get_recv_timeout_fake.custom_fake = coap_get_recv_timeout_fake_default;
	coap_parse_fake.custom_fake = coap_parse_fake_default;
	coap_request_send_fake.custom_fake = coap_request_send_fake_default;

	test_set_ret_val_socket_offload_recvfrom(socket_recvfrom_ret_values,
						 ARRAY_SIZE(socket_recvfrom_ret_values));
	test_set_ret_val_socket_offload_sendto(socket_sendto_ret_values,
					       ARRAY_SIZE(socket_sendto_ret_values));

	download_start(&client);

	zassert_true(wait_for_event(DOWNLOAD_CLIENT_EVT_DONE));
	zassert_equal(coap_get_recv_timeout_fake.call_count, 3);
	zassert_equal(coap_request_send_fake.call_count, 4);
	zassert_equal(coap_parse_fake.call_count, 3);

	download_stop(&client);
}

extern int mock_nrf_modem_lib_socket_offload_init(const struct device *dev);

#define TEST_SOCKET_PRIO 40
NET_SOCKET_OFFLOAD_REGISTER(mock_socket, TEST_SOCKET_PRIO, AF_UNSPEC, mock_socket_is_supported,
			    mock_socket_create);
NET_DEVICE_OFFLOAD_INIT(mock_socket, "mock_socket", mock_nrf_modem_lib_socket_offload_init, NULL,
			&mock_socket_iface_data, NULL, 0, &mock_if_api, 1500);

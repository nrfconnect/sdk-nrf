/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <modem/at_monitor.h>
#include <modem/location.h>
#include <modem/lte_lc.h>
#include <mock_nrf_modem_at.h>

#include "cmock_nrf_modem_at.h"
#include "cmock_nrf_modem_gnss.h"
#include "cmock_modem_key_mgmt.h"
#include "cmock_rest_client.h"
#include "cmock_nrf_cloud.h"
#include "cmock_nrf_cloud_rest.h"
#include "cmock_nrf_cloud_agnss.h"
#include "cmock_device.h"
#include "cmock_net_if.h"
#include "cmock_net_mgmt.h"
#include "cmock_wifi_mgmt.h"

/* NOTE: Sleep, e.g. k_sleep(K_MSEC(1)), is used after many location library API
 *       function calls because otherwise some of the threaded work in location library
 *       may not run.
 */

#if defined(CONFIG_LOCATION_METHOD_WIFI)

/* Define a dummy driver just that the linker finds it. Otherwise we get a complaint like:
 *     undefined reference to `__device_dts_ord_12'
 */
#define DT_DRV_COMPAT nordic_wlan0
DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, 0, NULL);

/* Custom mock for WiFi scan request net_mgmt_NET_REQUEST_WIFI_SCAN(). */
static struct net_if wifi_iface;
static int net_mgmt_NET_REQUEST_WIFI_SCAN_retval;
static bool net_mgmt_NET_REQUEST_WIFI_SCAN_expected;
static bool net_mgmt_NET_REQUEST_WIFI_SCAN_occurred;

/* CMock doesn't support parsing of syntax used in net_mgmt requests:
 *     extern int net_mgmt_##_mgmt_request(
 *         uint32_t mgmt_request, struct net_if *iface, void *data, size_t len)
 */
void __cmock_net_mgmt_NET_REQUEST_WIFI_SCAN_ExpectAndReturn(int retval)
{
	net_mgmt_NET_REQUEST_WIFI_SCAN_retval = retval;
}

int net_mgmt_NET_REQUEST_WIFI_SCAN(
	uint32_t mgmt_request,
	struct net_if *iface,
	void *data,
	size_t len)
{
	TEST_ASSERT_EQUAL(false, net_mgmt_NET_REQUEST_WIFI_SCAN_occurred);

	TEST_ASSERT_EQUAL(NET_REQUEST_WIFI_SCAN, mgmt_request);
	TEST_ASSERT_EQUAL_PTR(&wifi_iface, iface);
	TEST_ASSERT_NULL(data);
	TEST_ASSERT_EQUAL(0, len);

	net_mgmt_NET_REQUEST_WIFI_SCAN_occurred = true;

	return net_mgmt_NET_REQUEST_WIFI_SCAN_retval;
}

#endif /* defined(CONFIG_LOCATION_METHOD_WIFI) */

#define HTTPS_PORT 443

static struct location_event_data test_location_event_data[5] = {0};
static struct nrf_modem_gnss_pvt_data_frame test_pvt_data = {0};
static struct rest_client_req_context rest_req_ctx = { 0 };
static struct rest_client_resp_context rest_resp_ctx = { 0 };
static int location_cb_occurred;
static int location_cb_expected;
static int location_cb_occurred_2;
static int location_cb_expected_2;

K_SEM_DEFINE(event_handler_called_sem, 0, 1);
K_SEM_DEFINE(event_handler_called_sem_2, 0, 1);

/* Strings for GNSS positioning */
#if !defined(CONFIG_LOCATION_TEST_AGNSS)
static const char xmonitor_resp[] =
	"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"0140\",7,20,\"001F8414\","
	"334,6200,66,44,\"\","
	"\"11100000\",\"00010011\",\"01001001\"";

static const char xmonitor_resp_psm_on[] =
	"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"0140\",7,20,\"001F8414\","
	"334,6200,66,44,\"\","
	"\"00100001\",\"00101001\",\"01001001\"";
#endif

#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
/* PDN active response */
static const char cgact_resp_active[] = "+CGACT: 0,1";
#endif

/* Strings for cellular positioning */
static const char ncellmeas_resp_pci1[] =
	"%NCELLMEAS:0,\"00011B07\",\"26295\",\"00B7\",2300,7,63,31,"
	"150344527,2300,8,60,29,0,2400,11,55,26,184\r\n";

#if !defined(CONFIG_LOCATION_METHOD_WIFI)

static const char ncellmeas_resp_gci1[] =
	"%NCELLMEAS:0,\"00011B07\",\"26295\",\"00B7\",10512,9034,2300,7,63,31,150344527,1,0,"
	"\"00011B08\",\"26295\",\"00B7\",65535,0,2300,9,62,30,150345527,0,0\r\n";

static const char ncellmeas_resp_gci5[] =
	"%NCELLMEAS:0,\"00011B07\",\"26295\",\"00B7\",10512,9034,2300,7,63,31,150344527,1,0,"
	"\"00011B66\",\"26287\",\"00C3\",65535,0,4300,6,71,30,150345527,0,0,"
	"\"0002ABCD\",\"26287\",\"00C3\",65535,0,4300,6,71,30,150345527,0,0,"
	"\"00103425\",\"26244\",\"0056\",65535,0,6400,6,71,30,150345527,0,0,"
	"\"00076543\",\"26256\",\"00C3\",65535,0,62000,6,71,30,150345527,0,0,"
	"\"00011B08\",\"26295\",\"00B7\",65535,0,2300,9,62,30,150345527,0,0\r\n";
#endif

char http_resp[512];

static const char http_resp_header_ok[] =
	"HTTP/1.1 200 OK\r\n"
	"Server: awselb/2.0\r\n"
	"Date: Wed, 25 Aug 2021 04:50:34 GMT\r\n"
	"Content-Type: application/json\r\n"
	"Content-Length: 50\r\n"
	"Connection: close\r\n"
	"request-id: b17cd88c-8a7d-424c-a964-15a0e5d721d9\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"\r\n";

static char http_resp_body[128];

static const char http_resp_body_fmt[] =
	"{\"location\":{\"lat\":%.06f,\"lng\":%.06f,\"accuracy\":%f}}\r\n";

static char *const http_headers[] = {
	"Content-Type: application/json\r\n",
	"Connection: close\r\n",
	/* Note: Content-length set according to payload in HTTP library */
	NULL
};

/* at_monitor_dispatch() is implemented in at_monitor library and
 * we'll call it directly to fake received AT commands/notifications
 */
extern void at_monitor_dispatch(const char *at_notif);
/* method_gnss_event_handler() is implemented in the library and we'll call it directly
 * to fake received GNSS event.
 */
extern void method_gnss_event_handler(int event);

/* scan_wifi_net_mgmt_event_handler() is implemented in the library and we'll call it directly
 * to fake received Wi-Fi scan results.
 */
extern void scan_wifi_net_mgmt_event_handler(
	struct net_mgmt_event_callback *cb,
	uint32_t mgmt_event,
	struct net_if *iface);

static void helper_location_data_clear(void)
{
	memset(&test_location_event_data, 0, sizeof(test_location_event_data));
	memset(&test_pvt_data, 0, sizeof(test_pvt_data));
	memset(&rest_req_ctx, 0, sizeof(rest_req_ctx));
	memset(&rest_resp_ctx, 0, sizeof(rest_resp_ctx));

	/* Setting REST request and response parameters to default values */

	/* Set some of the defaults that are set in rest_client_request_defaults_set() */
	rest_req_ctx.connect_socket = 0;
	rest_req_ctx.keep_alive = false;
	rest_req_ctx.tls_peer_verify = REST_CLIENT_TLS_DEFAULT_PEER_VERIFY;
	rest_req_ctx.timeout_ms = 1 * 1000;

	/* Set variables that are handled in multicell location library when calling rest client */
	rest_req_ctx.http_method = HTTP_POST;
	rest_req_ctx.header_fields = (const char **)http_headers;
	rest_req_ctx.resp_buff = http_resp;
	rest_req_ctx.resp_buff_len = sizeof(http_resp);

	rest_resp_ctx.total_response_len = strlen(http_resp);
	rest_resp_ctx.response_len = strlen(http_resp + strlen(http_resp_header_ok));
	rest_resp_ctx.response = http_resp + strlen(http_resp_header_ok);
	rest_resp_ctx.http_status_code = REST_CLIENT_HTTP_STATUS_OK;
}

void setUp(void)
{
	helper_location_data_clear();
	location_cb_occurred = 0;
	location_cb_expected = 0;
	location_cb_occurred_2 = 0;
	location_cb_expected_2 = 0;
	k_sem_reset(&event_handler_called_sem);
	k_sem_reset(&event_handler_called_sem_2);

#if defined(CONFIG_LOCATION_METHOD_WIFI)
	net_mgmt_NET_REQUEST_WIFI_SCAN_retval = -1;
	net_mgmt_NET_REQUEST_WIFI_SCAN_expected = false;
	net_mgmt_NET_REQUEST_WIFI_SCAN_occurred = false;
#endif
	mock_nrf_modem_at_Init();
}

void tearDown(void)
{
	int err;

	if (location_cb_expected != location_cb_occurred) {
		/* Wait for location_event_handler call for 3 seconds.
		 * If it doesn't happen, next assert will fail the test.
		 */
		err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
		TEST_ASSERT_EQUAL(0, err);
	}
	TEST_ASSERT_EQUAL(location_cb_expected, location_cb_occurred);

	if (location_cb_expected_2 != location_cb_occurred_2) {
		/* Wait for location_event_handler_2 call for 3 seconds.
		 * If it doesn't happen, next assert will fail the test.
		 */
		err = k_sem_take(&event_handler_called_sem_2, K_SECONDS(3));
		TEST_ASSERT_EQUAL(0, err);
	}
	TEST_ASSERT_EQUAL(location_cb_expected_2, location_cb_occurred_2);
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	TEST_ASSERT_EQUAL(net_mgmt_NET_REQUEST_WIFI_SCAN_expected,
		net_mgmt_NET_REQUEST_WIFI_SCAN_occurred);
#endif
	k_sleep(K_MSEC(1));

	mock_nrf_modem_at_Verify();
}

static void location_event_data_verify(
	const struct location_event_data *expected,
	const struct location_event_data *event_data)
{
	TEST_ASSERT_EQUAL(expected->id, event_data->id);
	TEST_ASSERT_EQUAL(expected->method, event_data->method);

	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		TEST_ASSERT_EQUAL(expected->location.latitude, event_data->location.latitude);
		TEST_ASSERT_EQUAL(expected->location.longitude, event_data->location.longitude);
		TEST_ASSERT_EQUAL(expected->location.accuracy, event_data->location.accuracy);
		TEST_ASSERT_EQUAL(
			expected->location.datetime.valid,
			event_data->location.datetime.valid);

		/* Datetime verification is skipped if it's not valid.
		 * Cellular timestamps will be set but it's marked invalid because
		 * CONFIG_DATE_TIME=n. We cannot verify it anyway because it's read at runtime.
		 */
		if (event_data->location.datetime.valid) {
			TEST_ASSERT_EQUAL(
				expected->location.datetime.year,
				event_data->location.datetime.year);
			TEST_ASSERT_EQUAL(
				expected->location.datetime.month,
				event_data->location.datetime.month);
			TEST_ASSERT_EQUAL(
				expected->location.datetime.day,
				event_data->location.datetime.day);
			TEST_ASSERT_EQUAL(
				expected->location.datetime.hour,
				event_data->location.datetime.hour);
			TEST_ASSERT_EQUAL(
				expected->location.datetime.minute,
				event_data->location.datetime.minute);
			TEST_ASSERT_EQUAL(
				expected->location.datetime.second,
				event_data->location.datetime.second);
			TEST_ASSERT_EQUAL(
				expected->location.datetime.ms,
				event_data->location.datetime.ms);
		}
#if defined(CONFIG_LOCATION_DATA_DETAILS)
		if (expected->location.details.elapsed_time_method > 0) {
			TEST_ASSERT_GREATER_THAN_UINT32(
				0, event_data->location.details.elapsed_time_method);
			TEST_ASSERT_LESS_THAN_UINT32(
				1000, event_data->location.details.elapsed_time_method);
		}

		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.satellites_tracked,
			event_data->location.details.gnss.satellites_tracked);
		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.satellites_used,
			event_data->location.details.gnss.satellites_used);

		if (expected->location.details.gnss.elapsed_time_gnss > 0) {
			TEST_ASSERT_GREATER_THAN_UINT32(
				0, event_data->location.details.gnss.elapsed_time_gnss);
			TEST_ASSERT_LESS_THAN_UINT32(
				1000, event_data->location.details.gnss.elapsed_time_gnss);
		}

		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.pvt_data.flags,
			event_data->location.details.gnss.pvt_data.flags);
		TEST_ASSERT_EQUAL_DOUBLE(
			expected->location.details.gnss.pvt_data.latitude,
			event_data->location.details.gnss.pvt_data.latitude);
		TEST_ASSERT_EQUAL_DOUBLE(
			expected->location.details.gnss.pvt_data.longitude,
			event_data->location.details.gnss.pvt_data.longitude);
		TEST_ASSERT_EQUAL_DOUBLE(
			expected->location.details.gnss.pvt_data.accuracy,
			event_data->location.details.gnss.pvt_data.accuracy);
		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.pvt_data.datetime.year,
			event_data->location.details.gnss.pvt_data.datetime.year);
		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.pvt_data.datetime.month,
			event_data->location.details.gnss.pvt_data.datetime.month);
		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.pvt_data.datetime.day,
			event_data->location.details.gnss.pvt_data.datetime.day);
		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.pvt_data.datetime.hour,
			event_data->location.details.gnss.pvt_data.datetime.hour);
		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.pvt_data.datetime.minute,
			event_data->location.details.gnss.pvt_data.datetime.minute);
		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.pvt_data.datetime.seconds,
			event_data->location.details.gnss.pvt_data.datetime.seconds);
		TEST_ASSERT_EQUAL(
			expected->location.details.gnss.pvt_data.datetime.ms,
			event_data->location.details.gnss.pvt_data.datetime.ms);

		TEST_ASSERT_EQUAL(
			expected->location.details.cellular.ncells_count,
			event_data->location.details.cellular.ncells_count);
		TEST_ASSERT_EQUAL(
			expected->location.details.cellular.gci_cells_count,
			event_data->location.details.cellular.gci_cells_count);
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		TEST_ASSERT_EQUAL(
			expected->location.details.wifi.ap_count,
			event_data->location.details.wifi.ap_count);
#endif
#endif
		break;
	case LOCATION_EVT_TIMEOUT:
	case LOCATION_EVT_ERROR:
		/* TODO: Verify data: event_data->error.details*/
		break;
	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST:
		/* TODO: Verify data: event_data->agnss_request */
		break;
	case LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST:
		/* TODO: Verify data: event_data->cloud_location_request*/
		break;
	default:
		break;
	}

}

static void location_event_handler(const struct location_event_data *event_data)
{
	struct location_event_data *expected = &test_location_event_data[location_cb_occurred];

	location_event_data_verify(expected, event_data);

	location_cb_occurred++;

	k_sleep(K_MSEC(1));
	k_sem_give(&event_handler_called_sem);
}

/* 2nd event handler to test multiple event handlers. */
static void location_event_handler_2(const struct location_event_data *event_data)
{
	struct location_event_data *expected = &test_location_event_data[location_cb_occurred_2];

	location_event_data_verify(expected, event_data);

	location_cb_occurred_2++;

	k_sleep(K_MSEC(1));
	k_sem_give(&event_handler_called_sem_2);
}

/********* LOCATION INIT TESTS ***********************/

/* We need to test first init failures as initialization procedure cannot be made again
 * with different conditions once it's complete because there is no uninitialization function.
 */

/* Test event handler registration:
 * - Register NULL handler
 * - Deregister NULL handler
 * - Deregistration before handler is registered
 * - Register two handlers
 * - Deregister two handlers
 */
void test_location_handler_register(void)
{
	int ret;

	ret = location_handler_register(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	ret = location_handler_deregister(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = location_handler_deregister(location_event_handler);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	ret = location_handler_register(location_event_handler);
	TEST_ASSERT_EQUAL(0, ret);
	ret = location_handler_register(location_event_handler_2);
	TEST_ASSERT_EQUAL(0, ret);
	ret = location_handler_deregister(location_event_handler);
	TEST_ASSERT_EQUAL(0, ret);
	ret = location_handler_deregister(location_event_handler_2);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Test failure in setting gnss event handler. */
void test_location_init_fail_gnss_event_handler_set(void)
{
	int ret;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(
		&method_gnss_event_handler,
		-EPERM);

	ret = location_init(location_event_handler);
	TEST_ASSERT_EQUAL(-EPERM, ret);
}

/* Test failure in setting NULL location event handler. */
void test_location_init_fail_event_handler_set(void)
{
	int ret;

	ret = location_init(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/* Test operations before initialization:
 * - location request
 * - cancel
 */
void test_location_operations_before_init(void)
{
	int ret;

	ret = location_request(NULL);
	TEST_ASSERT_EQUAL(-EPERM, ret);

	ret = location_request_cancel();
	TEST_ASSERT_EQUAL(-EPERM, ret);
}

/* Test successful initialization. */
void test_location_init(void)
{
	int ret;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);
	// TODO: Change Ignores to Expects
	__cmock_modem_key_mgmt_exists_IgnoreAndReturn(0);
	__cmock_modem_key_mgmt_write_IgnoreAndReturn(0);

#if !defined(CONFIG_LOCATION_TEST_AGNSS)
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XMODEMSLEEP=1,0,10240", 0);
#endif

#if defined(CONFIG_LOCATION_METHOD_WIFI)
	/* __cmock_device_get_binding_ExpectAndReturn is not called for an unknown reason.
	 * __syscall in the function declaration may have something to do with it.
	 */
	__cmock_device_is_ready_IgnoreAndReturn(true);
	__cmock_net_if_lookup_by_dev_IgnoreAndReturn(&wifi_iface);
	__cmock_net_mgmt_init_event_callback_Ignore();
	__cmock_net_mgmt_add_event_callback_Ignore();

#endif
	ret = location_init(location_event_handler);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Test location_method_str() function. */
void test_location_method_str(void)
{
	const char *method;

	method = location_method_str(LOCATION_METHOD_CELLULAR);
	TEST_ASSERT_EQUAL_STRING("Cellular", method);

	method = location_method_str(LOCATION_METHOD_GNSS);
	TEST_ASSERT_EQUAL_STRING("GNSS", method);

	method = location_method_str(LOCATION_METHOD_WIFI);
	TEST_ASSERT_EQUAL_STRING("Wi-Fi", method);

	method = location_method_str(99);
	TEST_ASSERT_EQUAL_STRING("Unknown", method);
}

/********* GNSS POSITIONING TESTS ***********************/

char test_agnss_data[] = "test agnss data";

int nrf_cloud_rest_agnss_data_get_Stub(
	struct nrf_cloud_rest_context *const rest_ctx,
	struct nrf_cloud_rest_agnss_request const *const request,
	struct nrf_cloud_rest_agnss_result *const result,
	int NumCalls)
{
	struct nrf_modem_gnss_agnss_data_frame test_agnss_request = {
		.data_flags = 0x3f,
		.system_count = 2,
		.system = {
			{
				.system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
				.sv_mask_ephe = 0xffffffff,
				.sv_mask_alm = 0xffffffff
			},
			{
				.system_id = NRF_MODEM_GNSS_SYSTEM_QZSS,
				.sv_mask_ephe = 0x3ff,
				.sv_mask_alm = 0x3ff
			}
		}
	};

	TEST_ASSERT_EQUAL(-1, rest_ctx->connect_socket);
	TEST_ASSERT_EQUAL(false, rest_ctx->keep_alive);
	TEST_ASSERT_EQUAL(NRF_CLOUD_REST_TIMEOUT_NONE, rest_ctx->timeout_ms);
	TEST_ASSERT_EQUAL(0, rest_ctx->fragment_size);

	TEST_ASSERT_EQUAL(NRF_CLOUD_REST_AGNSS_REQ_CUSTOM, request->type);
	TEST_ASSERT_EQUAL(false, request->filtered);
	TEST_ASSERT_EQUAL(0, request->mask_angle);
	/* TODO: request->net_info not checked at the moment but could be done */

	TEST_ASSERT_EQUAL(test_agnss_request.data_flags, request->agnss_req->data_flags);
	TEST_ASSERT_EQUAL(test_agnss_request.system_count, request->agnss_req->system_count);
	TEST_ASSERT_EQUAL(
		test_agnss_request.system[0].system_id,
		request->agnss_req->system[0].system_id);
	TEST_ASSERT_EQUAL(
		test_agnss_request.system[0].sv_mask_ephe,
		request->agnss_req->system[0].sv_mask_ephe);
	TEST_ASSERT_EQUAL(
		test_agnss_request.system[0].sv_mask_alm,
		request->agnss_req->system[0].sv_mask_alm);
	TEST_ASSERT_EQUAL(
		test_agnss_request.system[1].system_id,
		request->agnss_req->system[1].system_id);
	TEST_ASSERT_EQUAL(
		test_agnss_request.system[1].sv_mask_ephe,
		request->agnss_req->system[1].sv_mask_ephe);
	TEST_ASSERT_EQUAL(
		test_agnss_request.system[1].sv_mask_alm,
		request->agnss_req->system[1].sv_mask_alm);

	result->buf = test_agnss_data;
	result->agnss_sz = sizeof(test_agnss_data);

	return 0;
}

/* Test successful GNSS location request. */
void test_location_gnss(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, 1, methods);
	config.methods[0].gnss.timeout = 120 * MSEC_PER_SEC;
	config.methods[0].gnss.accuracy = LOCATION_ACCURACY_NORMAL;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	location_cb_expected++;
#endif

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_GNSS_ASSISTANCE_REQUEST;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	location_cb_expected++;
#endif
	test_pvt_data.flags = NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID;
	test_pvt_data.latitude = 61.005;
	test_pvt_data.longitude = -45.997;
	test_pvt_data.accuracy = 15.83;
	test_pvt_data.datetime.year = 2021;
	test_pvt_data.datetime.month = 8;
	test_pvt_data.datetime.day = 13;
	test_pvt_data.datetime.hour = 12;
	test_pvt_data.datetime.minute = 34;
	test_pvt_data.datetime.seconds = 56;
	test_pvt_data.datetime.ms = 789;
	test_pvt_data.sv[0].sv = 2;
	test_pvt_data.sv[0].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
	test_pvt_data.sv[1].sv = 4;
	test_pvt_data.sv[1].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
	test_pvt_data.sv[2].sv = 6;
	test_pvt_data.sv[2].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
	test_pvt_data.sv[3].sv = 8;
	test_pvt_data.sv[3].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
	test_pvt_data.sv[4].sv = 10;
	test_pvt_data.sv[4].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	test_location_event_data[location_cb_expected].location.latitude = 61.005;
	test_location_event_data[location_cb_expected].location.longitude = -45.997;
	test_location_event_data[location_cb_expected].location.accuracy = 15.83;
	test_location_event_data[location_cb_expected].location.datetime.valid = true;
	test_location_event_data[location_cb_expected].location.datetime.year = 2021;
	test_location_event_data[location_cb_expected].location.datetime.month = 8;
	test_location_event_data[location_cb_expected].location.datetime.day = 13;
	test_location_event_data[location_cb_expected].location.datetime.hour = 12;
	test_location_event_data[location_cb_expected].location.datetime.minute = 34;
	test_location_event_data[location_cb_expected].location.datetime.second = 56;
	test_location_event_data[location_cb_expected].location.datetime.ms = 789;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].location.details.gnss.satellites_tracked = 5;
	test_location_event_data[location_cb_expected].location.details.gnss.satellites_used = 5;
	test_location_event_data[location_cb_expected].location.details.gnss.elapsed_time_gnss = 50;
	test_location_event_data[location_cb_expected].location.details.gnss.pvt_data =
		test_pvt_data;
#endif
	location_cb_expected++;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);

#if defined(CONFIG_LOCATION_TEST_AGNSS)
	/* Zero triggers new AGNSS data request */
	struct nrf_modem_gnss_agnss_expiry agnss_expiry = {
		.data_flags = NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST,
		.utc_expiry = 1,
		.klob_expiry = 2,
		.neq_expiry = 3,
		.integrity_expiry = 4,
		.position_expiry = 5,
		.sv_count = 6,
		.sv = {
			{
				.sv_id = 1,
				.system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
				.ephe_expiry = 1,
				.alm_expiry = 4
			},
			{
				.sv_id = 2,
				.system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
				.ephe_expiry = 100,
				.alm_expiry = 4
			},
			{
				.sv_id = 3,
				.system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
				.ephe_expiry = 0,
				.alm_expiry = 101
			},
			{
				.sv_id = 4,
				.system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
				.ephe_expiry = 6,
				.alm_expiry = 1
			},
			{
				.sv_id = 193,
				.system_id = NRF_MODEM_GNSS_SYSTEM_QZSS,
				.ephe_expiry = 0,
				.alm_expiry = 100
			},
			{
				.sv_id = 202,
				.system_id = NRF_MODEM_GNSS_SYSTEM_QZSS,
				.ephe_expiry = 100,
				.alm_expiry = 4
			},
		}
	};

	__cmock_nrf_modem_gnss_agnss_expiry_get_ExpectAndReturn(NULL, 0);
	__cmock_nrf_modem_gnss_agnss_expiry_get_IgnoreArg_agnss_expiry();
	__cmock_nrf_modem_gnss_agnss_expiry_get_ReturnMemThruPtr_agnss_expiry(
		&agnss_expiry, sizeof(agnss_expiry));
#endif
	__cmock_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__cmock_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__cmock_nrf_modem_gnss_start_ExpectAndReturn(0);

#if defined(CONFIG_LOCATION_TEST_AGNSS)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 2);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(LTE_LC_NW_REG_REGISTERED_HOME); /* Status */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0x10012002); /* Cell ID */

#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	__cmock_nrf_cloud_jwt_generate_ExpectAnyArgsAndReturn(0);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
#endif

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)

	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);

	__cmock_nrf_cloud_agnss_process_ExpectAndReturn(
		test_agnss_data, sizeof(test_agnss_data), 0);

	location_agnss_data_process(test_agnss_data, sizeof(test_agnss_data));
#else
	__cmock_nrf_cloud_rest_agnss_data_get_Stub(nrf_cloud_rest_agnss_data_get_Stub);
	__cmock_nrf_cloud_agnss_process_ExpectAndReturn(
		test_agnss_data, sizeof(test_agnss_data), 0);
#endif
#endif
	/* TODO: Cannot determine the used system mode but it's set as zero by default in lte_lc */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* LTE-M support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* NB-IoT support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* GNSS support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* LTE preference */

#if defined(CONFIG_LOCATION_TEST_AGNSS)

	/* A-GNSS gets updated current cell info using NCELLMEAS */
	at_monitor_dispatch(ncellmeas_resp_pci1);
	k_sleep(K_MSEC(1));

#else
	/* PSM is configured */
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp_psm_on, sizeof(xmonitor_resp_psm_on));

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	/* Use CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS value to trigger
	 * LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING that causes no actions
	 */
	at_monitor_dispatch("%XMODEMSLEEP: 1, 5000");
	k_sleep(K_MSEC(1));

	/* Enter PSM */
	at_monitor_dispatch("%XMODEMSLEEP: 1, 10");
	k_sleep(K_MSEC(1));
#endif
	/* An event other than NRF_MODEM_GNSS_EVT_PVT to see that no actions are done */
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_FIX);
	k_sleep(K_MSEC(1));

	__cmock_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, 0);
	__cmock_nrf_modem_gnss_read_IgnoreArg_buf();
	__cmock_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);
	k_sleep(K_MSEC(1));
}

/* Test timeout for the entire location request during GNSS.
 * A-GNSS request is skipped because it was just done (in the previous test) and
 * an hour hasn't passed.
 *
 * Note: Depends on previous test and that A-GNSS data is retrieved there.
 */
void test_location_gnss_location_request_timeout(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, 1, methods);
	config.timeout = 20;
	config.methods[0].gnss.timeout = 120 * MSEC_PER_SEC;
	config.methods[0].gnss.accuracy = LOCATION_ACCURACY_NORMAL;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	location_cb_expected++;
#endif
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_TIMEOUT;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	test_location_event_data[location_cb_expected].location.latitude = 61.005;
	test_location_event_data[location_cb_expected].location.longitude = -45.997;
	test_location_event_data[location_cb_expected].location.accuracy = 15.83;
	test_location_event_data[location_cb_expected].location.datetime.valid = true;
	test_location_event_data[location_cb_expected].location.datetime.year = 2021;
	test_location_event_data[location_cb_expected].location.datetime.month = 8;
	test_location_event_data[location_cb_expected].location.datetime.day = 13;
	test_location_event_data[location_cb_expected].location.datetime.hour = 12;
	test_location_event_data[location_cb_expected].location.datetime.minute = 34;
	test_location_event_data[location_cb_expected].location.datetime.second = 56;
	test_location_event_data[location_cb_expected].location.datetime.ms = 789;
	location_cb_expected++;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);

#if defined(CONFIG_LOCATION_TEST_AGNSS)
	/* Zero triggers new AGNSS data request */
	struct nrf_modem_gnss_agnss_expiry agnss_expiry = { 0 };

	__cmock_nrf_modem_gnss_agnss_expiry_get_ExpectAndReturn(NULL, 0);
	__cmock_nrf_modem_gnss_agnss_expiry_get_IgnoreArg_agnss_expiry();
	__cmock_nrf_modem_gnss_agnss_expiry_get_ReturnMemThruPtr_agnss_expiry(
		&agnss_expiry, sizeof(agnss_expiry));
#endif
	__cmock_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__cmock_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__cmock_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* TODO: Cannot determine the used system mode but it's set as zero by default in lte_lc */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* LTE-M support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* NB-IoT support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* GNSS support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* LTE preference */

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Wait for LOCATION_EVT_STARTED */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif

#if !defined(CONFIG_LOCATION_TEST_AGNSS)
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
#endif
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);

	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));
}

/********* CELLULAR POSITIONING TESTS ***********************/

void cellular_rest_req_resp_handle(int test_event_data_index)
{
	sprintf(http_resp_body, http_resp_body_fmt,
		test_location_event_data[test_event_data_index].location.latitude,
		test_location_event_data[test_event_data_index].location.longitude,
		(double)test_location_event_data[test_event_data_index].location.accuracy);

	sprintf(http_resp, "%s%s", http_resp_header_ok, http_resp_body);

	rest_resp_ctx.total_response_len = strlen(http_resp);
	rest_resp_ctx.response_len = strlen(http_resp + strlen(http_resp_header_ok));
	rest_resp_ctx.response = http_resp + strlen(http_resp_header_ok);

	__cmock_rest_client_request_defaults_set_ExpectAnyArgs();
	// TODO: We should verify rest_req_ctx
	//__cmock_rest_client_request_ExpectWithArrayAndReturn(&rest_req_ctx, 1, NULL, 0, 0);
	__cmock_rest_client_request_ExpectAndReturn(&rest_req_ctx, NULL, 0);
	__cmock_rest_client_request_IgnoreArg_req_ctx();
	__cmock_rest_client_request_IgnoreArg_resp_ctx();
	__cmock_rest_client_request_ReturnMemThruPtr_resp_ctx(&rest_resp_ctx,
		sizeof(rest_resp_ctx));
}

/* Test successful cellular location request utilizing HERE service.
 * Also try to make a location request while previous one is still pending.
 */
void test_location_cellular(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, 1, methods);

	config.methods[0].cellular.cell_count = 1;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	location_cb_expected++;
#endif

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	location_cb_expected++;
#endif

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	test_location_event_data[location_cb_expected].location.latitude = 61.50375;
	test_location_event_data[location_cb_expected].location.longitude = 23.896979;
	test_location_event_data[location_cb_expected].location.accuracy = 750.0;
	test_location_event_data[location_cb_expected].location.datetime.valid = false;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].location.details.cellular.ncells_count = 1;
	test_location_event_data[location_cb_expected].location.details.cellular.gci_cells_count =
		0;
#endif
	location_cb_expected++;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);

#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	cellular_rest_req_resp_handle(location_cb_expected - 1);

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;
#endif
	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Wait for LOCATION_EVT_STARTED */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif

	err = location_request(&config);
	TEST_ASSERT_EQUAL(-EBUSY, err);
	k_sleep(K_MSEC(1));

	/* Send NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp_pci1);
	k_sleep(K_MSEC(1));

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);

	struct location_data location_data = {
		.latitude = 61.50375,
		.longitude = 23.896979,
		.accuracy = 750.0,
		.datetime.valid = false
	};

	location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_SUCCESS, &location_data);
	k_sleep(K_MSEC(1));
#endif
}

/* Test cancelling cellular location request during NCELLMEAS. */
void test_location_cellular_cancel_during_ncellmeas(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, 1, methods);
	config.methods[0].cellular.cell_count = 2;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	location_cb_expected++;
#endif

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", 0);

	err = location_request_cancel();
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));
}

/* Test cellular timeout during the 1st NCELLMEAS. */
void test_location_cellular_timeout_during_1st_ncellmeas(void)
{
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, 1, methods);

	config.methods[0].cellular.cell_count = 1;
	config.methods[0].cellular.timeout = 1;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	location_cb_expected++;
#endif
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	test_location_event_data[location_cb_expected].location.latitude = 61.50375;
	test_location_event_data[location_cb_expected].location.longitude = 23.896979;
	test_location_event_data[location_cb_expected].location.accuracy = 750.0;
	test_location_event_data[location_cb_expected].location.datetime.valid = false;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].location.details.cellular.ncells_count = 1;
	test_location_event_data[location_cb_expected].location.details.cellular.gci_cells_count =
		0;
#endif
	location_cb_expected++;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", 0);

	cellular_rest_req_resp_handle(location_cb_expected - 1);

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	/* Wait enough that cellular timeout occurs */
	k_sleep(K_MSEC(10));

	/* Send NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp_pci1);
	k_sleep(K_MSEC(1));
#endif
}

/* Test cellular timeout during the 2nd NCELLMEAS for which indication is not sent
 * causing backup timer to expire.
 */
void test_location_cellular_timeout_during_2nd_ncellmeas_backup_timeout(void)
{
#if !defined(CONFIG_LOCATION_DATA_DETAILS)
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, 1, methods);

	config.methods[0].cellular.cell_count = 2;
	config.methods[0].cellular.timeout = 100;

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	test_location_event_data[location_cb_expected].location.latitude = 61.50375;
	test_location_event_data[location_cb_expected].location.longitude = 23.896979;
	test_location_event_data[location_cb_expected].location.accuracy = 750.0;
	location_cb_expected++;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,5", 0);

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;

	/* Wait a bit so that NCELLMEAS is sent from location lib before we send a response.
	 * Otherwise, lte_lc would ignore NCELLMEAS notification because no NCELLMEAS on going
	 * from lte_lc point of view.
	 */
	k_sleep(K_MSEC(1));

	at_monitor_dispatch(ncellmeas_resp_pci1);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", 0);

	cellular_rest_req_resp_handle(location_cb_expected - 1);

	/* scan_cellular.c has a backup timer of 2s which we need to wait */
	k_sleep(K_MSEC(2200));
#endif
#endif
}

/********* WIFI POSITIONING TESTS ***********************/

/* Test successful Wi-Fi location request utilizing HERE service. */
void test_location_wifi(void)
{
#if defined(CONFIG_LOCATION_METHOD_WIFI)
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_WIFI};

	location_config_defaults_set(&config, 1, methods);

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_WIFI;
	location_cb_expected++;
#endif
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_WIFI;
	test_location_event_data[location_cb_expected].location.latitude = 51.98765;
	test_location_event_data[location_cb_expected].location.longitude = 13.12345;
	test_location_event_data[location_cb_expected].location.accuracy = 50.0;
	test_location_event_data[location_cb_expected].location.datetime.valid = false;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].location.details.wifi.ap_count = 2;
#endif

	location_cb_expected++;
	net_mgmt_NET_REQUEST_WIFI_SCAN_expected = true;

	__cmock_net_mgmt_NET_REQUEST_WIFI_SCAN_ExpectAndReturn(0);

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	cellular_rest_req_resp_handle(location_cb_expected - 1);

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Wait for LOCATION_EVT_STARTED */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif
	struct net_mgmt_event_callback cb;
	const struct wifi_status status = {
		.status = WIFI_STATUS_CONN_SUCCESS
	};
	const struct wifi_scan_result scan_result1 = {
		.ssid = "TestAP1",
		.ssid_length = 7,
		.channel = 36,
		.mac = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB},
		.mac_length = 6
	};
	const struct wifi_scan_result scan_result2 = {
		.ssid = "TestAP2",
		.ssid_length = 7,
		.channel = 36,
		.mac = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66},
		.mac_length = 6
	};

	/* Send Wi-Fi scan response which further triggers the rest of the location calculation */
	cb.info = &scan_result1;
	scan_wifi_net_mgmt_event_handler(&cb, NET_EVENT_WIFI_SCAN_RESULT, NULL);
	k_sleep(K_MSEC(1));

	/* Unknown event to make sure it's ignored */
	scan_wifi_net_mgmt_event_handler(&cb, NET_EVENT_WIFI_IFACE_STATUS, NULL);
	k_sleep(K_MSEC(1));

	cb.info = &scan_result2;
	scan_wifi_net_mgmt_event_handler(&cb, NET_EVENT_WIFI_SCAN_RESULT, NULL);
	k_sleep(K_MSEC(1));

	cb.info = &status;
	scan_wifi_net_mgmt_event_handler(&cb, NET_EVENT_WIFI_SCAN_DONE, NULL);
	k_sleep(K_MSEC(1));
#endif
#endif
}

/* Test timeout during Wi-Fi scan. Only one scan result is received before the timeout and
 * hence position cannot be retrieved from the cloud.
 */
void test_location_wifi_timeout(void)
{
#if defined(CONFIG_LOCATION_METHOD_WIFI)
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_WIFI};

	location_config_defaults_set(&config, 1, methods);
	config.methods[0].wifi.timeout = 1;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_WIFI;
	location_cb_expected++;
#endif
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_ERROR;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_WIFI;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].location.details.wifi.ap_count = 2;
#endif
	location_cb_expected++;

	net_mgmt_NET_REQUEST_WIFI_SCAN_expected = true;

	__cmock_net_mgmt_NET_REQUEST_WIFI_SCAN_ExpectAndReturn(0);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Wait for LOCATION_EVT_STARTED */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif
	struct net_mgmt_event_callback cb;
	const struct wifi_status status = {
		.status = WIFI_STATUS_CONN_SUCCESS
	};
	const struct wifi_scan_result scan_result1 = {
		.ssid = "TestAP1",
		.ssid_length = 7,
		.channel = 36,
		.mac = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB},
		.mac_length = 6
	};
	const struct wifi_scan_result scan_result2 = {
		.ssid = "TestAP2",
		.ssid_length = 7,
		.channel = 36,
		.mac = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66},
		.mac_length = 6
	};

	/* Send Wi-Fi scan response which further triggers the rest of the location calculation */
	cb.info = &scan_result1;
	scan_wifi_net_mgmt_event_handler(&cb, NET_EVENT_WIFI_SCAN_RESULT, NULL);
	k_sleep(K_MSEC(100));

	/* The following events are coming too late so they are ignored */
	cb.info = &scan_result2;
	scan_wifi_net_mgmt_event_handler(&cb, NET_EVENT_WIFI_SCAN_RESULT, NULL);
	k_sleep(K_MSEC(1));
	cb.info = &status;
	scan_wifi_net_mgmt_event_handler(&cb, NET_EVENT_WIFI_SCAN_DONE, NULL);
	k_sleep(K_MSEC(1));
#endif
#endif
}

/********* GENERAL ERROR TESTS ***********************/

/* Test location request with unknown method. */
void test_error_unknown_method(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {99}; /* Set unknown method */

	location_config_defaults_set(&config, 1, methods);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(-EINVAL, err);
}

/* Test setting null default config. */
void test_error_default_config_set_null(void)
{
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(NULL, 1, methods);
}

/* Test location request with too many methods. */
void test_error_too_many_methods(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {
		LOCATION_METHOD_GNSS,
		LOCATION_METHOD_CELLULAR,
		LOCATION_METHOD_GNSS,
		LOCATION_METHOD_CELLULAR};

	/* Check that location_config_defaults_set returns an error with too many methods */
	location_config_defaults_set(&config, 4, methods);

	/* Check that location_request returns an error with too many methods */
	location_config_defaults_set(&config, 2, methods);
	config.methods_count = 4;

	err = location_request(&config);
	TEST_ASSERT_EQUAL(-EINVAL, err);
}

/* Test location request with LOCATION_METHOD_WIFI_CELLULAR in method list. */
void test_error_wifi_cellular_method(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {
		LOCATION_METHOD_GNSS,
		LOCATION_METHOD_WIFI_CELLULAR,
		LOCATION_METHOD_CELLULAR};

	/* Check that location_config_defaults_set returns an error with too many methods */
	location_config_defaults_set(&config, 3, methods);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(-EINVAL, err);
}

/* Test cancelling location request when there is no pending location request. */
void test_error_cancel_no_operation(void)
{
/* TODO: This flagging is required due to a bug revealed accidentally while skipping
 *       many other tests before this.
 *       Method information from previously successful positioning in test_location_gnss()
 *       is still stored and we call nrf_modem_gnss_stop() here which is not expected.
 */
#if !defined(CONFIG_LOCATION_DATA_DETAILS)
	int err;

	err = location_request_cancel();
	TEST_ASSERT_EQUAL(0, err);
#endif
}

/* Test PGPS data processing not supported. */
void test_location_pgps_data_process_fail_notsup(void)
{
	int err;

	err = location_pgps_data_process("testi", 6);
	TEST_ASSERT_EQUAL(-ENOTSUP, err);
}

/********* TESTS WITH SEVERAL POSITIONING METHODS ***********************/

/* Test default location request where fallback from GNSS to cellular occurs. */
void test_location_request_default(void)
{
#if !defined(CONFIG_LOCATION_METHOD_WIFI)
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	int err;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	location_cb_expected++;
#endif
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_FALLBACK;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	location_cb_expected++;
#endif
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	test_location_event_data[location_cb_expected].location.latitude = 61.50375;
	test_location_event_data[location_cb_expected].location.longitude = 23.896979;
	test_location_event_data[location_cb_expected].location.accuracy = 750.0;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].location.details.cellular.ncells_count = 1;
	test_location_event_data[location_cb_expected].location.details.cellular.gci_cells_count =
		3;
#endif
	location_cb_expected++;

	test_pvt_data.flags = NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);

#if defined(CONFIG_LOCATION_TEST_AGNSS)
	/* Setting values which doesn't require new A-GNSS request */
	struct nrf_modem_gnss_agnss_expiry agnss_expiry = {
		.data_flags = 0,
		.utc_expiry = 0xffff,
		.klob_expiry = 0xffff,
		.neq_expiry = 0xffff,
		.integrity_expiry = 0xffff,
		.position_expiry = 0xffff };

	__cmock_nrf_modem_gnss_agnss_expiry_get_ExpectAndReturn(NULL, 0);
	__cmock_nrf_modem_gnss_agnss_expiry_get_IgnoreArg_agnss_expiry();
	__cmock_nrf_modem_gnss_agnss_expiry_get_ReturnMemThruPtr_agnss_expiry(
		&agnss_expiry, sizeof(agnss_expiry));
#endif
	__cmock_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__cmock_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__cmock_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* TODO: Cannot determine the used system mode but it's set as zero by default in lte_lc */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* LTE-M support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* NB-IoT support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* GNSS support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* LTE preference */

	err = location_request(NULL);
	TEST_ASSERT_EQUAL(0, err);

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Wait for LOCATION_EVT_STARTED */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif

#if !defined(CONFIG_LOCATION_TEST_AGNSS)
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
#endif
	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));

	__cmock_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, -EINVAL);
	__cmock_nrf_modem_gnss_read_IgnoreArg_buf();
	__cmock_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Wait for LOCATION_EVT_FALLBACK */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif
	/***** Fallback to cellular *****/

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=3,5", 0);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=4,4", 0);

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;

	/* Wait a bit so that NCELLMEAS is sent from location lib before we send a response.
	 * Otherwise, lte_lc would ignore NCELLMEAS notification because no NCELLMEAS on going
	 * from lte_lc point of view.
	 */
	k_sleep(K_MSEC(1));

	/* Trigger NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp_pci1);

	k_sleep(K_MSEC(1));
	at_monitor_dispatch(ncellmeas_resp_gci1);
	k_sleep(K_MSEC(1));

	cellular_rest_req_resp_handle(location_cb_expected - 1);

	/* Although 5 cells are returned, we only request 4 including service cell so 3 GCI cells */
	at_monitor_dispatch(ncellmeas_resp_gci5);
	k_sleep(K_MSEC(1));
#endif
#endif
}

/* Test location request with:
 * - LOCATION_REQ_MODE_ALL for cellular and GNSS positioning
 * - undefined timeout
 * - negative timeout
 */
void test_location_request_mode_all_cellular_gnss(void)
{
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	int err;

	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR, LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, 2, methods);
	config.mode = LOCATION_REQ_MODE_ALL;
	config.methods[0].cellular.timeout = SYS_FOREVER_MS;
	config.methods[0].cellular.cell_count = 1;
	config.methods[1].gnss.timeout = -10;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	location_cb_expected++;
#endif

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	test_location_event_data[location_cb_expected].location.latitude = 61.50375;
	test_location_event_data[location_cb_expected].location.longitude = 23.896979;
	test_location_event_data[location_cb_expected].location.accuracy = 750.0;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].location.details.cellular.ncells_count = 1;
	test_location_event_data[location_cb_expected].location.details.cellular.gci_cells_count =
		0;
#endif
	location_cb_expected++;
	location_cb_expected_2++;

	test_pvt_data.flags = NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID;
	test_pvt_data.latitude = 60.987;
	test_pvt_data.longitude = -45.997;
	test_pvt_data.accuracy = 15.83;
	test_pvt_data.datetime.year = 2021;
	test_pvt_data.datetime.month = 8;
	test_pvt_data.datetime.day = 2;
	test_pvt_data.datetime.hour = 12;
	test_pvt_data.datetime.minute = 34;
	test_pvt_data.datetime.seconds = 23;
	test_pvt_data.datetime.ms = 789;
	test_pvt_data.sv[0].sv = 2;
	test_pvt_data.sv[0].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
	test_pvt_data.sv[1].sv = 4;
	test_pvt_data.sv[1].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
	test_pvt_data.sv[2].sv = 6;
	test_pvt_data.sv[2].flags = 0;
	test_pvt_data.sv[3].sv = 8;
	test_pvt_data.sv[3].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
	test_pvt_data.sv[4].sv = 10;
	test_pvt_data.sv[4].flags = NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX;
	test_pvt_data.sv[5].sv = 12;
	test_pvt_data.sv[5].flags = 0;

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	test_location_event_data[location_cb_expected].location.latitude = 60.987;
	test_location_event_data[location_cb_expected].location.longitude = -45.997;
	test_location_event_data[location_cb_expected].location.accuracy = 15.83;
	test_location_event_data[location_cb_expected].location.datetime.valid = true;
	test_location_event_data[location_cb_expected].location.datetime.year = 2021;
	test_location_event_data[location_cb_expected].location.datetime.month = 8;
	test_location_event_data[location_cb_expected].location.datetime.day = 2;
	test_location_event_data[location_cb_expected].location.datetime.hour = 12;
	test_location_event_data[location_cb_expected].location.datetime.minute = 34;
	test_location_event_data[location_cb_expected].location.datetime.second = 23;
	test_location_event_data[location_cb_expected].location.datetime.ms = 789;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].location.details.gnss.satellites_tracked = 6;
	test_location_event_data[location_cb_expected].location.details.gnss.satellites_used = 4;
	test_location_event_data[location_cb_expected].location.details.gnss.elapsed_time_gnss = 50;
	test_location_event_data[location_cb_expected].location.details.gnss.pvt_data =
		test_pvt_data;
#endif
	location_cb_expected++;
	location_cb_expected_2++;

	/* Add 2nd event handler to test that it receives the events too */
	err = location_handler_register(location_event_handler_2);
	TEST_ASSERT_EQUAL(0, err);

	/***** First cellular positioning *****/

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Wait for LOCATION_EVT_STARTED */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
	err = k_sem_take(&event_handler_called_sem_2, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif
	cellular_rest_req_resp_handle(location_cb_expected - 2);

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;

	/* Wait a bit so that NCELLMEAS is sent before we send response */
	k_sleep(K_MSEC(1));

	/* Trigger NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp_pci1);

	/* Wait for location_event_handler call for 3 seconds.
	 * If it doesn't happen, next assert will fail the test.
	 */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);

	err = k_sem_take(&event_handler_called_sem_2, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);

	/***** Then GNSS positioning *****/
	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);

#if defined(CONFIG_LOCATION_TEST_AGNSS)
	struct nrf_modem_gnss_agnss_expiry agnss_expiry = {
		.data_flags = 0,
		.utc_expiry = 0xffff,
		.klob_expiry = 0xffff,
		.neq_expiry = 0xffff,
		.integrity_expiry = 0xffff,
		.position_expiry = 0xffff };

	__cmock_nrf_modem_gnss_agnss_expiry_get_ExpectAndReturn(NULL, 0);
	__cmock_nrf_modem_gnss_agnss_expiry_get_IgnoreArg_agnss_expiry();
	__cmock_nrf_modem_gnss_agnss_expiry_get_ReturnMemThruPtr_agnss_expiry(
		&agnss_expiry, sizeof(agnss_expiry));
#endif
	__cmock_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__cmock_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__cmock_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* TODO: Cannot determine the used system mode but it's set as zero by default in lte_lc */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* LTE-M support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* NB-IoT support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* GNSS support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* LTE preference */

#if !defined(CONFIG_LOCATION_TEST_AGNSS)
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
#endif
	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));

	__cmock_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, 0);
	__cmock_nrf_modem_gnss_read_IgnoreArg_buf();
	__cmock_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);
	k_sleep(K_MSEC(1));
#endif
}

/* Test location request error/timeout with :
 * - Use cellular and GNSS positioning and error and timeout occurs, respectively
 * - Use LOCATION_REQ_MODE_ALL so we can check timeout event ID for both methods
 * - Tests also case when all fallbacks are failing
 */
void test_location_request_mode_all_cellular_error_gnss_timeout(void)
{
	int err;

	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR, LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, 2, methods);
	config.mode = LOCATION_REQ_MODE_ALL;
	config.methods[0].cellular.cell_count = 1;
	config.methods[1].gnss.timeout = 100;

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_STARTED;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	location_cb_expected++;
#endif
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_ERROR;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	location_cb_expected++;

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_TIMEOUT;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	location_cb_expected++;

	/* Deregister 2nd event handler used in previous test.
	 * Ignoring return value as in some configurations it hasn't been registered
	 */
	(void)location_handler_deregister(location_event_handler_2);

	/***** First cellular positioning *****/
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", -EFAULT);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Wait for LOCATION_EVT_STARTED */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
#endif
	/* Wait for location_event_handler call for 3 seconds.
	 * If it doesn't happen, next assert will fail the test.
	 */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);

	/***** Then GNSS positioning *****/
	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);

#if defined(CONFIG_LOCATION_TEST_AGNSS)
	struct nrf_modem_gnss_agnss_expiry agnss_expiry = {
		.data_flags = 0,
		.utc_expiry = 0xffff,
		.klob_expiry = 0xffff,
		.neq_expiry = 0xffff,
		.integrity_expiry = 0xffff,
		.position_expiry = 0xffff };

	__cmock_nrf_modem_gnss_agnss_expiry_get_ExpectAndReturn(NULL, 0);
	__cmock_nrf_modem_gnss_agnss_expiry_get_IgnoreArg_agnss_expiry();
	__cmock_nrf_modem_gnss_agnss_expiry_get_ReturnMemThruPtr_agnss_expiry(
		&agnss_expiry, sizeof(agnss_expiry));
#endif
	__cmock_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__cmock_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__cmock_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* TODO: Cannot determine the used system mode but it's set as zero by default in lte_lc */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* LTE-M support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* NB-IoT support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* GNSS support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* LTE preference */

#if !defined(CONFIG_LOCATION_TEST_AGNSS)
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
#endif
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);

	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));
}

/********* TESTS PERIODIC POSITIONING REQUESTS ***********************/

/* Test periodic location request and cancel it once some iterations are done. */
void test_location_gnss_periodic(void)
{
#if !defined(CONFIG_LOCATION_DATA_DETAILS)
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, 1, methods);
	config.interval = 1;
	config.methods[0].gnss.timeout = 120 * MSEC_PER_SEC;
	config.methods[0].gnss.accuracy = LOCATION_ACCURACY_NORMAL;

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	test_location_event_data[location_cb_expected].location.latitude = 61.005;
	test_location_event_data[location_cb_expected].location.longitude = -45.997;
	test_location_event_data[location_cb_expected].location.accuracy = 15.83;
	test_location_event_data[location_cb_expected].location.datetime.valid = true;
	test_location_event_data[location_cb_expected].location.datetime.year = 2021;
	test_location_event_data[location_cb_expected].location.datetime.month = 8;
	test_location_event_data[location_cb_expected].location.datetime.day = 13;
	test_location_event_data[location_cb_expected].location.datetime.hour = 12;
	test_location_event_data[location_cb_expected].location.datetime.minute = 34;
	test_location_event_data[location_cb_expected].location.datetime.second = 56;
	test_location_event_data[location_cb_expected].location.datetime.ms = 789;
	location_cb_expected++;

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_GNSS;
	test_location_event_data[location_cb_expected].location.latitude = 61.005;
	test_location_event_data[location_cb_expected].location.longitude = -44.123;
	test_location_event_data[location_cb_expected].location.accuracy = 15.83;
	test_location_event_data[location_cb_expected].location.datetime.valid = true;
	test_location_event_data[location_cb_expected].location.datetime.year = 2021;
	test_location_event_data[location_cb_expected].location.datetime.month = 8;
	test_location_event_data[location_cb_expected].location.datetime.day = 13;
	test_location_event_data[location_cb_expected].location.datetime.hour = 12;
	test_location_event_data[location_cb_expected].location.datetime.minute = 34;
	test_location_event_data[location_cb_expected].location.datetime.second = 56;
	test_location_event_data[location_cb_expected].location.datetime.ms = 789;
	location_cb_expected++;

	test_pvt_data.flags = NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID;
	test_pvt_data.latitude = 61.005;
	test_pvt_data.longitude = -45.997;
	test_pvt_data.accuracy = 15.83;
	test_pvt_data.datetime.year = 2021;
	test_pvt_data.datetime.month = 8;
	test_pvt_data.datetime.day = 13;
	test_pvt_data.datetime.hour = 12;
	test_pvt_data.datetime.minute = 34;
	test_pvt_data.datetime.seconds = 56;
	test_pvt_data.datetime.ms = 789;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);

#if defined(CONFIG_LOCATION_TEST_AGNSS)
	struct nrf_modem_gnss_agnss_expiry agnss_expiry = {
		.data_flags = 0,
		.utc_expiry = 0xffff,
		.klob_expiry = 0xffff,
		.neq_expiry = 0xffff,
		.integrity_expiry = 0xffff,
		.position_expiry = 0xffff };

	__cmock_nrf_modem_gnss_agnss_expiry_get_ExpectAndReturn(NULL, 0);
	__cmock_nrf_modem_gnss_agnss_expiry_get_IgnoreArg_agnss_expiry();
	__cmock_nrf_modem_gnss_agnss_expiry_get_ReturnMemThruPtr_agnss_expiry(
		&agnss_expiry, sizeof(agnss_expiry));
#endif
	__cmock_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__cmock_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__cmock_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* TODO: Cannot determine the used system mode but it's set as zero by default in lte_lc */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* LTE-M support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* NB-IoT support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* GNSS support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* LTE preference */

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

#if !defined(CONFIG_LOCATION_TEST_AGNSS)
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
#endif
	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));

	__cmock_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, 0);
	__cmock_nrf_modem_gnss_read_IgnoreArg_buf();
	__cmock_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);
	k_sleep(K_MSEC(1));

	/* Wait for location_event_handler call for 3 seconds.
	 * If it doesn't happen, next assert will fail the test.
	 */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	/* 2nd GNSS fix */
	test_pvt_data.flags = NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID;
	test_pvt_data.latitude = 61.005;
	test_pvt_data.longitude = -44.123;
	test_pvt_data.accuracy = 15.83;
	test_pvt_data.datetime.year = 2021;
	test_pvt_data.datetime.month = 8;
	test_pvt_data.datetime.day = 13;
	test_pvt_data.datetime.hour = 12;
	test_pvt_data.datetime.minute = 34;
	test_pvt_data.datetime.seconds = 56;
	test_pvt_data.datetime.ms = 789;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);

#if defined(CONFIG_LOCATION_TEST_AGNSS)
	__cmock_nrf_modem_gnss_agnss_expiry_get_ExpectAndReturn(NULL, 0);
	__cmock_nrf_modem_gnss_agnss_expiry_get_IgnoreArg_agnss_expiry();
	__cmock_nrf_modem_gnss_agnss_expiry_get_ReturnMemThruPtr_agnss_expiry(
		&agnss_expiry, sizeof(agnss_expiry));
#endif
	__cmock_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__cmock_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__cmock_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* TODO: Cannot determine the used system mode but it's set as zero by default in lte_lc */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* LTE-M support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* NB-IoT support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(1); /* GNSS support */
	__mock_nrf_modem_at_scanf_ReturnVarg_int(0); /* LTE preference */

#if !defined(CONFIG_LOCATION_TEST_AGNSS)
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
#endif
	k_sleep(K_MSEC(1500));
	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));

	__cmock_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, 0);
	__cmock_nrf_modem_gnss_read_IgnoreArg_buf();
	__cmock_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);

	/* Wait for location_event_handler call for 3 seconds.
	 * If it doesn't happen, next assert will fail the test.
	 */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	/* Stop periodic by cancelling location request */
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);
	err = location_request_cancel();
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	/* Check that no actions are taken if GNSS event is sent after cancelling */
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);
	k_sleep(K_MSEC(1));
#endif
}

/* Test periodic location request and cancel it once some iterations are done. */
void test_location_cellular_periodic(void)
{
#if !defined(CONFIG_LOCATION_DATA_DETAILS)
#if !defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, 1, methods);
	config.interval = 1;
	config.methods[0].cellular.cell_count = 1;

	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	test_location_event_data[location_cb_expected].location.latitude = 61.50375;
	test_location_event_data[location_cb_expected].location.longitude = 23.896979;
	test_location_event_data[location_cb_expected].location.accuracy = 750.0;
	test_location_event_data[location_cb_expected].location.datetime.valid = false;
	location_cb_expected++;

	/* TODO: Set different values to first iteration */
	test_location_event_data[location_cb_expected].id = LOCATION_EVT_LOCATION;
	test_location_event_data[location_cb_expected].method = LOCATION_METHOD_CELLULAR;
	test_location_event_data[location_cb_expected].location.latitude = 61.5037;
	test_location_event_data[location_cb_expected].location.longitude = 23.896979;
	test_location_event_data[location_cb_expected].location.accuracy = 750.0;
	test_location_event_data[location_cb_expected].location.datetime.valid = false;
	location_cb_expected++;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	cellular_rest_req_resp_handle(0);

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	/* Trigger NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp_pci1);

	/* Wait for location_event_handler call for 3 seconds.
	 * If it doesn't happen, next assert will fail the test.
	 */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	cellular_rest_req_resp_handle(1);

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));
	/* Wait a bit more than the interval so that NCELLMEAS is sent before we send response
	 * Note that we could first send results and then location library would send NCELLMEAS and
	 * the test wouldn't see a failure so these things would need to be checked from the logs.
	 */
	k_sleep(K_MSEC(1500));
	/* Trigger NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp_pci1);

	/* Wait for location_event_handler call for 3 seconds.
	 * If it doesn't happen, next assert will fail the test.
	 */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	/* Stop periodic by cancelling location request */
	err = location_request_cancel();
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));
#endif
#endif
}

/* This is needed because AT Monitor library is initialized in SYS_INIT. */
static int location_test_sys_init(void)
{
	__cmock_nrf_modem_at_notif_handler_set_ExpectAnyArgsAndReturn(0);

	return 0;
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}

SYS_INIT(location_test_sys_init, POST_KERNEL, 0);

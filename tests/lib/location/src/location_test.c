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
#include <mock_nrf_modem_gnss.h>
#include <mock_modem_key_mgmt.h>
#include <mock_rest_client.h>


static struct location_event_data test_location_event_data = {0};
static struct nrf_modem_gnss_pvt_data_frame test_pvt_data = {0};
static struct rest_client_req_context rest_req_ctx = { 0 };
static struct rest_client_resp_context rest_resp_ctx = { 0 };
static bool location_callback_called_occurred;
static bool location_callback_called_expected;

K_SEM_DEFINE(event_handler_called_sem, 0, 1);

/* Strings for GNSS positioning */
static const char xmonitor_resp[] =
	"%XMONITOR: 1,\"Operator\",\"OP\",\"20065\",\"0140\",7,20,\"001F8414\","
	"334,6200,66,44,\"\","
	"\"11100000\",\"00010011\",\"01001001\"";

/* Strings for cellular positioning */
static const char ncellmeas_resp[] =
	"%NCELLMEAS:0,\"00011B07\",\"26295\",\"00B7\",2300,7,63,31,"
	"150344527,2300,8,60,29,0,2400,11,55,26,184\r\n";

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
	location_callback_called_occurred = false;
	location_callback_called_expected = false;
	k_sem_reset(&event_handler_called_sem);

	mock_nrf_modem_at_Init();
	mock_nrf_modem_gnss_Init();
	mock_rest_client_Init();
	mock_modem_key_mgmt_Init();
}

void tearDown(void)
{
	if (location_callback_called_expected) {
		/* Wait for location_event_handler call for 3 seconds.
		 * If it doesn't happen, next assert will fail the test.
		 */
		k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	}
	TEST_ASSERT_EQUAL(location_callback_called_expected, location_callback_called_occurred);

	mock_nrf_modem_at_Verify();
	mock_nrf_modem_gnss_Verify();
	mock_rest_client_Verify();
	mock_modem_key_mgmt_Verify();
}

static void location_event_handler(const struct location_event_data *event_data)
{
	location_callback_called_occurred = true;

	TEST_ASSERT_EQUAL(test_location_event_data.id, event_data->id);
	TEST_ASSERT_EQUAL(test_location_event_data.location.latitude,
		event_data->location.latitude);
	TEST_ASSERT_EQUAL(test_location_event_data.location.longitude,
		event_data->location.longitude);
	TEST_ASSERT_EQUAL(test_location_event_data.location.accuracy,
		event_data->location.accuracy);
	TEST_ASSERT_EQUAL(test_location_event_data.location.datetime.valid,
		event_data->location.datetime.valid);

	/* Datetime verification is skipped if it's not valid.
	 * Cellular timestamps will be set but it's marked invalid because CONFIG_DATE_TIME=n.
	 * We cannot verify it anyway because it's read at runtime.
	 */
	if (event_data->location.datetime.valid) {
		TEST_ASSERT_EQUAL(test_location_event_data.location.datetime.year,
			event_data->location.datetime.year);
		TEST_ASSERT_EQUAL(test_location_event_data.location.datetime.month,
			event_data->location.datetime.month);
		TEST_ASSERT_EQUAL(test_location_event_data.location.datetime.day,
			event_data->location.datetime.day);
		TEST_ASSERT_EQUAL(test_location_event_data.location.datetime.hour,
			event_data->location.datetime.hour);
		TEST_ASSERT_EQUAL(test_location_event_data.location.datetime.minute,
			event_data->location.datetime.minute);
		TEST_ASSERT_EQUAL(test_location_event_data.location.datetime.second,
			event_data->location.datetime.second);
		TEST_ASSERT_EQUAL(test_location_event_data.location.datetime.ms,
			event_data->location.datetime.ms);
	}
	k_sem_give(&event_handler_called_sem);
}

/********* LOCATION INIT TESTS ***********************/

void test_location_init(void)
{
	int ret;

	// TODO: Change Ignores to Expects
	__wrap_nrf_modem_gnss_event_handler_set_IgnoreAndReturn(0);
	__wrap_modem_key_mgmt_exists_IgnoreAndReturn(0);

	/* TODO: This would be correct but printf syntax is what we receive into the mock
	 *       so we cannot check that XMODEMSLEEP parameters are correct.
	 * __wrap_nrf_modem_at_printf_ExpectAndReturn("AT%XMODEMSLEEP=1,0,10240", 0);
	 */
	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%XMODEMSLEEP=1,%d,%d", 0);

	ret = location_init(location_event_handler);
	TEST_ASSERT_EQUAL(0, ret);
}

/********* GNSS POSITIONING TESTS ***********************/

/* Test successful GNSS location request. */
void test_location_gnss(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, 1, methods);
	config.methods[0].gnss.timeout = 120 * MSEC_PER_SEC;
	config.methods[0].gnss.accuracy = LOCATION_ACCURACY_NORMAL;

	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 61.005;
	test_location_event_data.location.longitude = -45.997;
	test_location_event_data.location.accuracy = 15.83;
	test_location_event_data.location.datetime.valid = true;
	test_location_event_data.location.datetime.year = 2021;
	test_location_event_data.location.datetime.month = 8;
	test_location_event_data.location.datetime.day = 13;
	test_location_event_data.location.datetime.hour = 12;
	test_location_event_data.location.datetime.minute = 34;
	test_location_event_data.location.datetime.second = 56;
	test_location_event_data.location.datetime.ms = 789;

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

	location_callback_called_expected = true;

	__wrap_nrf_modem_gnss_event_handler_set_IgnoreAndReturn(0);

	__wrap_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__wrap_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__wrap_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* TODO: We cannot determine the used system mode here
	 *       but it's set as zero by default in lte_lc
	 */
	__wrap_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);
	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

	__wrap_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__wrap_nrf_modem_at_cmd_IgnoreArg_buf();
	__wrap_nrf_modem_at_cmd_IgnoreArg_len();
	__wrap_nrf_modem_at_cmd_ReturnArrayThruPtr_buf((char *)xmonitor_resp, sizeof(xmonitor_resp));
	at_monitor_dispatch("+CSCON: 0");

	__wrap_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, 0);
	__wrap_nrf_modem_gnss_read_IgnoreArg_buf();
	__wrap_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	__wrap_nrf_modem_gnss_stop_ExpectAndReturn(0);
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);
}

/********* CELLULAR POSITIONING TESTS ***********************/

void cellular_rest_req_resp_handle(void)
{
	sprintf(http_resp_body, http_resp_body_fmt,
		test_location_event_data.location.latitude,
		test_location_event_data.location.longitude,
		test_location_event_data.location.accuracy);

	sprintf(http_resp, "%s%s", http_resp_header_ok, http_resp_body);

	rest_resp_ctx.total_response_len = strlen(http_resp);
	rest_resp_ctx.response_len = strlen(http_resp + strlen(http_resp_header_ok));
	rest_resp_ctx.response = http_resp + strlen(http_resp_header_ok);

	__wrap_rest_client_request_defaults_set_ExpectAnyArgs();
	// TODO: We should verify rest_req_ctx
	//__wrap_rest_client_request_ExpectWithArrayAndReturn(&rest_req_ctx, 1, NULL, 0, 0);
	__wrap_rest_client_request_ExpectAndReturn(&rest_req_ctx, NULL, 0);
	__wrap_rest_client_request_IgnoreArg_req_ctx();
	__wrap_rest_client_request_IgnoreArg_resp_ctx();
	__wrap_rest_client_request_ReturnMemThruPtr_resp_ctx(&rest_resp_ctx, sizeof(rest_resp_ctx));
}

/* Test successful cellular location request utilizing HERE service. */
void test_location_cellular(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, 1, methods);

	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 61.50375;
	test_location_event_data.location.longitude = 23.896979;
	test_location_event_data.location.accuracy = 750.0;
	test_location_event_data.location.datetime.valid = false;

	location_callback_called_expected = true;

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%NCELLMEAS", 0);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

	cellular_rest_req_resp_handle();

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_MULTICELL_LOCATION_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = CONFIG_MULTICELL_LOCATION_HERE_HTTPS_PORT;
	rest_req_ctx.host = CONFIG_MULTICELL_LOCATION_HERE_HOSTNAME;

	/* Trigger NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp);
}

/********* TESTS WITH SEVERAL POSITIONING METHODS ***********************/

/* Test default location request where fallback from GNSS to cellular occurs. */
void test_location_request_default(void)
{
	int err;

	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 61.50375;
	test_location_event_data.location.longitude = 23.896979;
	test_location_event_data.location.accuracy = 750.0;

	location_callback_called_expected = true;

	test_pvt_data.flags = NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID;

	__wrap_nrf_modem_gnss_event_handler_set_IgnoreAndReturn(0);

	__wrap_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__wrap_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__wrap_nrf_modem_gnss_start_ExpectAndReturn(0);

	/* TODO: We cannot determine the used system mode here
	 *       but it's set as zero by default in lte_lc
	 */
	__wrap_nrf_modem_at_scanf_ExpectAndReturn(
		"AT%XSYSTEMMODE?", "%%XSYSTEMMODE: %d,%d,%d,%d", 4);

	err = location_request(NULL);
	TEST_ASSERT_EQUAL(0, err);

	__wrap_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__wrap_nrf_modem_at_cmd_IgnoreArg_buf();
	__wrap_nrf_modem_at_cmd_IgnoreArg_len();
	__wrap_nrf_modem_at_cmd_ReturnArrayThruPtr_buf((char *)xmonitor_resp, sizeof(xmonitor_resp));
	at_monitor_dispatch("+CSCON: 0");

	__wrap_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, -EINVAL);
	__wrap_nrf_modem_gnss_read_IgnoreArg_buf();
	__wrap_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);

	/***** Fallback to cellular *****/

	__wrap_nrf_modem_at_printf_ExpectAndReturn("AT%%NCELLMEAS", 0);

	cellular_rest_req_resp_handle();

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_MULTICELL_LOCATION_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = CONFIG_MULTICELL_LOCATION_HERE_HTTPS_PORT;
	rest_req_ctx.host = CONFIG_MULTICELL_LOCATION_HERE_HOSTNAME;

	/* Trigger NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp);
}

/* This is needed because AT Monitor library is initialized in SYS_INIT. */
static int location_test_sys_init(const struct device *unused)
{
	__wrap_nrf_modem_at_notif_handler_set_ExpectAnyArgsAndReturn(0);

	return 0;
}

/* It is required to be added to each test. That is because unity is using
 * different main signature (returns int) and zephyr expects main which does
 * not return value.
 */
extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}

SYS_INIT(location_test_sys_init, POST_KERNEL, 0);

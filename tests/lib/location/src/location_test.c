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

/* NOTE: Sleep, e.g. k_sleep(K_MSEC(1)), is used after many location library API
 *       function calls because otherwise some of the threaded work in location library
 *       may not run.
 */

#define HTTPS_PORT 443

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

/* PDN active response */
static const char cgact_resp_active[] = "+CGACT: 0,1";

/* Strings for cellular positioning */
static const char ncellmeas_resp_pci1[] =
	"%NCELLMEAS:0,\"00011B07\",\"26295\",\"00B7\",2300,7,63,31,"
	"150344527,2300,8,60,29,0,2400,11,55,26,184\r\n";

static const char ncellmeas_resp_gci1[] =
	"%NCELLMEAS:0,\"00011B07\",\"26295\",\"00B7\",10512,9034,2300,7,63,31,150344527,1,0,"
	"\"00011B08\",\"26295\",\"00B7\",65535,0,2300,9,62,30,150345527,0,0\r\n";

static const char ncellmeas_resp_gci5[] =
	"%NCELLMEAS:0,\"00011B07\",\"26295\",\"00B7\",10512,9034,2300,7,63,31,150344527,1,0,"
	"\"00011B66\",\"26287\",\"00C3\",65535,0,4300,6,71,30,150345527,0,0,"
	"\"0002ABCD\",\"26287\",\"00C3\",65535,0,4300,6,71,30,150345527,0,0,"
	"\"00103425\",\"26244\",\"0056\",65535,0,6400,6,71,30,150345527,0,0,"
	"\"00076543\",\"26256\",\"00C3\",65535,0,620000,6,71,30,150345527,0,0,"
	"\"00011B08\",\"26295\",\"00B7\",65535,0,2300,9,62,30,150345527,0,0\r\n";

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
}

void tearDown(void)
{
	int err;

	if (location_callback_called_expected) {
		/* Wait for location_event_handler call for 3 seconds.
		 * If it doesn't happen, next assert will fail the test.
		 */
		err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
		TEST_ASSERT_EQUAL(0, err);
	}
	TEST_ASSERT_EQUAL(location_callback_called_expected, location_callback_called_occurred);

	mock_nrf_modem_at_Verify();
}

static void location_event_handler(const struct location_event_data *event_data)
{
	TEST_ASSERT_EQUAL(false, location_callback_called_occurred);
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

/* We need to test first init failures as initialization procedure cannot be made again
 * with different conditions once it's complete because there is no uninitialization function.
 */

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

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XMODEMSLEEP=1,0,10240", 0);

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

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);
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

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));

	__cmock_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, 0);
	__cmock_nrf_modem_gnss_read_IgnoreArg_buf();
	__cmock_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);
	k_sleep(K_MSEC(1));
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

	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 61.50375;
	test_location_event_data.location.longitude = 23.896979;
	test_location_event_data.location.accuracy = 750.0;
	test_location_event_data.location.datetime.valid = false;

	location_callback_called_expected = true;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	cellular_rest_req_resp_handle();

	/* Select cellular service to be used */
	rest_req_ctx.url = "here.api"; /* Needs a fix once rest_req_ctx is verified */
	rest_req_ctx.sec_tag = CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG;
	rest_req_ctx.port = HTTPS_PORT;
	rest_req_ctx.host = CONFIG_LOCATION_SERVICE_HERE_HOSTNAME;

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	err = location_request(&config);
	TEST_ASSERT_EQUAL(-EBUSY, err);
	k_sleep(K_MSEC(1));

	/* Trigger NCELLMEAS response which further triggers the rest of the location calculation */
	at_monitor_dispatch(ncellmeas_resp_pci1);
	k_sleep(K_MSEC(1));
}

/* Test cancelling cellular location request. */
void test_location_cellular_cancel_during_ncellmeas(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, 1, methods);
	config.methods[0].cellular.cell_count = 2;
	location_callback_called_expected = false;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEASSTOP", 0);

	err = location_request_cancel();
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));
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

/* Test cancelling location request when there is no pending location request. */
void test_error_cancel_no_operation(void)
{
	int err;

	err = location_request_cancel();
	TEST_ASSERT_EQUAL(0, err);
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

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);
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

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));

	__cmock_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, -EINVAL);
	__cmock_nrf_modem_gnss_read_IgnoreArg_buf();
	__cmock_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);

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

	cellular_rest_req_resp_handle();

	at_monitor_dispatch(ncellmeas_resp_gci5);
	k_sleep(K_MSEC(1));
}

/* Test location request with:
 * - LOCATION_REQ_MODE_ALL for cellular and GNSS positioning
 * - undefined timeout
 * - negative timeout
 */
void test_location_request_mode_all_cellular_gnss(void)
{
	int err;

	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR, LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, 2, methods);
	config.mode = LOCATION_REQ_MODE_ALL;
	config.methods[0].cellular.timeout = SYS_FOREVER_MS;
	config.methods[0].cellular.cell_count = 1;
	config.methods[1].gnss.timeout = -10;

	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 61.50375;
	test_location_event_data.location.longitude = 23.896979;
	test_location_event_data.location.accuracy = 750.0;

	location_callback_called_expected = true;

	/***** First cellular positioning *****/

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

	cellular_rest_req_resp_handle();

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
	TEST_ASSERT_EQUAL(location_callback_called_expected, location_callback_called_occurred);
	location_callback_called_occurred = false;
	k_sem_reset(&event_handler_called_sem);

	/***** Then GNSS positioning *****/
	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 60.987;
	test_location_event_data.location.longitude = -45.997;
	test_location_event_data.location.accuracy = 15.83;
	test_location_event_data.location.datetime.valid = true;
	test_location_event_data.location.datetime.year = 2021;
	test_location_event_data.location.datetime.month = 8;
	test_location_event_data.location.datetime.day = 2;
	test_location_event_data.location.datetime.hour = 12;
	test_location_event_data.location.datetime.minute = 34;
	test_location_event_data.location.datetime.second = 23;
	test_location_event_data.location.datetime.ms = 789;

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
	test_pvt_data.flags = NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);
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

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));

	__cmock_nrf_modem_gnss_read_ExpectAndReturn(
		NULL, sizeof(test_pvt_data), NRF_MODEM_GNSS_DATA_PVT, 0);
	__cmock_nrf_modem_gnss_read_IgnoreArg_buf();
	__cmock_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_PVT);
	k_sleep(K_MSEC(1));
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

	test_location_event_data.id = LOCATION_EVT_ERROR;

	location_callback_called_expected = true;

	/***** First cellular positioning *****/
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", -EFAULT);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

	/* Wait for location_event_handler call for 3 seconds.
	 * If it doesn't happen, next assert will fail the test.
	 */
	err = k_sem_take(&event_handler_called_sem, K_SECONDS(3));
	TEST_ASSERT_EQUAL(0, err);
	TEST_ASSERT_EQUAL(location_callback_called_expected, location_callback_called_occurred);
	location_callback_called_occurred = false;
	k_sem_reset(&event_handler_called_sem);

	/***** Then GNSS positioning *****/
	test_location_event_data.id = LOCATION_EVT_TIMEOUT;

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);
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

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);

	at_monitor_dispatch("+CSCON: 0");
	k_sleep(K_MSEC(1));
}

/********* TESTS PERIODIC POSITIONING REQUESTS ***********************/

/* Test periodic location request and cancel it once some iterations are done. */
void test_location_gnss_periodic(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_GNSS};

	location_config_defaults_set(&config, 1, methods);
	config.interval = 1;
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

	__cmock_nrf_modem_gnss_event_handler_set_ExpectAndReturn(&method_gnss_event_handler, 0);
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

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));
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
	TEST_ASSERT_EQUAL(location_callback_called_expected, location_callback_called_occurred);
	location_callback_called_occurred = false;
	k_sem_reset(&event_handler_called_sem);

	/* 2nd GNSS fix */

	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 61.005;
	test_location_event_data.location.longitude = -44.123;
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

	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT%%XMONITOR", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)xmonitor_resp, sizeof(xmonitor_resp));

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
	TEST_ASSERT_EQUAL(location_callback_called_expected, location_callback_called_occurred);
	location_callback_called_occurred = false;
	k_sem_reset(&event_handler_called_sem);

	/* Stop periodic by cancelling location request */
	location_callback_called_expected = false;
	__cmock_nrf_modem_gnss_stop_ExpectAndReturn(0);
	err = location_request_cancel();
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));
}

/* Test periodic location request and cancel it once some iterations are done. */
void test_location_cellular_periodic(void)
{
	int err;
	struct location_config config = { 0 };
	enum location_method methods[] = {LOCATION_METHOD_CELLULAR};

	location_config_defaults_set(&config, 1, methods);
	config.interval = 1;
	config.methods[0].cellular.cell_count = 1;

	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 61.50375;
	test_location_event_data.location.longitude = 23.896979;
	test_location_event_data.location.accuracy = 750.0;
	test_location_event_data.location.datetime.valid = false;

	location_callback_called_expected = true;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%NCELLMEAS=1", 0);
	__cmock_nrf_modem_at_cmd_ExpectAndReturn(NULL, 0, "AT+CGACT?", 0);
	__cmock_nrf_modem_at_cmd_IgnoreArg_buf();
	__cmock_nrf_modem_at_cmd_IgnoreArg_len();
	__cmock_nrf_modem_at_cmd_ReturnArrayThruPtr_buf(
		(char *)cgact_resp_active, sizeof(cgact_resp_active));

	cellular_rest_req_resp_handle();

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
	TEST_ASSERT_EQUAL(location_callback_called_expected, location_callback_called_occurred);
	location_callback_called_occurred = false;
	k_sem_reset(&event_handler_called_sem);

	/* TODO: Set different values to first iteration */
	test_location_event_data.id = LOCATION_EVT_LOCATION;
	test_location_event_data.location.latitude = 61.5037;
	test_location_event_data.location.longitude = 23.896979;
	test_location_event_data.location.accuracy = 750.0;
	test_location_event_data.location.datetime.valid = false;

	cellular_rest_req_resp_handle();

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
	TEST_ASSERT_EQUAL(location_callback_called_expected, location_callback_called_occurred);
	location_callback_called_occurred = false;
	k_sem_reset(&event_handler_called_sem);

	/* Stop periodic by cancelling location request */
	location_callback_called_expected = false;
	err = location_request_cancel();
	TEST_ASSERT_EQUAL(0, err);
	k_sleep(K_MSEC(1));
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

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <kernel.h>
#include <modem/location.h>
#include <modem/lte_lc.h>
#include <mock_at_cmd.h>
#include <mock_nrf_modem_gnss.h>
#include <mock_modem_key_mgmt.h>
#include <mock_http_request.h>


struct loc_event_data test_loc_event_data = {0};
struct nrf_modem_gnss_pvt_data_frame test_pvt_data = {0};
static bool location_callback_called_occurred;
static bool location_callback_called_expected;

K_SEM_DEFINE(event_handler_called_sem, 0, 1);

/* method_gnss_event_handler() is implemented in the library and we'll call it directly
 * to fake received GNSS event.
 */
extern void method_gnss_event_handler(int event);
/* lte_lc_at_handler() is implemented in LTE link control library and we'll call it directly
 * to fake received neighbour cell measurements.
 */
extern void lte_lc_at_handler(void *context, const char *response);


static void helper_location_data_clear(void)
{
	memset(&test_loc_event_data, 0, sizeof(test_loc_event_data));
	memset(&test_pvt_data, 0, sizeof(test_pvt_data));
}

void setUp(void)
{
	helper_location_data_clear();
	location_callback_called_occurred = false;
	location_callback_called_expected = false;

	mock_at_cmd_Init();
	mock_nrf_modem_gnss_Init();
	mock_http_request_Init();
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

	mock_at_cmd_Verify();
	mock_nrf_modem_gnss_Verify();
	mock_http_request_Verify();
	mock_modem_key_mgmt_Verify();
}

static void location_event_handler(const struct loc_event_data *event_data)
{
	location_callback_called_occurred = true;

	TEST_ASSERT_EQUAL(test_loc_event_data.id, event_data->id);
	TEST_ASSERT_EQUAL(test_loc_event_data.method, event_data->method);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.latitude, event_data->location.latitude);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.longitude, event_data->location.longitude);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.accuracy, event_data->location.accuracy);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.datetime.valid,
		event_data->location.datetime.valid);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.datetime.year,
		event_data->location.datetime.year);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.datetime.month,
		event_data->location.datetime.month);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.datetime.day,
		event_data->location.datetime.day);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.datetime.hour,
		event_data->location.datetime.hour);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.datetime.minute,
		event_data->location.datetime.minute);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.datetime.second,
		event_data->location.datetime.second);
	TEST_ASSERT_EQUAL(test_loc_event_data.location.datetime.ms,
		event_data->location.datetime.ms);

	k_sem_give(&event_handler_called_sem);
}

/********* LOCATION INIT/UNINIT TESTS ***********************/

void test_loc_init(void)
{
	int ret;

	// TODO: Change Ignores to Expects
	__wrap_nrf_modem_gnss_init_IgnoreAndReturn(0);
	__wrap_nrf_modem_gnss_event_handler_set_IgnoreAndReturn(0);
	__wrap_modem_key_mgmt_exists_IgnoreAndReturn(0);
	__wrap_at_cmd_write_ExpectAndReturn("AT%XMODEMSLEEP=1,0,10240", NULL, 0, NULL, 0);

	ret = location_init(location_event_handler);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_loc_gnss(void)
{
	int err;
	struct loc_config config = { 0 };
	struct loc_method_config methods[1] = { 0 };

	config.interval = 0; /* Single position */
	config.methods_count = 1;
	config.methods = methods;
	methods[0].method = LOC_METHOD_GNSS;
	methods[0].gnss.timeout = 120;
	methods[0].gnss.accuracy = LOC_ACCURACY_NORMAL;

	test_loc_event_data.id = LOC_EVT_LOCATION;
	test_loc_event_data.method = LOC_METHOD_GNSS;
	test_loc_event_data.location.latitude = 61.005;
	test_loc_event_data.location.longitude = -45.997;
	test_loc_event_data.location.accuracy = 15.83;
	test_loc_event_data.location.datetime.valid = true;
	test_loc_event_data.location.datetime.year = 2021;
	test_loc_event_data.location.datetime.month = 8;
	test_loc_event_data.location.datetime.day = 13;
	test_loc_event_data.location.datetime.hour = 12;
	test_loc_event_data.location.datetime.minute = 34;
	test_loc_event_data.location.datetime.second = 56;
	test_loc_event_data.location.datetime.ms = 789;

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

	/* TODO: Tommi got 0,0,1,0 when trying the AT command but that fails here with -E2BIG */
	static const char system_mode_resp[] = "%XSYSTEMMODE: 1,0,1";
	static const char cereg_resp[] =
		"+CEREG: 5,1,\"5A00\",\"00038107\",7,,,\"00011110\",\"11100000\"";

	__wrap_nrf_modem_gnss_event_handler_set_IgnoreAndReturn(0);

	__wrap_nrf_modem_gnss_fix_interval_set_ExpectAndReturn(1, 0);
	__wrap_nrf_modem_gnss_use_case_set_ExpectAndReturn(
		NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START, 0);
	__wrap_nrf_modem_gnss_start_ExpectAndReturn(0);

	__wrap_at_cmd_write_ExpectAndReturn("AT%XSYSTEMMODE?", "", 0, NULL, 0);
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_ReturnArrayThruPtr_buf((char *)system_mode_resp,
						   sizeof(system_mode_resp));

	__wrap_at_cmd_write_ExpectAndReturn("AT+CEREG=5", NULL, 0, NULL, 0);

	__wrap_at_cmd_write_ExpectAndReturn("AT+CEREG?", "", 0, NULL, 0);
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_ReturnArrayThruPtr_buf((char *)cereg_resp, sizeof(cereg_resp));

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

	__wrap_nrf_modem_gnss_read_ExpectAndReturn(NULL, sizeof(test_pvt_data),
						   NRF_MODEM_GNSS_DATA_PVT, 0);
	__wrap_nrf_modem_gnss_read_IgnoreArg_buf();
	__wrap_nrf_modem_gnss_read_ReturnMemThruPtr_buf(&test_pvt_data, sizeof(test_pvt_data));
	__wrap_nrf_modem_gnss_stop_ExpectAndReturn(0);
	method_gnss_event_handler(NRF_MODEM_GNSS_EVT_FIX);
}

static const char ncellmeas_resp[] =
	"%NCELLMEAS:0,\"00011B07\",\"26295\",\"00B7\",2300,7,63,31,"
	"150344527,2300,8,60,29,0,2400,11,55,26,184\r\n";

static char http_resp[512];

static const char http_resp_fmt[] =
	"HTTP/1.1 200 OK\r\n"
	"Server: awselb/2.0\r\n"
	"Date: Wed, 25 Aug 2021 04:50:34 GMT\r\n"
	"Content-Type: application/json\r\n"
	"Content-Length: 50\r\n"
	"Connection: close\r\n"
	"request-id: b17cd88c-8a7d-424c-a964-15a0e5d721d9\r\n"
	"Access-Control-Allow-Origin: *\r\n"
	"\r\n"
	"{\"location\":{\"lat\":%.06f,\"lng\":%.06f,\"accuracy\":%f}}\r\n";

static const char http_req[] =
	"POST /v2/locate?apiKey=MyApiKey HTTP/1.1\r\n"
	"Host: positioning.hereapi.com\r\n"
	"Content-Type: application/json\r\n"
	"Connection: close\r\n"
	"Content-Length: 77\r\n\r\n"
	"{\"lte\":[{\"mcc\": 262,\"mnc\": 95,\"cid\": 72455,"
	"\"nmr\":[{\"earfcn\": 8,\"pci\": 60}]}]}";

void test_loc_cellular(void)
{
	int err;
	struct loc_config config = { 0 };
	struct loc_method_config methods[1] = { 0 };

	config.interval = 0; /* Single position */
	config.methods_count = 1;
	config.methods = methods;
	config.methods[0].method = LOC_METHOD_CELLULAR;

	test_loc_event_data.id = LOC_EVT_LOCATION;
	test_loc_event_data.method = LOC_METHOD_CELLULAR;
	test_loc_event_data.location.latitude = 61.50375;
	test_loc_event_data.location.longitude = 23.896979;
	test_loc_event_data.location.accuracy = 750.0;

	location_callback_called_expected = true;

	//enum at_cmd_state state = AT_CMD_OK;

	__wrap_at_cmd_write_ExpectAndReturn("AT%NCELLMEAS", NULL, 0, NULL, 0);
	// TODO: Get rid of or at least check Ignore's in this file
	__wrap_at_cmd_write_IgnoreArg_buf();
	__wrap_at_cmd_write_IgnoreArg_buf_len();
	__wrap_at_cmd_write_IgnoreArg_state();
	//__wrap_at_cmd_write_ReturnArrayThruPtr_buf(resp, sizeof(resp));
	//__wrap_at_cmd_write_ReturnThruPtr_state(&state);

	err = location_request(&config);
	TEST_ASSERT_EQUAL(0, err);

	// TODO: MNC, MCC TAC must come from some test context
	__wrap_execute_http_request_ExpectAndReturn(http_req, strlen(http_req), "", 512, 0);
	sprintf(http_resp, http_resp_fmt,
		test_loc_event_data.location.latitude,
		test_loc_event_data.location.longitude,
		test_loc_event_data.location.accuracy);
	__wrap_execute_http_request_ReturnArrayThruPtr_response(http_resp, sizeof(http_resp));

	__wrap_at_cmd_write_ExpectAndReturn("AT%NCELLMEASSTOP", NULL, 0, NULL, 0);

	/* Trigger NCELLMEAS response which further triggers the rest of the location calculation */
	lte_lc_at_handler(NULL, ncellmeas_resp);
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

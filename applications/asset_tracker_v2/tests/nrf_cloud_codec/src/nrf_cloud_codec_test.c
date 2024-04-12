/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>

#include "cloud_codec.h"
#include <cJSON_os.h>
#include <errno.h>

static struct cloud_codec_data codec;
static int ret;

void setUp(void)
{
	ret = cloud_codec_init(NULL, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	memset(&codec, 0, sizeof(codec));
}

void tearDown(void)
{
	cJSON_FreeString(codec.buf);
}

#define CONF_RECV_EXAMPLE \
"{"\
	"\"config\":{"\
		"\"activeMode\":false,"\
		"\"locationTimeout\":60,"\
		"\"activeWaitTime\":120,"\
		"\"movementResolution\":120,"\
		"\"movementTimeout\":3600,"\
		"\"accThreshAct\":10,"\
		"\"accThreshInact\":5,"\
		"\"accTimeoutInact\":1,"\
		"\"nod\":["\
			"\"gnss\","\
			"\"ncell\""\
		"]"\
	"}"\
"}"

#define CONF_SEND_EXAMPLE "{\"state\":{\"reported\":" CONF_RECV_EXAMPLE "}}"

const static struct cloud_data_cfg config_struct_example = {
	.active_mode = false,
	.location_timeout = 60,
	.active_wait_timeout = 120,
	.movement_resolution = 120,
	.movement_timeout = 3600,
	.accelerometer_activity_threshold = 10,
	.accelerometer_inactivity_threshold = 5,
	.accelerometer_inactivity_timeout = 1,
	.no_data = {
		.gnss = true,
		.neighbor_cell = true,
	},
};

#define UI_EXAMPLE \
"{"\
	"\"data\":\"1\","\
	"\"appId\":\"BUTTON\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123"\
"}"

const static struct cloud_data_ui ui_data_example = {
	.queued = true,
	.btn = 1,
	.btn_ts = 1563968747123,
};

#define BAT_BATCH_EXAMPLE \
"[{"\
	"\"appId\":\"BATTERY\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":\"50\""\
"}]"

const static struct cloud_data_battery bat_data_example = {
	.bat = 50,
	.bat_ts = 1563968747123,
	.queued = true,
};

#define GNSS_BATCH_EXAMPLE \
"[{"\
	"\"appId\":\"GNSS\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":{\"lng\":30,\"lat\":40,\"acc\":180,\"alt\":245,\"spd\":5,\"hdg\":39}"\
"}]"

const static struct cloud_data_gnss gnss_data_example = {
	.queued = true,
	.gnss_ts = 1563968747123,
	.pvt = {
		.lat = 40,
		.lon = 30,
		.acc = 180,
		.alt = 245,
		.alt_acc = 10,
		.spd = 5,
		.spd_acc = 1,
		.hdg = 39,
		.hdg_acc = 5
	}
};

#define GNSS_BATCH_EXAMPLE_NO_HEADING \
"[{"\
	"\"appId\":\"GNSS\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":{\"lng\":30,\"lat\":40,\"acc\":180,\"alt\":245,\"spd\":5}"\
"}]"

const static struct cloud_data_gnss gnss_data_example_no_heading = {
	.queued = true,
	.gnss_ts = 1563968747123,
	.pvt = {
		.lat = 40,
		.lon = 30,
		.acc = 180,
		.alt = 245,
		.alt_acc = 10,
		.spd = 5,
		.spd_acc = 1,
		.hdg = 39,
		.hdg_acc = 180
	}
};

#define MODEM_DYNAMIC_BATCH_EXAMPLE \
"[{"\
	"\"appId\":\"DEVICE\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":{"\
		"\"networkInfo\":{"\
			"\"currentBand\":3,"\
			"\"networkMode\":\"NB-IoT\","\
			"\"rsrp\":-8,"\
			"\"areaCode\":12,"\
			"\"mccmnc\":24202,"\
			"\"cellID\":33703719,"\
			"\"ipAddress\":\"10.81.183.99\""\
		"}"\
	"}"\
"},{"\
	"\"appId\":\"RSRP\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":\"-8\""\
"}]"

const static struct cloud_data_modem_dynamic modem_dyn_data_example = {
	.band = 3,
	.nw_mode = LTE_LC_LTE_MODE_NBIOT,
	.rsrp = -8,
	.area = 12,
	.mccmnc = "24202",
	.cell = 33703719,
	.ip = "10.81.183.99",
	.ts = 1000,
	.queued = true,
};

#define SENSORS_BATCH_EXAMPLE \
"[{"\
	"\"appId\":\"AIR_QUAL\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":\"50\""\
"},{"\
	"\"appId\":\"HUMID\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":\"50.00\""\
"},{"\
	"\"appId\":\"TEMP\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":\"23.00\""\
"},{"\
	"\"appId\":\"AIR_PRESS\","\
	"\"messageType\":\"DATA\","\
	"\"ts\":1563968747123,"\
	"\"data\":\"101.00\""\
"}]"

const static struct cloud_data_sensors sensor_data_example = {
	.temperature = 23,
	.humidity = 50,
	.pressure = 101,
	.bsec_air_quality = 50,
	.env_ts = 1563968747123,
	.queued = true,
};


/* tests encoding an empty cloud_location object */
void test_enc_cloud_location_empty(void)
{
	struct cloud_data_cloud_location data = { .queued = true };

	ret = cloud_codec_encode_cloud_location(&codec, &data);

	TEST_ASSERT_EQUAL(-ENOTSUP, ret);
	TEST_ASSERT_EQUAL_PTR(NULL, codec.buf);
	TEST_ASSERT_EQUAL(true, data.queued);
}

/* tests encoding of A-GNSS request (unsupported) */
void test_enc_agnss_req(void)
{
	struct cloud_data_agnss_request data = {0};

	ret = cloud_codec_encode_agnss_request(&codec, &data);

	TEST_ASSERT_EQUAL(-ENOTSUP, ret);
	TEST_ASSERT_EQUAL_PTR(NULL, codec.buf);
	TEST_ASSERT_EQUAL(false, data.queued);
}

/* tests encoding of PGPS request (unsupported) */
void test_enc_pgps_req(void)
{
	struct cloud_data_pgps_request data = {0};

	ret = cloud_codec_encode_pgps_request(&codec, &data);

	TEST_ASSERT_EQUAL(-ENOTSUP, ret);
	TEST_ASSERT_EQUAL_PTR(NULL, codec.buf);
	TEST_ASSERT_EQUAL(false, data.queued);
}

/* tests config decoding supplying a NULL pointer */
void test_dec_config_null(void)
{
	ret = cloud_codec_decode_config(NULL, 0, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/* tests config decoding supplying an empty string */
void test_dec_config_empty(void)
{
	const char *str = "";
	size_t len = 0;

	ret = cloud_codec_decode_config(str, len, NULL);
	TEST_ASSERT_EQUAL(-ENOENT, ret);
}

/* tests config decoding a string that contains no object */
void test_dec_config_no_object(void)
{
	const char *str = "[]";
	size_t len = strlen(str);

	ret = cloud_codec_decode_config(str, len, NULL);
	TEST_ASSERT_EQUAL(-ENOENT, ret);
}

/* tests config decoding an old version that will be skipped */
void test_dec_config_already_handled(void)
{
	const char *str = "{\"version\":1}";
	size_t len = strlen(str);

	ret = cloud_codec_decode_config(str, len, NULL);
	ret = cloud_codec_decode_config(str, len, NULL);
	TEST_ASSERT_EQUAL(-ECANCELED, ret);
}

/* tests config decoding a string that contains an empty object */
void test_dec_config_empty_object(void)
{
	const char *str = "{}";
	size_t len = strlen(str);

	ret = cloud_codec_decode_config(str, len, NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

/* tests config decoding a string that contains a state object that is empty */
void test_dec_config_empty_state_object(void)
{
	const char *str = "{\"state\":{}}";
	size_t len = strlen(str);

	ret = cloud_codec_decode_config(str, len, NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

/* tests config decoding a string that contains a messageType instead of a config */
void test_dec_config_data_message(void)
{
	const char *str = "{\"messageType\":\"\"}";
	size_t len = strlen(str);

	ret = cloud_codec_decode_config(str, len, NULL);
	TEST_ASSERT_EQUAL(-ENOENT, ret);

	str = "{\"appID\":\"\"}";
	len = strlen(str);

	ret = cloud_codec_decode_config(str, len, NULL);
	TEST_ASSERT_EQUAL(-ENOENT, ret);
}

/* tests config decoding a string that contains an empty configuration */
void test_dec_config_empty_config_object(void)
{
	const char *str = "{\"config\":{}}";
	size_t len = strlen(str);

	const struct cloud_data_cfg null_cfg = {0};
	struct cloud_data_cfg cfg = null_cfg;

	ret = cloud_codec_decode_config(str, len, &cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, memcmp(&null_cfg, &cfg, sizeof(null_cfg)));
}

/* tests config decoding a string that contains an empty configuration nested in a state object */
void test_dec_config_empty_state_config_object(void)
{
	const char *str = "{\"state\":{\"config\":{}}}";
	size_t len = strlen(str);

	const struct cloud_data_cfg null_cfg = {0};
	struct cloud_data_cfg cfg = null_cfg;

	ret = cloud_codec_decode_config(str, len, &cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, memcmp(&null_cfg, &cfg, sizeof(null_cfg)));
}

/* tests config decoding a string that contains an empty configuration */
void test_dec_config_good_weather(void)
{
	const char *str = CONF_RECV_EXAMPLE;
	size_t len = strlen(str);

	const struct cloud_data_cfg *desired_cfg = &config_struct_example;
	struct cloud_data_cfg cfg = {0};

	ret = cloud_codec_decode_config(str, len, &cfg);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, memcmp(desired_cfg, &cfg, sizeof(const struct cloud_data_cfg)));
}

/* tests config decoding a string that contains a typical configuration string */
void test_enc_config_good_weather(void)
{
	struct cloud_data_cfg data = config_struct_example;

	ret = cloud_codec_encode_config(&codec, &data);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, strncmp(CONF_SEND_EXAMPLE, codec.buf, strlen(CONF_SEND_EXAMPLE)));
}

/* tests encoding an empty UI data object */
void test_enc_ui_empty(void)
{
	struct cloud_data_ui data = {0};

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

/* tests encoding a UI data object with a too large button ID */
void test_enc_ui_btn_too_big(void)
{
	struct cloud_data_ui data = {
		.queued = true,
		.btn = 10,
	};

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

/* tests encoding a typical UI data object */
void test_enc_ui_good_weather(void)
{
	struct cloud_data_ui data = ui_data_example;

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, strncmp(UI_EXAMPLE, codec.buf, strlen(UI_EXAMPLE)));
}

/* tests encoding a set of empty data objects */
void test_enc_data_empty(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	ret = cloud_codec_encode_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf);
	TEST_ASSERT_EQUAL(-ENOTSUP, ret);
	TEST_ASSERT_EQUAL_PTR(NULL, codec.buf);
}

/* tests batch encoding zero-length buffers */
void test_enc_batch_data_empty(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				0, 0, 0, 0, 0, 0, 0);

	TEST_ASSERT_EQUAL(-ENODATA, ret);
	TEST_ASSERT_EQUAL_PTR(NULL, codec.buf);
}

/* tests batch encoding single-element empty buffers */
void test_enc_batch_data_single_empty_element(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
	TEST_ASSERT_EQUAL_PTR(NULL, codec.buf);
}

/* tests batch encoding typical battery data */
void test_enc_batch_data_single_battery(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = bat_data_example;

	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, strncmp(BAT_BATCH_EXAMPLE, codec.buf, strlen(BAT_BATCH_EXAMPLE)));
	TEST_ASSERT_FALSE(bat_buf.queued);
}

/* tests batch encoding battery data with a too large value */
void test_enc_batch_data_single_battery_too_big(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {
		.bat = INT16_MAX,
		.bat_ts = 1563968747123,
		.queued = true,
	};
	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
	TEST_ASSERT_TRUE(bat_buf.queued);
}

/* tests batch encoding typical GNSS data */
void test_enc_batch_data_gnss(void)
{
	struct cloud_data_gnss gnss_buf = gnss_data_example;
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL_STRING(GNSS_BATCH_EXAMPLE, codec.buf);
	TEST_ASSERT_FALSE(gnss_buf.queued);
}

/* tests batch encoding GNSS data without heading */
void test_enc_batch_data_gnss_no_heading(void)
{
	struct cloud_data_gnss gnss_buf = gnss_data_example_no_heading;
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL_STRING(GNSS_BATCH_EXAMPLE_NO_HEADING, codec.buf);
	TEST_ASSERT_FALSE(gnss_buf.queued);
}

/* tests batch encoding typical dynamic modem data */
void test_enc_batch_data_modem_dynamic(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = modem_dyn_data_example;
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, strncmp(MODEM_DYNAMIC_BATCH_EXAMPLE,
				     codec.buf,
				     strlen(MODEM_DYNAMIC_BATCH_EXAMPLE)));
	TEST_ASSERT_FALSE(modem_dyn_buf.queued);
}

/* tests batch encoding dynamic modem data with a too small rsrp */
void test_enc_batch_data_modem_dynamic_rsrp_too_small(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = modem_dyn_data_example;
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	modem_dyn_buf.rsrp = INT16_MIN;
	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
	TEST_ASSERT_FALSE(modem_dyn_buf.queued);
}

/* tests batch encoding typical sensor data */
void test_enc_batch_data_sensor(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = sensor_data_example;
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {0};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(EXIT_SUCCESS, ret);
	TEST_ASSERT_EQUAL(0, strncmp(SENSORS_BATCH_EXAMPLE,
			  codec.buf, strlen(SENSORS_BATCH_EXAMPLE)));
	TEST_ASSERT_FALSE(sensor_buf.queued);
}

/* tests batch encoding UI data with a too large button ID */
void test_enc_batch_data_ui_toobig(void)
{
	struct cloud_data_gnss gnss_buf = {0};
	struct cloud_data_sensors sensor_buf = {0};
	struct cloud_data_modem_static modem_stat_buf = {0};
	struct cloud_data_modem_dynamic modem_dyn_buf = {0};
	struct cloud_data_ui ui_buf = {
		.queued = true,
		.btn = 10,
	};
	struct cloud_data_impact impact_buf = {0};
	struct cloud_data_battery bat_buf = {0};

	ret = cloud_codec_encode_batch_data(&codec,
				&gnss_buf,
				&sensor_buf,
				&modem_stat_buf,
				&modem_dyn_buf,
				&ui_buf,
				&impact_buf,
				&bat_buf,
				1, 1, 1, 1, 1, 1, 1);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
	TEST_ASSERT_TRUE(ui_buf.queued);
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

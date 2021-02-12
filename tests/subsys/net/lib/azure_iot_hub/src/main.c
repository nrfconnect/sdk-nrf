/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ztest.h>
#include <net/azure_iot_hub.h>

#include "azure_iot_hub_topic.h"

static void test_topic_parse_devicebound(void)
{
	int err;
	const char *topic = "devices/my-device/messages/devicebound/";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, 0, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_DEVICEBOUND,
		      "Incorrect topic type");
	zassert_equal(topic_data.prop_bag_count, 0,
		      "Incorrect property bag count");
}

static void test_topic_parse_twin_update_desired(void)
{
	int err;
	const char *topic = "$iothub/twin/PATCH/properties/desired/?$version=9";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, 0, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_TWIN_UPDATE_DESIRED,
		      "Incorrect topic type");
	zassert_equal(topic_data.prop_bag_count, 1,
		      "Incorrect property bag count");
	zassert_true(strcmp(topic_data.prop_bag[0].key, "$version") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[0].value, "9") == 0, NULL);
}

static void test_topic_parse_twin_update_result(void)
{
	int err;
	const char *topic = "$iothub/twin/res/200/?$rid=738&$version=135";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, 0, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_TWIN_UPDATE_RESULT,
		      "Incorrect topic type");
	zassert_equal(topic_data.status, 200, NULL);
	zassert_equal(topic_data.prop_bag_count, 2,
		      "Incorrect property bag count");
	zassert_true(strcmp(topic_data.prop_bag[0].key, "$rid") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[0].value, "738") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[1].key, "$version") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[1].value, "135") == 0, NULL);
}

static void test_topic_parse_direct_method(void)
{
	int err;
	const char *topic = "$iothub/methods/POST/my_method/?$rid=387";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, 0, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_DIRECT_METHOD,
		      "Incorrect topic type");
	zassert_true(strcmp(topic_data.name, "my_method") == 0, NULL);
	zassert_equal(topic_data.prop_bag_count, 1,
		      "Incorrect property bag count");
	zassert_true(strcmp(topic_data.prop_bag[0].key, "$rid") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[0].value, "387") == 0, NULL);
}

static void test_topic_parse_dps_reg_result(void)
{
	int err;
	const char *topic =
		"$dps/registrations/res/204/?$rid=59765&retry-after=3";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, 0, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_DPS_REG_RESULT,
		      "Incorrect topic type");
	zassert_equal(topic_data.status, 204, NULL);
	zassert_equal(topic_data.prop_bag_count, 2,
		      "Incorrect property bag count");
	zassert_true(strcmp(topic_data.prop_bag[0].key, "$rid") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[0].value, "59765") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[1].key, "retry-after") == 0,
			    NULL);
	zassert_true(strcmp(topic_data.prop_bag[1].value, "3") == 0, NULL);
}

static void test_topic_parse_prop_bag_overload(void)
{
	int err;
	const char *topic =
		"$dps/registrations/res/204/?$rid=59765&retry-after=3&"
		"test1=0x0&tes2&test3&test4&test5";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, 0, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_DPS_REG_RESULT,
		      "Incorrect topic type");
	zassert_equal(topic_data.status, 204, NULL);
	zassert_equal(topic_data.prop_bag_count, 5,
		      "Incorrect property bag count");
	zassert_true(strcmp(topic_data.prop_bag[0].key, "$rid") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[0].value, "59765") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[1].key, "retry-after") == 0,
			    NULL);
	zassert_true(strcmp(topic_data.prop_bag[1].value, "3") == 0, NULL);
}

static void test_topic_parse_unknown_topic(void)
{
	int err;
	const char *topic =
		"ive/never/seen/this/topic/before/?whos=this";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, 0, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_UNEXPECTED,
		      "Incorrect topic type");
	zassert_equal(topic_data.prop_bag_count, 0,
		      "Incorrect property bag count");
}

static void test_topic_add_prop_bags(void)
{
	char *key1 = "key1";
	char *val1 = "value1";
	char *key2 = "key2";
	char *val2 = "";
	char *key3 = "key3";
	struct azure_iot_hub_prop_bag bags[] = {
		{
			.key = key1,
			.value = val1,
		},
		{
			.key = key2,
			.value = val2,
		},
		{
			.key = key3,
			.value = NULL,
		},
	};
	char *prop_bag_str = azure_iot_hub_prop_bag_str_get(bags,
							    ARRAY_SIZE(bags));

	zassert_not_null(prop_bag_str, NULL);
	zassert_equal(strcmp(prop_bag_str, "?key1=value1&key2=&key3"), 0,
		      "Incorrect property bag string");
	azure_iot_hub_prop_bag_free(prop_bag_str);
}

static void test_topic_add_prop_bags_reverse(void)
{
	char *key1 = "key1";
	char *value1 = "value1";
	char *key2 = "key2";
	char *value2 = "";
	char *key3 = "key3";
	struct azure_iot_hub_prop_bag bags[] = {
		{
			.key = key3,
			.value = NULL,
		},
		{
			.key = key2,
			.value = value2,
		},
		{
			.key = key1,
			.value = value1,
		},
	};
	char *prop_bag_str = azure_iot_hub_prop_bag_str_get(bags,
							    ARRAY_SIZE(bags));

	zassert_not_null(prop_bag_str, NULL);
	zassert_equal(strcmp(prop_bag_str, "?key3&key2=&key1=value1"), 0,
		      "Incorrect property bag string");
	azure_iot_hub_prop_bag_free(prop_bag_str);
}

static void test_topic_parse_long(void)
{
	int err;
	const char *topic =
		"devices/my-device/messages/devicebound/"
		"%24.mid=456132-235-a2fd-8458-56432854d&"
		"%24.to=%2Fdevices%2Fmy-device%2Fmessages%2Fdevicebound&"
		"key1&key2=&key3=value3";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, 0, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_DEVICEBOUND,
		      "Incorrect topic type");
	zassert_equal(topic_data.prop_bag_count, 5,
		      "Incorrect property bag count");
	zassert_true(strcmp(topic_data.prop_bag[0].key, "%24.mid") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[0].value,
			    "456132-235-a2fd-8458-56432854d") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[1].key, "%24.to") == 0,
			    NULL);
	zassert_true(strcmp(topic_data.prop_bag[1].value,
		     "%2Fdevices%2Fmy-device%2Fmessages%2Fdevicebound") == 0,
		     NULL);
	zassert_true(strcmp(topic_data.prop_bag[2].key, "key1") == 0, NULL);
	zassert_equal(strlen(topic_data.prop_bag[2].value), 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[3].key, "key2") == 0, NULL);
	zassert_equal(strlen(topic_data.prop_bag[3].value), 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[4].key, "key3") == 0, NULL);
	zassert_true(strcmp(topic_data.prop_bag[4].value, "value3") == 0, NULL);
}

static void test_topic_prop_bag_too_long(void)
{
	int err;
	const char *topic =
		"devices/my-device/messages/devicebound/"
		"?thispropbagisjustwaytoolongtobeparsedandputintothebuffer";
	struct topic_parser_data topic_data = {
		.topic = topic,
		.topic_len = strlen(topic),
		.type = TOPIC_TYPE_UNKNOWN,
	};

	err = azure_iot_hub_topic_parse(&topic_data);
	zassert_equal(err, -ENOMEM, NULL);
	zassert_equal(topic_data.type, TOPIC_TYPE_DEVICEBOUND,
		      "Incorrect topic type");
	zassert_equal(topic_data.prop_bag_count, 0,
		      "Incorrect property bag count");
}

void test_main(void)
{
	ztest_test_suite(azure_iot_hub_topic,
			 ztest_unit_test(test_topic_parse_devicebound),
			 ztest_unit_test(test_topic_parse_twin_update_desired),
			 ztest_unit_test(test_topic_parse_twin_update_result),
			 ztest_unit_test(test_topic_parse_direct_method),
			 ztest_unit_test(test_topic_parse_dps_reg_result),
			 ztest_unit_test(test_topic_parse_prop_bag_overload),
			 ztest_unit_test(test_topic_parse_unknown_topic),
			 ztest_unit_test(test_topic_add_prop_bags),
			 ztest_unit_test(test_topic_add_prop_bags_reverse),
			 ztest_unit_test(test_topic_parse_long),
			 ztest_unit_test(test_topic_prop_bag_too_long)
			 );
	ztest_run_test_suite(azure_iot_hub_topic);
}

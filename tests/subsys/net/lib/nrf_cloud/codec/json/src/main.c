/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Unit tests for the nRF Cloud public JSON codec API defined in
 * <net/nrf_cloud_codec.h> (include/net/ — public API).
 *
 * Scope:
 *   nrf_cloud_codec.h is the public API for JSON object construction and
 *   encoding.  Its functions can be exercised directly on simulated platforms
 *   because cJSON has no hardware or network dependencies.  This is in
 *   contrast to the higher-level transport APIs (nrf_cloud.h,
 *   nrf_cloud_coap.h) which require an active cloud connection and are
 *   better validated through hardware-in-the-loop and system-level
 *   integration tests.
 *
 * Coverage: JSON object lifecycle, adders, getters, bulk operations, and
 * cloud encoding.  CBOR coverage is intentionally deferred to a separate
 * suite (codec/cbor/) which exercises the internal coap_codec.h layer.
 *
 * No mocks are used: cJSON is a pure heap-based library with no
 * platform or hardware dependencies.  The three internal helper functions
 * (get_num_from_obj, get_string_from_obj, get_bool_from_obj) and the memory
 * wrappers (nrf_cloud_calloc/free/malloc) are provided by src/fakes.c.
 */

#include <zephyr/ztest.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_defs.h>
#include <cJSON.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/*
 * SUITE: nrf_cloud_codec_lifecycle
 * Tests for nrf_cloud_obj_init, nrf_cloud_obj_msg_init,
 * nrf_cloud_obj_bulk_init, nrf_cloud_obj_reset, nrf_cloud_obj_free.
 */

ZTEST_SUITE(nrf_cloud_codec_lifecycle, NULL, NULL, NULL, NULL, NULL);

ZTEST(nrf_cloud_codec_lifecycle, test_obj_init_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_not_null(obj.json);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_init_null)
{
	zassert_equal(nrf_cloud_obj_init(NULL), -EINVAL);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_init_already_initialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_init(&obj), -ENOTEMPTY);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_msg_init_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), 0);
	zassert_not_null(obj.json);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_msg_init_null_obj)
{
	zassert_equal(nrf_cloud_obj_msg_init(NULL, "MY_APP", "DATA"), -EINVAL);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_msg_init_null_app_id)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, NULL, "DATA"), -EINVAL);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_msg_init_null_msg_type)
{
	/* msg_type is documented as optional; NULL is valid */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", NULL), 0);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_msg_init_already_initialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), 0);
	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), -ENOTEMPTY);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_bulk_init_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_bulk_init(&obj), 0);
	zassert_not_null(obj.json);
	zassert_true(cJSON_IsArray(obj.json));
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_bulk_init_null)
{
	zassert_equal(nrf_cloud_obj_bulk_init(NULL), -EINVAL);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_bulk_init_already_initialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_bulk_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_bulk_init(&obj), -ENOTEMPTY);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_reset_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	/* reset clears the pointer but does NOT free heap memory; free first */
	cJSON_Delete(obj.json);
	zassert_equal(nrf_cloud_obj_reset(&obj), 0);
	zassert_is_null(obj.json);
	zassert_equal(obj.enc_src, NRF_CLOUD_ENC_SRC_NONE);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_reset_null)
{
	zassert_equal(nrf_cloud_obj_reset(NULL), -EINVAL);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_free_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_free(&obj), 0);
	zassert_is_null(obj.json);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_free_uninitialized)
{
	/* cJSON_Delete(NULL) is a no-op; freeing an uninitialized object is safe */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_free(&obj), 0);
}

ZTEST(nrf_cloud_codec_lifecycle, test_obj_free_null)
{
	zassert_equal(nrf_cloud_obj_free(NULL), -EINVAL);
}

/*
 * SUITE: nrf_cloud_codec_adders
 * Tests for all nrf_cloud_obj_*_add functions.
 * Each test uses a fresh, initialized JSON object provided by the fixture.
 */

static struct nrf_cloud_obj adder_obj;

static void adders_before(void *f)
{
	ARG_UNUSED(f);
	NRF_CLOUD_OBJ_JSON_DEFINE(o);

	adder_obj = o;
	zassert_equal(nrf_cloud_obj_init(&adder_obj), 0);
}

static void adders_after(void *f)
{
	ARG_UNUSED(f);
	nrf_cloud_obj_free(&adder_obj);
}

ZTEST_SUITE(nrf_cloud_codec_adders, NULL, NULL, adders_before, adders_after, NULL);

/* num_add */

ZTEST(nrf_cloud_codec_adders, test_obj_num_add_valid)
{
	double val;

	zassert_equal(nrf_cloud_obj_num_add(&adder_obj, "num_key", 42.0, false), 0);
	zassert_equal(nrf_cloud_obj_num_get(&adder_obj, "num_key", &val), 0);
	zassert_equal(val, 42.0);
}

ZTEST(nrf_cloud_codec_adders, test_obj_num_add_float)
{
	double val;

	zassert_equal(nrf_cloud_obj_num_add(&adder_obj, "float_key", 3.14, false), 0);
	zassert_equal(nrf_cloud_obj_num_get(&adder_obj, "float_key", &val), 0);
	zassert_true(fabs(val - 3.14) < 0.001);
}

ZTEST(nrf_cloud_codec_adders, test_obj_num_add_data_child)
{
	/* data_child=true nests the value under a "data" sub-object */
	zassert_equal(nrf_cloud_obj_num_add(&adder_obj, "sub_key", 99.0, true), 0);

	cJSON *data = cJSON_GetObjectItem(adder_obj.json, NRF_CLOUD_JSON_DATA_KEY);

	zassert_not_null(data);

	cJSON *item = cJSON_GetObjectItem(data, "sub_key");

	zassert_not_null(item);
	zassert_true(cJSON_IsNumber(item));
	zassert_equal(item->valuedouble, 99.0);
}

ZTEST(nrf_cloud_codec_adders, test_obj_num_add_null_obj)
{
	zassert_equal(nrf_cloud_obj_num_add(NULL, "key", 1.0, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_num_add_null_key)
{
	zassert_equal(nrf_cloud_obj_num_add(&adder_obj, NULL, 1.0, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_num_add_uninitialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(uninit);

	zassert_equal(nrf_cloud_obj_num_add(&uninit, "key", 1.0, false), -ENOENT);
}

/* ts_add (multiple timestamps to evaluate limits and correctness) */

ZTEST(nrf_cloud_codec_adders, test_obj_ts_add_zero)
{
	double val;

	zassert_equal(nrf_cloud_obj_ts_add(&adder_obj, 0), 0);
	zassert_equal(nrf_cloud_obj_num_get(&adder_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY, &val), 0);
	zassert_equal((int64_t)val, (int64_t)0);
}

ZTEST(nrf_cloud_codec_adders, test_obj_ts_add_epoch)
{
	/* 2023-11-14 ~1700000000000 ms; value is below 2^53 so exact in double */
	const int64_t ts = 1700000000000LL;
	double val;

	zassert_equal(nrf_cloud_obj_ts_add(&adder_obj, ts), 0);
	zassert_equal(nrf_cloud_obj_num_get(&adder_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY, &val), 0);
	zassert_equal((int64_t)val, ts);
}

ZTEST(nrf_cloud_codec_adders, test_obj_ts_add_large)
{
	/* 10^15 ms is the largest power-of-ten below 2^53 (~9e15), exact in double */
	const int64_t ts = 1000000000000000LL;
	double val;

	zassert_equal(nrf_cloud_obj_ts_add(&adder_obj, ts), 0);
	zassert_equal(nrf_cloud_obj_num_get(&adder_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY, &val), 0);
	zassert_equal((int64_t)val, ts);
}

ZTEST(nrf_cloud_codec_adders, test_obj_ts_add_null_obj)
{
	zassert_equal(nrf_cloud_obj_ts_add(NULL, 1000), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_ts_add_uninitialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(uninit);

	zassert_equal(nrf_cloud_obj_ts_add(&uninit, 1000), -ENOENT);
}

/* str_add */

ZTEST(nrf_cloud_codec_adders, test_obj_str_add_valid)
{
	char *str = NULL;

	zassert_equal(nrf_cloud_obj_str_add(&adder_obj, "str_key", "hello", false), 0);
	zassert_equal(nrf_cloud_obj_str_get(&adder_obj, "str_key", &str), 0);
	zassert_not_null(str);
	zassert_str_equal(str, "hello");
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_add_data_child)
{
	zassert_equal(nrf_cloud_obj_str_add(&adder_obj, "str_child", "world", true), 0);

	cJSON *data = cJSON_GetObjectItem(adder_obj.json, NRF_CLOUD_JSON_DATA_KEY);

	zassert_not_null(data);

	cJSON *item = cJSON_GetObjectItem(data, "str_child");

	zassert_not_null(item);
	zassert_true(cJSON_IsString(item));
	zassert_str_equal(item->valuestring, "world");
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_add_null_obj)
{
	zassert_equal(nrf_cloud_obj_str_add(NULL, "key", "val", false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_add_null_val)
{
	zassert_equal(nrf_cloud_obj_str_add(&adder_obj, "key", NULL, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_add_null_key)
{
	/* For JSON type, NULL key is checked inside the JSON case */
	zassert_equal(nrf_cloud_obj_str_add(&adder_obj, NULL, "val", false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_add_uninitialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(uninit);

	zassert_equal(nrf_cloud_obj_str_add(&uninit, "key", "val", false), -ENOENT);
}

/* bool_add */

ZTEST(nrf_cloud_codec_adders, test_obj_bool_add_true)
{
	bool val;

	zassert_equal(nrf_cloud_obj_bool_add(&adder_obj, "bool_key", true, false), 0);
	zassert_equal(nrf_cloud_obj_bool_get(&adder_obj, "bool_key", &val), 0);
	zassert_true(val);
}

ZTEST(nrf_cloud_codec_adders, test_obj_bool_add_false)
{
	bool val;

	zassert_equal(nrf_cloud_obj_bool_add(&adder_obj, "bool_false", false, false), 0);
	zassert_equal(nrf_cloud_obj_bool_get(&adder_obj, "bool_false", &val), 0);
	zassert_false(val);
}

ZTEST(nrf_cloud_codec_adders, test_obj_bool_add_null_obj)
{
	zassert_equal(nrf_cloud_obj_bool_add(NULL, "key", true, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_bool_add_null_key)
{
	zassert_equal(nrf_cloud_obj_bool_add(&adder_obj, NULL, true, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_bool_add_uninitialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(uninit);

	zassert_equal(nrf_cloud_obj_bool_add(&uninit, "key", true, false), -ENOENT);
}

/* null_add */

ZTEST(nrf_cloud_codec_adders, test_obj_null_add_valid)
{
	zassert_equal(nrf_cloud_obj_null_add(&adder_obj, "null_key", false), 0);

	cJSON *item = cJSON_GetObjectItem(adder_obj.json, "null_key");

	zassert_not_null(item);
	zassert_true(cJSON_IsNull(item));
}

ZTEST(nrf_cloud_codec_adders, test_obj_null_add_null_obj)
{
	zassert_equal(nrf_cloud_obj_null_add(NULL, "key", false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_null_add_null_key)
{
	zassert_equal(nrf_cloud_obj_null_add(&adder_obj, NULL, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_null_add_uninitialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(uninit);

	zassert_equal(nrf_cloud_obj_null_add(&uninit, "key", false), -ENOENT);
}

/* object_add */

ZTEST(nrf_cloud_codec_adders, test_obj_object_add_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(child);
	double val;

	zassert_equal(nrf_cloud_obj_init(&child), 0);
	zassert_equal(nrf_cloud_obj_num_add(&child, "child_num", 7.0, false), 0);
	zassert_equal(nrf_cloud_obj_object_add(&adder_obj, "nested", &child, false), 0);
	/* object_add resets child (json ownership transferred to adder_obj) */
	zassert_is_null(child.json);

	/* Detach to verify the nested content via the getter API */
	NRF_CLOUD_OBJ_JSON_DEFINE(detached);

	zassert_equal(nrf_cloud_obj_object_detach(&adder_obj, "nested", &detached), 0);
	zassert_equal(nrf_cloud_obj_num_get(&detached, "child_num", &val), 0);
	zassert_equal(val, 7.0);
	nrf_cloud_obj_free(&detached);
}

ZTEST(nrf_cloud_codec_adders, test_obj_object_add_null_obj)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(child);

	zassert_equal(nrf_cloud_obj_object_add(NULL, "key", &child, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_object_add_null_key)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(child);

	zassert_equal(nrf_cloud_obj_init(&child), 0);
	zassert_equal(nrf_cloud_obj_object_add(&adder_obj, NULL, &child, false), -EINVAL);
	nrf_cloud_obj_free(&child);
}

ZTEST(nrf_cloud_codec_adders, test_obj_object_add_null_child)
{
	zassert_equal(nrf_cloud_obj_object_add(&adder_obj, "key", NULL, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_object_add_uninitialized_child)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(uninit_child);

	zassert_equal(nrf_cloud_obj_object_add(&adder_obj, "key", &uninit_child, false), -ENOENT);
}

/* int_array_add */

ZTEST(nrf_cloud_codec_adders, test_obj_int_array_add_single)
{
	const uint32_t arr[] = {42};

	zassert_equal(nrf_cloud_obj_int_array_add(&adder_obj, "int_arr", arr, 1, false), 0);

	cJSON *item = cJSON_GetObjectItem(adder_obj.json, "int_arr");

	zassert_not_null(item);
	zassert_true(cJSON_IsArray(item));
	zassert_equal(cJSON_GetArraySize(item), 1);
	zassert_equal((uint32_t)cJSON_GetArrayItem(item, 0)->valuedouble, 42U);
}

ZTEST(nrf_cloud_codec_adders, test_obj_int_array_add_multi)
{
	const uint32_t arr[] = {1, 2, 3};

	zassert_equal(nrf_cloud_obj_int_array_add(&adder_obj, "ints", arr, 3, false), 0);

	cJSON *item = cJSON_GetObjectItem(adder_obj.json, "ints");

	zassert_not_null(item);
	zassert_equal(cJSON_GetArraySize(item), 3);
	zassert_equal((uint32_t)cJSON_GetArrayItem(item, 0)->valuedouble, 1U);
	zassert_equal((uint32_t)cJSON_GetArrayItem(item, 1)->valuedouble, 2U);
	zassert_equal((uint32_t)cJSON_GetArrayItem(item, 2)->valuedouble, 3U);
}

ZTEST(nrf_cloud_codec_adders, test_obj_int_array_add_null_obj)
{
	const uint32_t arr[] = {1};

	zassert_equal(nrf_cloud_obj_int_array_add(NULL, "key", arr, 1, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_int_array_add_null_array)
{
	zassert_equal(nrf_cloud_obj_int_array_add(&adder_obj, "key", NULL, 1, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_int_array_add_zero_count)
{
	const uint32_t arr[] = {1};

	zassert_equal(nrf_cloud_obj_int_array_add(&adder_obj, "key", arr, 0, false), -EINVAL);
}

/* str_array_add */

ZTEST(nrf_cloud_codec_adders, test_obj_str_array_add_single)
{
	static const char * const arr[] = {"one"};

	zassert_equal(nrf_cloud_obj_str_array_add(&adder_obj, "str_arr", arr, 1, false), 0);

	cJSON *item = cJSON_GetObjectItem(adder_obj.json, "str_arr");

	zassert_not_null(item);
	zassert_true(cJSON_IsArray(item));
	zassert_equal(cJSON_GetArraySize(item), 1);
	zassert_str_equal(cJSON_GetArrayItem(item, 0)->valuestring, "one");
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_array_add_multi)
{
	static const char * const arr[] = {"a", "b", "c"};

	zassert_equal(nrf_cloud_obj_str_array_add(&adder_obj, "strs", arr, 3, false), 0);

	cJSON *item = cJSON_GetObjectItem(adder_obj.json, "strs");

	zassert_not_null(item);
	zassert_equal(cJSON_GetArraySize(item), 3);
	zassert_str_equal(cJSON_GetArrayItem(item, 0)->valuestring, "a");
	zassert_str_equal(cJSON_GetArrayItem(item, 1)->valuestring, "b");
	zassert_str_equal(cJSON_GetArrayItem(item, 2)->valuestring, "c");
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_array_add_null_obj)
{
	static const char * const arr[] = {"x"};

	zassert_equal(nrf_cloud_obj_str_array_add(NULL, "key", arr, 1, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_array_add_null_array)
{
	zassert_equal(nrf_cloud_obj_str_array_add(&adder_obj, "key", NULL, 1, false), -EINVAL);
}

ZTEST(nrf_cloud_codec_adders, test_obj_str_array_add_zero_count)
{
	static const char * const arr[] = {"x"};

	zassert_equal(nrf_cloud_obj_str_array_add(&adder_obj, "key", arr, 0, false), -EINVAL);
}

/*
 * SUITE: nrf_cloud_codec_getters
 * Error-path tests for nrf_cloud_obj_num_get, nrf_cloud_obj_str_get,
 * nrf_cloud_obj_bool_get, nrf_cloud_obj_msg_check, nrf_cloud_obj_object_detach.
 * Happy-path coverage is provided by the round-trip tests in the adders suite.
 */

ZTEST_SUITE(nrf_cloud_codec_getters, NULL, NULL, NULL, NULL, NULL);

/* num_get */

ZTEST(nrf_cloud_codec_getters, test_obj_num_get_null_obj)
{
	double val;

	zassert_equal(nrf_cloud_obj_num_get(NULL, "key", &val), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_num_get_null_key)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	double val;

	zassert_equal(nrf_cloud_obj_num_get(&obj, NULL, &val), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_num_get_null_out)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_num_get(&obj, "key", NULL), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_num_get_uninitialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	double val;

	/* obj.json == NULL → get_num_from_obj receives NULL → -ENOENT */
	zassert_equal(nrf_cloud_obj_num_get(&obj, "key", &val), -ENOENT);
}

ZTEST(nrf_cloud_codec_getters, test_obj_num_get_key_not_found)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	double val;

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_num_get(&obj, "missing_key", &val), -ENODEV);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_num_get_wrong_type)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	double val;

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_str_add(&obj, "str_key", "hello", false), 0);
	zassert_equal(nrf_cloud_obj_num_get(&obj, "str_key", &val), -ENOMSG);
	nrf_cloud_obj_free(&obj);
}

/* str_get */

ZTEST(nrf_cloud_codec_getters, test_obj_str_get_null_obj)
{
	char *str;

	zassert_equal(nrf_cloud_obj_str_get(NULL, "key", &str), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_str_get_null_key)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	char *str;

	zassert_equal(nrf_cloud_obj_str_get(&obj, NULL, &str), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_str_get_null_out)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_str_get(&obj, "key", NULL), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_str_get_key_not_found)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	char *str;

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_str_get(&obj, "missing", &str), -ENODEV);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_str_get_wrong_type)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	char *str;

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_num_add(&obj, "num_key", 1.0, false), 0);
	zassert_equal(nrf_cloud_obj_str_get(&obj, "num_key", &str), -ENOMSG);
	nrf_cloud_obj_free(&obj);
}

/* bool_get */

ZTEST(nrf_cloud_codec_getters, test_obj_bool_get_null_obj)
{
	bool val;

	zassert_equal(nrf_cloud_obj_bool_get(NULL, "key", &val), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_bool_get_null_key)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	bool val;

	zassert_equal(nrf_cloud_obj_bool_get(&obj, NULL, &val), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_bool_get_null_out)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_bool_get(&obj, "key", NULL), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_bool_get_key_not_found)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	bool val;

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_bool_get(&obj, "missing", &val), -ENODEV);
	nrf_cloud_obj_free(&obj);
}

/* msg_check */

ZTEST(nrf_cloud_codec_getters, test_obj_msg_check_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), 0);
	zassert_equal(nrf_cloud_obj_msg_check(&obj, "MY_APP", "DATA"), 0);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_msg_check_app_id_only)
{
	/* Passing NULL for msg_type checks only the app ID */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), 0);
	zassert_equal(nrf_cloud_obj_msg_check(&obj, "MY_APP", NULL), 0);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_msg_check_msg_type_only)
{
	/* Passing NULL for app_id checks only the msg_type */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), 0);
	zassert_equal(nrf_cloud_obj_msg_check(&obj, NULL, "DATA"), 0);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_msg_check_wrong_app_id)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), 0);
	zassert_equal(nrf_cloud_obj_msg_check(&obj, "OTHER_APP", "DATA"), -ENOMSG);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_msg_check_wrong_msg_type)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), 0);
	zassert_equal(nrf_cloud_obj_msg_check(&obj, "MY_APP", "OTHER_TYPE"), -ENOMSG);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_msg_check_null_obj)
{
	zassert_equal(nrf_cloud_obj_msg_check(NULL, "APP", "TYPE"), -EINVAL);
}

ZTEST(nrf_cloud_codec_getters, test_obj_msg_check_both_null)
{
	/* Both app_id and msg_type NULL → -EINVAL */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_init(&obj, "MY_APP", "DATA"), 0);
	zassert_equal(nrf_cloud_obj_msg_check(&obj, NULL, NULL), -EINVAL);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_msg_check_uninitialized)
{
	/* obj.json == NULL → -EINVAL (checked before type dispatch) */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_msg_check(&obj, "APP", "TYPE"), -EINVAL);
}

/* object_detach */

ZTEST(nrf_cloud_codec_getters, test_obj_object_detach_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(child);
	NRF_CLOUD_OBJ_JSON_DEFINE(detached);
	char *str = NULL;

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_init(&child), 0);
	zassert_equal(nrf_cloud_obj_str_add(&child, "k", "v", false), 0);
	zassert_equal(nrf_cloud_obj_object_add(&obj, "nested", &child, false), 0);

	zassert_equal(nrf_cloud_obj_object_detach(&obj, "nested", &detached), 0);
	zassert_equal(nrf_cloud_obj_str_get(&detached, "k", &str), 0);
	zassert_str_equal(str, "v");

	nrf_cloud_obj_free(&detached);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_object_detach_key_not_found)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(out);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_object_detach(&obj, "missing", &out), -ENODEV);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_object_detach_not_an_object)
{
	/* Key exists but its value is a string, not an object → -ENOMSG */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(out);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_str_add(&obj, "str_key", "val", false), 0);
	zassert_equal(nrf_cloud_obj_object_detach(&obj, "str_key", &out), -ENOMSG);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_getters, test_obj_object_detach_uninitialized)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(out);

	zassert_equal(nrf_cloud_obj_object_detach(&obj, "key", &out), -ENOENT);
}

/*
 * SUITE: nrf_cloud_codec_bulk
 * Tests for nrf_cloud_obj_bulk_check and nrf_cloud_obj_bulk_add.
 */

ZTEST_SUITE(nrf_cloud_codec_bulk, NULL, NULL, NULL, NULL, NULL);

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_check_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(bulk);

	zassert_equal(nrf_cloud_obj_bulk_init(&bulk), 0);
	zassert_true(nrf_cloud_obj_bulk_check(&bulk));
	nrf_cloud_obj_free(&bulk);
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_check_not_array)
{
	/* A plain JSON object is not a bulk container */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_false(nrf_cloud_obj_bulk_check(&obj));
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_check_null)
{
	zassert_false(nrf_cloud_obj_bulk_check(NULL));
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_add_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(bulk);
	NRF_CLOUD_OBJ_JSON_DEFINE(item);

	zassert_equal(nrf_cloud_obj_bulk_init(&bulk), 0);
	zassert_equal(nrf_cloud_obj_init(&item), 0);
	zassert_equal(nrf_cloud_obj_num_add(&item, "val", 1.0, false), 0);
	zassert_equal(nrf_cloud_obj_bulk_add(&bulk, &item), 0);
	/* bulk_add transfers ownership but does not reset item.json (unlike object_add);
	 * null it out explicitly to avoid a dangling pointer.
	 */
	item.json = NULL;
	zassert_equal(cJSON_GetArraySize(bulk.json), 1);
	nrf_cloud_obj_free(&bulk);
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_add_multiple)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(bulk);

	zassert_equal(nrf_cloud_obj_bulk_init(&bulk), 0);

	for (int i = 0; i < 3; i++) {
		NRF_CLOUD_OBJ_JSON_DEFINE(entry);

		zassert_equal(nrf_cloud_obj_init(&entry), 0);
		zassert_equal(nrf_cloud_obj_bulk_add(&bulk, &entry), 0);
		entry.json = NULL; /* ownership transferred; prevent dangling pointer */
	}

	zassert_equal(cJSON_GetArraySize(bulk.json), 3);
	nrf_cloud_obj_free(&bulk);
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_add_null_bulk)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(item);

	zassert_equal(nrf_cloud_obj_bulk_add(NULL, &item), -EINVAL);
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_add_null_item)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(bulk);

	zassert_equal(nrf_cloud_obj_bulk_init(&bulk), 0);
	zassert_equal(nrf_cloud_obj_bulk_add(&bulk, NULL), -EINVAL);
	nrf_cloud_obj_free(&bulk);
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_add_not_array)
{
	/* bulk is a plain object, not an array → -ENODEV */
	NRF_CLOUD_OBJ_JSON_DEFINE(not_bulk);
	NRF_CLOUD_OBJ_JSON_DEFINE(item);

	zassert_equal(nrf_cloud_obj_init(&not_bulk), 0);
	zassert_equal(nrf_cloud_obj_init(&item), 0);
	zassert_equal(nrf_cloud_obj_bulk_add(&not_bulk, &item), -ENODEV);
	nrf_cloud_obj_free(&not_bulk);
	nrf_cloud_obj_free(&item);
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_add_uninitialized_bulk)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(bulk);
	NRF_CLOUD_OBJ_JSON_DEFINE(item);

	zassert_equal(nrf_cloud_obj_init(&item), 0);
	zassert_equal(nrf_cloud_obj_bulk_add(&bulk, &item), -ENOENT);
	nrf_cloud_obj_free(&item);
}

ZTEST(nrf_cloud_codec_bulk, test_obj_bulk_add_uninitialized_item)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(bulk);
	NRF_CLOUD_OBJ_JSON_DEFINE(uninit_item);

	zassert_equal(nrf_cloud_obj_bulk_init(&bulk), 0);
	zassert_equal(nrf_cloud_obj_bulk_add(&bulk, &uninit_item), -ENOENT);
	nrf_cloud_obj_free(&bulk);
}

/*
 * SUITE: nrf_cloud_codec_encode
 * Tests for nrf_cloud_obj_cloud_encode and nrf_cloud_obj_cloud_encoded_free.
 */

ZTEST_SUITE(nrf_cloud_codec_encode, NULL, NULL, NULL, NULL, NULL);

ZTEST(nrf_cloud_codec_encode, test_obj_cloud_encode_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_str_add(&obj, "key", "value", false), 0);
	zassert_equal(nrf_cloud_obj_cloud_encode(&obj), 0);

	zassert_equal(obj.enc_src, NRF_CLOUD_ENC_SRC_CLOUD_ENCODED);
	zassert_not_null(obj.encoded_data.ptr);
	zassert_true(obj.encoded_data.len > 0);
	/* Verify the encoded JSON string contains our key */
	zassert_not_null(strstr((const char *)obj.encoded_data.ptr, "key"));

	nrf_cloud_obj_cloud_encoded_free(&obj);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_encode, test_obj_cloud_encode_null)
{
	zassert_equal(nrf_cloud_obj_cloud_encode(NULL), -EINVAL);
}

ZTEST(nrf_cloud_codec_encode, test_obj_cloud_encode_uninitialized)
{
	/* cJSON_PrintUnformatted(NULL) returns NULL → -ENOMEM */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_cloud_encode(&obj), -ENOMEM);
}

ZTEST(nrf_cloud_codec_encode, test_obj_cloud_encoded_free_valid)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_cloud_encode(&obj), 0);
	zassert_equal(nrf_cloud_obj_cloud_encoded_free(&obj), 0);
	zassert_equal(obj.enc_src, NRF_CLOUD_ENC_SRC_NONE);
	zassert_is_null(obj.encoded_data.ptr);
	nrf_cloud_obj_free(&obj);
}

ZTEST(nrf_cloud_codec_encode, test_obj_cloud_encoded_free_null)
{
	zassert_equal(nrf_cloud_obj_cloud_encoded_free(NULL), -EINVAL);
}

ZTEST(nrf_cloud_codec_encode, test_obj_cloud_encoded_free_not_encoded)
{
	/* enc_src != NRF_CLOUD_ENC_SRC_CLOUD_ENCODED → -EACCES */
	NRF_CLOUD_OBJ_JSON_DEFINE(obj);

	zassert_equal(nrf_cloud_obj_init(&obj), 0);
	zassert_equal(nrf_cloud_obj_cloud_encoded_free(&obj), -EACCES);
	nrf_cloud_obj_free(&obj);
}

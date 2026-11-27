/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Ownership tests for the shadow delta/transform decoders: after the caller
 * frees the input object and the decoded data, nothing may remain allocated.
 */

#include <zephyr/ztest.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_defs.h>
#include <nrf_cloud_codec_internal.h>
#include <stdlib.h>
#include <string.h>

static int live_allocs;

static void *counting_malloc(size_t size)
{
	void *p = malloc(size);

	if (p) {
		live_allocs++;
	}
	return p;
}

static void counting_free(void *p)
{
	if (p) {
		live_allocs--;
	}
	free(p);
}

static void *setup(void)
{
	struct nrf_cloud_os_mem_hooks hooks = {
		.malloc_fn = counting_malloc,
		.free_fn = counting_free,
	};

	zassert_ok(nrf_cloud_codec_init(&hooks));
	return NULL;
}

static void before(void *f)
{
	ARG_UNUSED(f);
	live_allocs = 0;
}

static void decode_input(struct nrf_cloud_obj *obj, const char *json)
{
	const struct nrf_cloud_data in = { .ptr = json, .len = strlen(json) };

	zassert_ok(nrf_cloud_obj_input_decode(obj, &in));
	zassert_true(live_allocs > 0);
}

ZTEST_SUITE(nrf_cloud_shadow_decode, NULL, setup, before, NULL, NULL);

ZTEST(nrf_cloud_shadow_decode, test_transform_result_no_leak)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(shadow_obj);
	struct nrf_cloud_obj_shadow_transform tf;

	decode_input(&shadow_obj, "{\"tf\":{\"pairing\":{\"state\":\"paired\"}}}");

	zassert_ok(nrf_cloud_obj_shadow_transform_decode(&shadow_obj, &tf));
	zassert_false(tf.is_err);
	zassert_not_null(tf.result.obj.json);

	nrf_cloud_obj_free(&shadow_obj);
	nrf_cloud_obj_shadow_transform_free(&tf);
	zassert_equal(live_allocs, 0, "%d allocation(s) leaked", live_allocs);
}

ZTEST(nrf_cloud_shadow_decode, test_transform_error_no_leak)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(shadow_obj);
	struct nrf_cloud_obj_shadow_transform tf;

	decode_input(&shadow_obj, "{\"err\":40000,\"position\":3,\"message\":\"bad expr\"}");

	zassert_ok(nrf_cloud_obj_shadow_transform_decode(&shadow_obj, &tf));
	zassert_true(tf.is_err);
	zassert_equal(tf.error.code, 40000);
	zassert_equal(tf.error.pos, 3);

	/* Error data must outlive the input object; the MQTT FSM frees it before
	 * notifying the application.
	 */
	nrf_cloud_obj_free(&shadow_obj);
	zassert_true(live_allocs > 0);
	zassert_str_equal(tf.error.msg, "bad expr");

	nrf_cloud_obj_shadow_transform_free(&tf);
	zassert_equal(live_allocs, 0, "%d allocation(s) leaked", live_allocs);
}

ZTEST(nrf_cloud_shadow_decode, test_delta_state_no_leak)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(shadow_obj);
	struct nrf_cloud_obj_shadow_delta delta;

	decode_input(&shadow_obj,
		     "{\"version\":7,\"timestamp\":1000,\"state\":{\"config\":{\"x\":1}}}");

	zassert_ok(nrf_cloud_obj_shadow_delta_decode(&shadow_obj, &delta));
	zassert_false(delta.is_err);
	zassert_equal(delta.ver, 7);
	zassert_not_null(delta.state.json);

	nrf_cloud_obj_free(&shadow_obj);
	nrf_cloud_obj_shadow_delta_free(&delta);
	zassert_equal(live_allocs, 0, "%d allocation(s) leaked", live_allocs);
}

ZTEST(nrf_cloud_shadow_decode, test_delta_error_no_leak)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(shadow_obj);
	struct nrf_cloud_obj_shadow_delta delta;

	decode_input(&shadow_obj,
		     "{\"version\":7,\"timestamp\":1000,\"err\":40100,\"message\":\"conflict\"}");

	zassert_ok(nrf_cloud_obj_shadow_delta_decode(&shadow_obj, &delta));
	zassert_true(delta.is_err);
	zassert_equal(delta.error.code, 40100);

	nrf_cloud_obj_free(&shadow_obj);
	zassert_true(live_allocs > 0);
	zassert_str_equal(delta.error.msg, "conflict");

	nrf_cloud_obj_shadow_delta_free(&delta);
	zassert_equal(live_allocs, 0, "%d allocation(s) leaked", live_allocs);
}

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef CONFIG_TEST_SUIT_PLATFORM_FETCH_VARIANT_APP
#include <zephyr/ztest.h>
#include <suit_platform.h>
#include <dfu/suit_dfu_fetch_source.h>
#include <mock_suit_ipuc.h>
#include <zephyr/fff.h>
#include <suit_dfu_cache_rw.h>
#include <drivers/flash/flash_ipuc.h>
#include <suit_plat_mem_util.h>

#if IS_ENABLED(CONFIG_FLASH)
#if (DT_NODE_EXISTS(DT_CHOSEN(zephyr_flash_controller)))
#define SUIT_PLAT_INTERNAL_NVM_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller))
#else
#define SUIT_PLAT_INTERNAL_NVM_DEV DEVICE_DT_GET(DT_CHOSEN(zephyr_flash))
#endif
#else
#define SUIT_PLAT_INTERNAL_NVM_DEV NULL
#endif

static const uint8_t test_data[] = {0,	1,  2,	3,  4,	5,  6,	7,  8,	9,  10, 11, 12, 13, 14, 15,
				    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
				    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
				    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
static const uint8_t test_data_sdfw[] = {'S', 'D', 'F', 'W', '.', 'b', 'i', 'n'};
static const uint8_t test_data_scfw[] = {'S', 'y', 's', 'C', 't', 'r', 'l', '.', 'b', 'i', 'n'};

/* clang-format off */
static const uint8_t component_rad_0x80000_0x1000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x20, /* Data */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x08, 0x00, 0x00,
		0x43, /* bstr(3) */
			0x19, /* uint16 */
				0x10, 0x00,
};
static const uint8_t component_app_0x90000_0x1000[] = {
	0x84, /* array(4) */
		0x44, /* bstr(4) */
			0x63, /* text(3) */
				'M', 'E', 'M',
		0x41, /* bstr(1) */
			0x20, /* Data */
		0x45, /* bstr(5) */
			0x1a, /* uint32 */
				0x00, 0x09, 0x00, 0x00,
		0x43, /* bstr(3) */
			0x19, /* uint16 */
				0x10, 0x00,
};
/* clang-format on */

static suit_plat_err_t suit_ipuc_get_count_single_fake_func(size_t *count)
{
	zassert_not_null(count, "Caller must provide non-Null pointer");

	*count = 1;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t
suit_ipuc_get_info_0x80000_0x1000_fake_func(size_t idx, struct zcbor_string *component_id,
					    suit_manifest_role_t *role)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_not_null(role, "Caller must provide non-Null pointer to read role");
	zassert_equal(idx, 0, "Unexpected idx value");

	component_id->value = component_rad_0x80000_0x1000;
	component_id->len = sizeof(component_rad_0x80000_0x1000);
	*role = SUIT_MANIFEST_RAD_LOCAL_1;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_write_rad_fake_func(struct zcbor_string *component_id,
						     size_t offset, uintptr_t buffer,
						     size_t chunk_size, bool last_chunk)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_equal(component_id->len, sizeof(component_rad_0x80000_0x1000),
		      "Invalid component ID length passed");
	zassert_equal(memcmp(component_id->value, component_rad_0x80000_0x1000, component_id->len),
		      0, "Invalid component ID value passed");

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_get_count_fake_func(size_t *count)
{
	zassert_not_null(count, "Caller must provide non-Null pointer");

	*count = 2;

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_get_info_fake_func(size_t idx, struct zcbor_string *component_id,
						    suit_manifest_role_t *role)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_not_null(role, "Caller must provide non-Null pointer to read role");
	zassert_true(idx < 2, "Unexpected idx value");

	if (idx == 0) {
		component_id->value = component_rad_0x80000_0x1000;
		component_id->len = sizeof(component_rad_0x80000_0x1000);
		*role = SUIT_MANIFEST_RAD_LOCAL_1;
	} else if (idx == 1) {
		component_id->value = component_app_0x90000_0x1000;
		component_id->len = sizeof(component_app_0x90000_0x1000);
		*role = SUIT_MANIFEST_APP_LOCAL_1;
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t suit_ipuc_write_fake_func(struct zcbor_string *component_id, size_t offset,
						 uintptr_t buffer, size_t chunk_size,
						 bool last_chunk)
{
	zassert_not_null(component_id, "Caller must provide non-Null pointer to read component_id");
	zassert_equal(component_id->len, sizeof(component_app_0x90000_0x1000),
		      "Invalid component ID length passed");
	zassert_equal(memcmp(component_id->value, component_app_0x90000_0x1000, component_id->len),
		      0, "Invalid component ID value passed");

	zassert_equal(flash_write(SUIT_PLAT_INTERNAL_NVM_DEV, 0x90000 + offset, (uint8_t *)buffer,
				  chunk_size),
		      0, "Unable to modify NVM to match requested changes");

	return SUIT_PLAT_SUCCESS;
}

static int fetch_request_fn(const uint8_t *uri, size_t uri_length, uint32_t session_id)
{
	int rc = 0;

	if (((uri_length == sizeof("http://databucket.com")) &&
	     memcmp(uri, "http://databucket.com", uri_length) == 0)) {
		zassert_equal(uri_length, sizeof("http://databucket.com"),
			      "Unexpected URI length: %d", uri_length);
		zassert_equal(memcmp(uri, "http://databucket.com", uri_length), 0,
			      "Unexpected URI value");
		rc = suit_dfu_fetch_source_write_fetched_data(session_id, test_data,
							      sizeof(test_data));
	} else if (((uri_length == sizeof("http://databucket.com/sdfw.bin")) &&
		    memcmp(uri, "http://databucket.com/sdfw.bin", uri_length) == 0)) {
		zassert_equal(uri_length, sizeof("http://databucket.com/sdfw.bin"),
			      "Unexpected URI length: %d", uri_length);
		zassert_equal(memcmp(uri, "http://databucket.com/sdfw.bin", uri_length), 0,
			      "Unexpected URI value");
		rc = suit_dfu_fetch_source_write_fetched_data(session_id, test_data_sdfw,
							      sizeof(test_data_sdfw));
	} else if (((uri_length == sizeof("http://databucket.com/scfw.bin")) &&
		    memcmp(uri, "http://databucket.com/scfw.bin", uri_length) == 0)) {
		zassert_equal(uri_length, sizeof("http://databucket.com/scfw.bin"),
			      "Unexpected URI length: %d", uri_length);
		zassert_equal(memcmp(uri, "http://databucket.com/scfw.bin", uri_length), 0,
			      "Unexpected URI value");
		rc = suit_dfu_fetch_source_write_fetched_data(session_id, test_data_scfw,
							      sizeof(test_data_scfw));
	} else {
		zassert_true(false, "Unsupporte URI value");
	}

	if (rc != 0) {
		return SUIT_PLAT_ERR_CRASH;
	}

	return SUIT_PLAT_SUCCESS;
}

static void test_before(void *data)
{
	/* Reset mocks */
	mock_suit_ipuc_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();

	/* Erase NVM area */
	zassert_equal(flash_erase(SUIT_PLAT_INTERNAL_NVM_DEV, 0x90000, 0x1000), 0,
		      "Unable to erase NVM before test execution");

	/* Load IPUCs as SUIT caches */
	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;

	suit_dfu_cache_rw_init();
	zassert_equal(suit_dfu_cache_0_resize(), SUIT_PLAT_SUCCESS,
		      "Failed to initialize SUIT cache before test execution.");
	zassert_equal(suit_ipuc_get_count_fake.call_count, 2,
		      "Incorrect number of suit_ipuc_get_count() calls (%d)",
		      suit_ipuc_get_count_fake.call_count);
	/* 2x ((count: search for max) + (info of the max))*/
	zassert_equal(suit_ipuc_get_info_fake.call_count, 6,
		      "Incorrect number of suit_ipuc_get_info() calls (%d)",
		      suit_ipuc_get_info_fake.call_count);

	/* Reset mocks */
	mock_suit_ipuc_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

static void test_after(void *data)
{
	uintptr_t cache_ipuc_address = (uintptr_t)suit_plat_mem_ptr_get(0x90000);
	size_t cache_ipuc_size = 0x1000;
	struct device *flash_ipuc = NULL;
	uintptr_t ipuc_address = 0;
	size_t ipuc_size = 0;

	/* SUIT cache IPUC is not released automatically as it is possible to populate cache
	 * with miltiple payloads.
	 * Relase them manually to preserve constant number of IPUCs for all test cases.
	 */
	flash_ipuc =
		flash_ipuc_find(cache_ipuc_address, cache_ipuc_size, &ipuc_address, &ipuc_size);
	if (flash_ipuc != NULL) {
		flash_ipuc_release(flash_ipuc);
	}
}

ZTEST_SUITE(fetch_app_mem_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(fetch_app_mem_tests, test_fetch_to_ipuc_mem_OK)
{
	struct zcbor_string uri = {.value = "http://databucket.com",
				   .len = sizeof("http://databucket.com")};
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* [h'MEM', h'20', h'1A00080000', h'191000'] */
	uint8_t valid_dst_value[] = {0x84, 0x44, 0x63, 'M',  'E',  'M',	 0x41, 0x20, 0x45,
				     0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};

	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_single_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_0x80000_0x1000_fake_func;
	suit_ipuc_write_setup_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_rad_fake_func;

	zassert_equal(suit_dfu_fetch_source_register(fetch_request_fn), SUIT_PLAT_SUCCESS,
		      "Failed to register fetch source mock");

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch(dst_handle, &uri, &valid_dst_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	zassert_equal(suit_ipuc_get_count_fake.call_count, 2,
		      "Incorrect number of suit_ipuc_get_count() calls (%d)",
		      suit_ipuc_get_count_fake.call_count);
	zassert_equal(suit_ipuc_get_info_fake.call_count, 2,
		      "Incorrect number of suit_ipuc_get_info() calls (%d)",
		      suit_ipuc_get_info_fake.call_count);
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls (%d)",
		      suit_ipuc_write_setup_fake.call_count);
	zassert_equal(suit_ipuc_write_fake.call_count, 2,
		      "Incorrect number of suit_ipuc_write() calls (%d)",
		      suit_ipuc_write_fake.call_count);

	zassert_equal(suit_ipuc_write_fake.arg1_history[0], 0,
		      "Unexpected offset in the first suit_ipuc_write() call (0x%x)",
		      suit_ipuc_write_fake.arg1_history[0]);
	zassert_equal_ptr(suit_ipuc_write_fake.arg2_history[0], test_data,
			  "Unexpected data pointer in the first suit_ipuc_write() call (0x%x)",
			  suit_ipuc_write_fake.arg2_history[0]);
	zassert_equal(suit_ipuc_write_fake.arg3_history[0], sizeof(test_data),
		      "Unexpected data size in the first suit_ipuc_write() call (0x%x)",
		      suit_ipuc_write_fake.arg3_history[0]);
	zassert_equal(suit_ipuc_write_fake.arg4_history[0], false,
		      "Write closed on the first suit_ipuc_write() call");

	zassert_equal(suit_ipuc_write_fake.arg1_history[1], sizeof(test_data),
		      "Unexpected offset in the second suit_ipuc_write() call (0x%x)",
		      suit_ipuc_write_fake.arg1_history[1]);
	zassert_equal_ptr(suit_ipuc_write_fake.arg2_history[1], NULL,
			  "Unexpected data pointer in the second suit_ipuc_write() call (0x%x)",
			  suit_ipuc_write_fake.arg2_history[1]);
	zassert_equal(suit_ipuc_write_fake.arg3_history[1], 0,
		      "Unexpected data size in the second suit_ipuc_write() call (0x%x)",
		      suit_ipuc_write_fake.arg3_history[1]);
	zassert_equal(suit_ipuc_write_fake.arg4_history[1], true,
		      "Write was not closed on the second suit_ipuc_write() call");

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);
}

ZTEST_SUITE(fetch_app_cache_tests, NULL, NULL, test_before, test_after, NULL);

ZTEST(fetch_app_cache_tests, test_fetch_to_ipuc_cache_OK)
{
	struct zcbor_string uri = {.value = "http://databucket.com",
				   .len = sizeof("http://databucket.com")};
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* clang-format off */
	uint8_t valid_dst_value[] = {
		0x82, /* array(2) */
			0x4B, /* bytes(11) */
				0x6A, /* text(10) */
					'C', 'A', 'C', 'H', 'E', '_', 'P', 'O', 'O', 'L',
			0x42, /* bytes(2) */
				0x18, /* uint8_t */
					CONFIG_SUIT_CACHE_APP_IPUC_ID,
	};
	/* clang-format on */
	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};
	size_t cached_size = 0;
	const uint8_t *cached_payload = NULL;

	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;
	suit_ipuc_write_setup_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;

	zassert_equal(suit_dfu_fetch_source_register(fetch_request_fn), SUIT_PLAT_SUCCESS,
		      "Failed to register fetch source mock");

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_fetch(dst_handle, &uri, &valid_dst_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls (%d)",
		      suit_ipuc_write_setup_fake.call_count);
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls (%d)",
		      suit_ipuc_get_count_fake.call_count);
	zassert_equal(suit_ipuc_get_info_fake.call_count, 3,
		      "Incorrect number of suit_ipuc_get_info() calls (%d)",
		      suit_ipuc_get_info_fake.call_count);

	zassert_true(suit_ipuc_write_fake.call_count >= 4,
		     "Too small number of suit_ipuc_write() calls (%d)",
		     suit_ipuc_write_fake.call_count);

	size_t last_write = suit_ipuc_write_fake.call_count - 1;

	for (size_t i = 0; i < suit_ipuc_write_fake.call_count; i++) {
		printk("\t%d\n", suit_ipuc_write_fake.arg3_history[i]);
		zassert_equal(suit_ipuc_write_fake.arg4_history[i], false,
			      "Write closed too early in suit_ipuc_write() call");
	}

	/* partition header + slot header + URI + (data length) */
	/* data */

	/* slot length */
	zassert_equal(suit_ipuc_write_fake.arg3_history[last_write - 1], 4,
		      "Unexpected data size in the third suit_ipuc_write() call (0x%x)",
		      suit_ipuc_write_fake.arg3_history[last_write - 1]);

	/* slot end marker */
	zassert_equal(suit_ipuc_write_fake.arg3_history[last_write], 1,
		      "Unexpected data size in the fourth suit_ipuc_write() call (0x%x)",
		      suit_ipuc_write_fake.arg3_history[last_write]);

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);

	/* Verify the cache contents. */
	zassert_equal(suit_dfu_cache_search(uri.value, uri.len, &cached_payload, &cached_size),
		      SUIT_PLAT_SUCCESS, "Failed to find cached entry inside the SUIT cache");
	zassert_equal(cached_size, sizeof(test_data), "Invalid size of the cached data (%d)",
		      cached_size);
	zassert_equal(memcmp(cached_payload, test_data, sizeof(test_data)), 0,
		      "Invalid cached payload");
}

ZTEST(fetch_app_cache_tests, test_fetch_to_ipuc_cache_sdfw_scfw_OK)
{
	struct zcbor_string uri_sdfw = {.value = "http://databucket.com/sdfw.bin",
					.len = sizeof("http://databucket.com/sdfw.bin")};
	struct zcbor_string uri_scfw = {.value = "http://databucket.com/scfw.bin",
					.len = sizeof("http://databucket.com/scfw.bin")};
	/* Create handle that will be used as destination */
	suit_component_t dst_handle;
	/* clang-format off */
	uint8_t valid_dst_value[] = {
		0x82, /* array(2) */
			0x4B, /* bytes(11) */
				0x6A, /* text(10) */
					'C', 'A', 'C', 'H', 'E', '_', 'P', 'O', 'O', 'L',
			0x42, /* bytes(2) */
				0x18, /* uint8_t */
					CONFIG_SUIT_CACHE_SDFW_IPUC_ID,
	};
	/* clang-format on */
	struct zcbor_string valid_dst_component_id = {
		.value = valid_dst_value,
		.len = sizeof(valid_dst_value),
	};
	size_t cached_size = 0;
	const uint8_t *cached_payload = NULL;

	suit_ipuc_get_count_fake.custom_fake = suit_ipuc_get_count_fake_func;
	suit_ipuc_get_info_fake.custom_fake = suit_ipuc_get_info_fake_func;
	suit_ipuc_write_setup_fake.return_val = SUIT_PLAT_SUCCESS;
	suit_ipuc_write_fake.custom_fake = suit_ipuc_write_fake_func;

	zassert_equal(suit_dfu_fetch_source_register(fetch_request_fn), SUIT_PLAT_SUCCESS,
		      "Failed to register fetch source mock");

	int ret = suit_plat_create_component_handle(&valid_dst_component_id, false, &dst_handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	/* Fetch SDFW binary */
	ret = suit_plat_fetch(dst_handle, &uri_sdfw, &valid_dst_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls (%d)",
		      suit_ipuc_write_setup_fake.call_count);
	zassert_equal(suit_ipuc_get_count_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_get_count() calls (%d)",
		      suit_ipuc_get_count_fake.call_count);
	zassert_equal(suit_ipuc_get_info_fake.call_count, 3,
		      "Incorrect number of suit_ipuc_get_info() calls (%d)",
		      suit_ipuc_get_info_fake.call_count);

	/* Fetch SCFW binary using the same IPUC and component context */
	ret = suit_plat_fetch(dst_handle, &uri_scfw, &valid_dst_component_id, NULL);
	zassert_equal(ret, SUIT_SUCCESS, "suit_plat_fetch failed - error %i", ret);

	/* The write_setup call count should remain the same - IPUC must not be erased at this
	 * point.
	 */
	zassert_equal(suit_ipuc_write_setup_fake.call_count, 1,
		      "Incorrect number of suit_ipuc_write_setup() calls (%d)",
		      suit_ipuc_write_setup_fake.call_count);

	ret = suit_plat_release_component_handle(dst_handle);
	zassert_equal(ret, SUIT_SUCCESS, "dst_handle release failed - error %i", ret);

	/* Verify the cache contents. */
	zassert_equal(
		suit_dfu_cache_search(uri_sdfw.value, uri_sdfw.len, &cached_payload, &cached_size),
		SUIT_PLAT_SUCCESS, "Failed to find SDFW entry inside the SUIT cache");
	zassert_equal(cached_size, sizeof(test_data_sdfw), "Invalid SDFW size in cache (%d)",
		      cached_size);
	zassert_equal(memcmp(cached_payload, test_data_sdfw, sizeof(test_data_sdfw)), 0,
		      "Invalid SDFW payload");

	zassert_equal(
		suit_dfu_cache_search(uri_scfw.value, uri_scfw.len, &cached_payload, &cached_size),
		SUIT_PLAT_SUCCESS, "Failed to find SCFW entry inside the SUIT cache");
	zassert_equal(cached_size, sizeof(test_data_scfw), "Invalid SCFW size in cache (%d)",
		      cached_size);
	zassert_equal(memcmp(cached_payload, test_data_scfw, sizeof(test_data_scfw)), 0,
		      "Invalid SCFW payload");
}
#endif /* CONFIG_TEST_SUIT_PLATFORM_FETCH_VARIANT_APP */

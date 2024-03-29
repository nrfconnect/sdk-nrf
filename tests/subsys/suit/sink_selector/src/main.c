/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_ram_sink.h>
#include <suit_memptr_sink.h>
#include <suit_flash_sink.h>
#include <suit_sink.h>
#include <suit_sink_selector.h>
#include <suit_platform.h>
#include <suit_platform_internal.h>

#include <suit_types.h>
#include <suit_memptr_storage.h>
#include <suit_platform_internal.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

ZTEST_SUITE(sink_selector_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(sink_selector_tests, test_select_memptr_sink_OK)
{
	suit_component_t handle;
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};
	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};
	struct stream_sink sink;

	int ret = suit_plat_create_component_handle(&valid_component_id, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	int err = suit_sink_select(handle, &sink);

	zassert_equal(err, SUIT_SUCCESS, "sink_selector: selecting memptr_sink failed - error %i",
		      err);
}

ZTEST(sink_selector_tests, test_select_flash_sink_OK)
{
	suit_component_t handle;
	/* [h'MEM', h'02', h'1A00080000', h'191000'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x02, 0x45,
				 0x1A, 0x00, 0x08, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};
	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};
	struct stream_sink sink;

	int ret = suit_plat_create_component_handle(&valid_component_id, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	int err = suit_sink_select(handle, &sink);

	zassert_equal(err, SUIT_SUCCESS, "sink_selector: selecting flash_sink failed - error %i",
		      err);
}

ZTEST(sink_selector_tests, test_select_ram_sink_OK)
{
	suit_component_t handle;

	/* ['MEM', h'02', h'1A20000000', h'191000'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x02, 0x45,
				 0x1A, 0x20, 0x00, 0x00, 0x00, 0x43, 0x19, 0x10, 0x00};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};
	struct stream_sink sink;

	int ret = suit_plat_create_component_handle(&valid_component_id, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	int err = suit_sink_select(handle, &sink);

	zassert_equal(err, SUIT_SUCCESS, "sink_selector: selecting flash_sink failed - error %i",
		      err);
}

#if CONFIG_SUIT_STREAM_SINK_SDFW
ZTEST(sink_selector_tests, test_select_sdfw_sink_OK)
{
	suit_component_t handle;
	/* [h'SOC_SPEC', h'01'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'S', 'O',  'C', '_',
				 'S',  'P',  'E',  'C', 0x41, 0x01};
	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};
	struct stream_sink sink;

	int ret = suit_plat_create_component_handle(&valid_component_id, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	int err = suit_sink_select(handle, &sink);

	zassert_equal(err, SUIT_SUCCESS, "sink_selector: selecting swdf_sink failed - error %i",
		      err);
}
#endif

/* Invalid component_id - zcbor wise */
ZTEST(sink_selector_tests, test_select_invalid_component_id)
{
	suit_component_t handle;
	/* [h'MEM', h'02', h'1000080000', h'08'] */
	uint8_t invalid_value[] = {0x84, 0x44, 0x63, 'M',  'E',	 'M',  0x41, 0x02,
				   0x45, 0x10, 0x00, 0x08, 0x00, 0x00, 0x41, 0x08};
	struct zcbor_string invalid_component_id = {
		.value = invalid_value,
		.len = sizeof(invalid_value),
	};

	int ret = suit_plat_create_component_handle(&invalid_component_id, &handle);

	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "create_component_handle unexpected error %i", ret);
}

/* Invalid component */
ZTEST(sink_selector_tests, test_select_unsupported_component)
{
	suit_component_t handle;
	/* [h'INSTLD_MFST', h'56dc9a1428d852d3bd62e77a08bc8b91'] */
	uint8_t invalid_value[] = {0x82, 0x4c, 0x6b, 'I',  'N',	 'S',  'T',  'L',  'D',	 '_',  'M',
				   'F',	 'S',  'T',  0x50, 0x56, 0xdc, 0x9a, 0x14, 0x28, 0xd8, 0x52,
				   0xd3, 0xbd, 0x62, 0xe7, 0x7a, 0x08, 0xbc, 0x8b, 0x91};
	struct zcbor_string invalid_component_id = {
		.value = invalid_value,
		.len = sizeof(invalid_value),
	};
	struct stream_sink sink;

	int ret = suit_plat_create_component_handle(&invalid_component_id, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	int err = suit_sink_select(handle, &sink);

	zassert_not_equal(err, SUIT_SUCCESS,
			  "sink_selector should have failed - unsupported component");
}

ZTEST(sink_selector_tests, test_suit_sink_select_invalid_handle)
{
	suit_component_t handle = 0;
	struct stream_sink sink;

	int err = suit_sink_select(handle, &sink);

	zassert_not_equal(err, SUIT_SUCCESS, "sink_selector should have failed - invalid handle");
}

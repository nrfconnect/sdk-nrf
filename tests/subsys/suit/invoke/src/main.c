/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_platform.h>
#include <suit_types.h>

ZTEST_SUITE(invoke_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(invoke_tests, test_invoke_OK)
{
	suit_component_t handle;
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* [h'MEM', h'02', h'1A00080000', h'09'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x00,
				 0x45, 0x1A, 0x00, 0x08, 0x00, 0x00, 0x41, 0x08};
#else  /* CONFIG_SOC_SERIES_NRF54HX */
	/* [h'MEM', h'00', h'1A00080000', h'08'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x00,
				 0x45, 0x1A, 0x00, 0x08, 0x00, 0x00, 0x41, 0x08};
#endif /* CONFIG_SOC_SERIES_NRF54HX */

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, NULL);
	zassert_not_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER, "suit_plat_invoke failed - error %i",
			  ret);
	zassert_not_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
			  "suit_plat_invoke failed - error %i", ret);
}

ZTEST(invoke_tests, test_invoke_NOK_unsupported_component_type)
{
	suit_component_t handle;
	/* [h'CAND_IMG', h'02'] */
	uint8_t valid_value[] = {0x82, 0x49, 0x68, 'C', 'A',  'N', 'D',
				 '_',  'I',  'M',  'G', 0x41, 0x02};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, NULL);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "suit_plat_invoke should have failed - unsupported component type");
}

ZTEST(invoke_tests, test_invoke_NOK_unsupported_cpu_id)
{
	suit_component_t handle;
	/* [h'MEM', h'06', h'1A00080000', h'08'] */
	uint8_t valid_value[] = {0x84, 0x44, 0x63, 'M',	 'E',  'M',  0x41, 0x06,
				 0x45, 0x1A, 0x00, 0x08, 0x00, 0x00, 0x41, 0x08};

	struct zcbor_string valid_component_id = {
		.value = valid_value,
		.len = sizeof(valid_value),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, NULL);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER,
		      "suit_plat_invoke should have failed - unsupported cpu_id");
}

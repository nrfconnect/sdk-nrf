/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_plat_err.h>
#include <suit_platform.h>
#include <suit_types.h>

ZTEST_SUITE(invoke_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(invoke_tests, test_invoke_OK)
{
	suit_component_t handle;
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* [h'MEM', h'02', h'1A00080000', h'08'] */
	/* NCSDK-30466: Sitch to the correct CPU IDs */
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

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, NULL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "suit_plat_invoke failed - error %i", ret);
	zassert_not_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
			  "suit_plat_invoke failed - error %i", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
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

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, NULL);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_COMPONENT_ID,
		      "suit_plat_invoke should have failed - unsupported component type");
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
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

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, NULL);
	zassert_equal(ret, SUIT_ERR_CRASH,
		      "suit_plat_invoke should have failed - unsupported cpu_id");
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_tests, test_invoke_NOK_asynchronous_with_timeout)
{
	suit_component_t handle;
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* [h'MEM', h'02', h'1A00080000', h'08'] */
	/* NCSDK-30466: Sitch to the correct CPU IDs */
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
	/* clang-format off */
	uint8_t invoke_args_buf[] = {
		0xA2, /* map (2 elements) */
			0x01, /* key: synchronous-invoke */
				0xF4, /* value: false */
			0x02, /* key: timeout */
				0x0A, /* value: 10ms */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER,
		      "Asynchronous invoke with timeout not detected (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_tests, test_invoke_NOK_asynchronous_with_unsupported_args)
{
	suit_component_t handle;
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* [h'MEM', h'02', h'1A00080000', h'08'] */
	/* NCSDK-30466: Sitch to the correct CPU IDs */
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
	/* clang-format off */
	uint8_t invoke_args_buf[] = {
		0xA2, /* map (2 elements) */
			0x01, /* key: synchronous-invoke */
				0xF4, /* value: false */
			0x03, /* key: unsupported */
				0x0A, /* value: 10 */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER,
		      "Asynchronous invoke with unsupported arguments not detected (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_tests, test_invoke_OK_asynchronous_without_timeout)
{
	suit_component_t handle;
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* [h'MEM', h'02', h'1A00080000', h'08'] */
	/* NCSDK-30466: Sitch to the correct CPU IDs */
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
	/* clang-format off */
	uint8_t invoke_args_buf[] = {
		0xA1, /* map (1 element) */
			0x01, /* key: synchronous-invoke */
				0xF4, /* value: false */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	zassert_equal(ret, SUIT_SUCCESS,
		      "Asynchronous invoke without timeout not supported (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_tests, test_invoke_NOK_synchronous_without_timeout)
{
	suit_component_t handle;
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* [h'MEM', h'02', h'1A00080000', h'08'] */
	/* NCSDK-30466: Sitch to the correct CPU IDs */
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
	/* clang-format off */
	uint8_t invoke_args_buf[] = {
		0xA1, /* map (1 element) */
			0x01, /* key: synchronous-invoke */
				0xF5, /* value: true */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER,
		      "Synchronous invoke without timeout not detected (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_tests, test_invoke_NOK_synchronous_with_unsupported_args)
{
	suit_component_t handle;
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* [h'MEM', h'02', h'1A00080000', h'08'] */
	/* NCSDK-30466: Sitch to the correct CPU IDs */
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
	/* clang-format off */
	uint8_t invoke_args_buf[] = {
		0xA3, /* map (3 elements) */
			0x01, /* key: synchronous-invoke */
				0xF5, /* value: true */
			0x02, /* key: timeout */
				0x0A, /* value: 10ms */
			0x03, /* key: unsupported */
				0x0A, /* value: 10 */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	zassert_equal(ret, SUIT_ERR_UNSUPPORTED_PARAMETER,
		      "Synchronous invoke with unsupported arguments not detected (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_tests, test_invoke_OK_synchronous_with_timeout)
{
	suit_component_t handle;
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* [h'MEM', h'02', h'1A00080000', h'08'] */
	/* NCSDK-30466: Sitch to the correct CPU IDs */
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
	/* clang-format off */
	uint8_t invoke_args_buf[] = {
		0xA2, /* map (2 elements) */
			0x01, /* key: synchronous-invoke */
				0xF5, /* value: true */
			0x02, /* key: timeout */
				0x0A, /* value: 10ms */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);

	/* Since SSF service to inform about successful boot is not called - the error indicates
	 * timeout event
	 */
	zassert_equal(ret, SUIT_FAIL_CONDITION,
		      "Synchronous invoke with timeout not supported (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

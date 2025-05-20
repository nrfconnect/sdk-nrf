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
#include <suit_events.h>

#define STACKSIZE 1024
#define PRIORITY  (CONFIG_ZTEST_THREAD_PRIORITY - 1)

static void invoke_app_10ms(void)
{
	k_sleep(K_MSEC(10));
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* NCSDK-30466: Sitch to the correct CPU IDs */
	(void)suit_event_post(0x00, SUIT_EVENT_INVOKE_SUCCESS);
#else  /* CONFIG_SOC_SERIES_NRF54HX */
	(void)suit_event_post(0x00, SUIT_EVENT_INVOKE_SUCCESS);
#endif /* CONFIG_SOC_SERIES_NRF54HX */
	printk("[EVT] APP booted\n");
}

static void invoke_app_fail_20ms(void)
{
	k_sleep(K_MSEC(20));
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* NCSDK-30466: Sitch to the correct CPU IDs */
	(void)suit_event_post(0x00, SUIT_EVENT_INVOKE_FAIL);
#else  /* CONFIG_SOC_SERIES_NRF54HX */
	(void)suit_event_post(0x00, SUIT_EVENT_INVOKE_FAIL);
#endif /* CONFIG_SOC_SERIES_NRF54HX */
	printk("[EVT] APP failed to boot\n");
}

static void invoke_app_both_10ms(void)
{
	k_sleep(K_MSEC(10));
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* NCSDK-30466: Sitch to the correct CPU IDs */
	(void)suit_event_post(0x00, SUIT_EVENT_INVOKE_FAIL | SUIT_EVENT_INVOKE_SUCCESS);
#else  /* CONFIG_SOC_SERIES_NRF54HX */
	(void)suit_event_post(0x00, SUIT_EVENT_INVOKE_FAIL | SUIT_EVENT_INVOKE_SUCCESS);
#endif /* CONFIG_SOC_SERIES_NRF54HX */
	printk("[EVT] APP booted and failed at the same time\n");
}

static void invoke_rad_10ms(void)
{
	k_sleep(K_MSEC(10));
#ifdef CONFIG_SOC_SERIES_NRF54HX
	/* NCSDK-30466: Sitch to the correct CPU IDs */
	(void)suit_event_post(0x01, SUIT_EVENT_INVOKE_SUCCESS);
#else  /* CONFIG_SOC_SERIES_NRF54HX */
	(void)suit_event_post(0x01, SUIT_EVENT_INVOKE_SUCCESS);
#endif /* CONFIG_SOC_SERIES_NRF54HX */
	printk("[EVT] RAD booted\n");
}

static void invoke_unknown_10ms(void)
{
	k_sleep(K_MSEC(10));
	(void)suit_event_post(0x05, SUIT_EVENT_INVOKE_SUCCESS);
	printk("[EVT] UNKNOWN booted\n");
}

static void invoke_all_10ms(void)
{
	k_sleep(K_MSEC(10));
	(void)suit_event_post(SUIT_EVENT_CPU_ID_ANY, SUIT_EVENT_INVOKE_SUCCESS);
	printk("[EVT] ALL booted\n");
}

K_THREAD_DEFINE(THREAD_APP_10MS, STACKSIZE, invoke_app_10ms, NULL, NULL, NULL, PRIORITY, 0, -1);

K_THREAD_DEFINE(THREAD_APP_FAIL_20MS, STACKSIZE, invoke_app_fail_20ms, NULL, NULL, NULL, PRIORITY,
		0, -1);

K_THREAD_DEFINE(THREAD_APP_BOTH_10MS, STACKSIZE, invoke_app_both_10ms, NULL, NULL, NULL, PRIORITY,
		0, -1);

K_THREAD_DEFINE(THREAD_RAD_10MS, STACKSIZE, invoke_rad_10ms, NULL, NULL, NULL, PRIORITY, 0, -1);

K_THREAD_DEFINE(THREAD_UNKNOWN_10MS, STACKSIZE, invoke_unknown_10ms, NULL, NULL, NULL, PRIORITY, 0,
		-1);

K_THREAD_DEFINE(THREAD_ALL_10MS, STACKSIZE, invoke_all_10ms, NULL, NULL, NULL, PRIORITY, 0, -1);

ZTEST_SUITE(invoke_syn_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(invoke_syn_tests, test_invoke_sync_OK)
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
				0x14, /* value: 20ms */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	/* Start a delayed successful APP invoke event thread. */
	k_thread_start(THREAD_APP_10MS);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	k_thread_join(THREAD_APP_10MS, K_FOREVER);

	zassert_equal(ret, SUIT_SUCCESS, "Synchronous invoke with timeout failed (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_syn_tests, test_invoke_sync_OK_failed)
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
				0x17, /* value: 23ms */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	/* Start a delayed failed APP invoke event thread. */
	k_thread_start(THREAD_APP_FAIL_20MS);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	k_thread_join(THREAD_APP_FAIL_20MS, K_FOREVER);

	zassert_equal(ret, SUIT_FAIL_CONDITION,
		      "Synchronous invoke with timeout did not fail (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_syn_tests, test_invoke_sync_NOK_other_core)
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
				0x14, /* value: 20ms */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	/* Start a delayed successful RAD invoke event thread. */
	k_thread_start(THREAD_RAD_10MS);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	k_thread_join(THREAD_RAD_10MS, K_FOREVER);

	zassert_equal(ret, SUIT_FAIL_CONDITION,
		      "Synchronous invoke with timeout did not occur (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_syn_tests, test_invoke_sync_NOK_unknown_core)
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
				0x14, /* value: 20ms */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	/* Start a delayed successful invoke event for unknown CPU thread. */
	k_thread_start(THREAD_UNKNOWN_10MS);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	k_thread_join(THREAD_UNKNOWN_10MS, K_FOREVER);

	zassert_equal(ret, SUIT_FAIL_CONDITION,
		      "Synchronous invoke with timeout did not occur (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_syn_tests, test_invoke_sync_OK_all_cores)
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
				0x14, /* value: 20ms */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	/* Start a delayed successful all-cores invoke event thread. */
	k_thread_start(THREAD_ALL_10MS);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	k_thread_join(THREAD_ALL_10MS, K_FOREVER);

	zassert_equal(ret, SUIT_SUCCESS, "Synchronous invoke with timeout failed (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

ZTEST(invoke_syn_tests, test_invoke_sync_OK_two_events)
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
				0x17, /* value: 23ms */
	};
	/* clang-format on */
	struct zcbor_string invoke_args_zcbor = {
		.value = invoke_args_buf,
		.len = sizeof(invoke_args_buf),
	};

	int ret = suit_plat_create_component_handle(&valid_component_id, false, &handle);

	zassert_equal(ret, SUIT_SUCCESS, "create_component_handle failed - error %i", ret);

	/* Start a delayed fairly successful APP invoke event thread. */
	k_thread_start(THREAD_APP_BOTH_10MS);

	ret = suit_plat_invoke(handle, &invoke_args_zcbor);
	k_thread_join(THREAD_APP_BOTH_10MS, K_FOREVER);

	zassert_equal(ret, SUIT_SUCCESS, "Synchronous invoke with timeout failed (err: %d)", ret);
	zassert_equal(SUIT_SUCCESS, suit_plat_release_component_handle(handle),
		      "Unable to release component handle after test");
}

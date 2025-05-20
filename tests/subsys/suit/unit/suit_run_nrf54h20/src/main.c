/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <mocks.h>
#include <mocks_sdfw.h>
#include <suit_cpu_run.h>
#include <drivers/nrfx_common.h>

#define TEST_FAKE_RUN_ADDRESS ((intptr_t)0xDEADBEEF)
#define TEST_FAKE_RUN_ADDRESS2 ((intptr_t)0xABCDEFED)

extern void suit_plat_halt_all_cores_simulate(void);

static void test_before(void *data)
{
	suit_plat_halt_all_cores_simulate();

	/* Reset mocks */
	mocks_reset();
	mocks_sdfw_reset();

	/* Reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(suit_run_nrf54h20_tests, NULL, NULL, test_before, NULL, NULL);

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_halt_unsupported_processors)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_PPR);
	zassert_equal(ret, SUIT_PLAT_ERR_UNSUPPORTED, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_FLPR);
	zassert_equal(ret, SUIT_PLAT_ERR_UNSUPPORTED, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_halt(0xFF);
	zassert_equal(ret, SUIT_PLAT_ERR_INVAL, "Unexpected error code: %d", ret);

	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 0,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_halt_not_running_cores)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_APPLICATION);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_RADIOCORE);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_SYSCTRL);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 0,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_unsupported_cores)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_PPR, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_ERR_UNSUPPORTED, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_FLPR, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_ERR_UNSUPPORTED, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_run(0xFF, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_ERR_INVAL, "Unexpected error code: %d", ret);

	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 0,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(vprs_sysctrl_start_fake.call_count, 0,
		      "Incorrect number of vprs_sysctrl_start() calls");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_already_running)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_APPLICATION, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_APPLICATION, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE, "Unexpected error code: %d", ret);

	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_APPLICATION,
		      "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS,
		      "Incorrect run address passed to reset_mgr_init_and_boot_processor()");

	RESET_FAKE(reset_mgr_init_and_boot_processor);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_RADIOCORE, TEST_FAKE_RUN_ADDRESS2);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_RADIOCORE, TEST_FAKE_RUN_ADDRESS2);
	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE, "Unexpected error code: %d", ret);

	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_RADIOCORE,
		      "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS2,
		      "Incorrect run address passed to reset_mgr_init_and_boot_processor()");


	RESET_FAKE(reset_mgr_init_and_boot_processor);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_SYSCTRL, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_SYSCTRL, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_ERR_INCORRECT_STATE, "Unexpected error code: %d", ret);

	zassert_equal(vprs_sysctrl_start_fake.call_count, 1,
				  "Incorrect number of vprs_sysctrl_start() calls");
	zassert_equal(vprs_sysctrl_start_fake.arg0_val, TEST_FAKE_RUN_ADDRESS,
				  "Incorrect run address passed to vprs_sysctrl_start()");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_local_cores_success)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_APPLICATION, TEST_FAKE_RUN_ADDRESS);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_APPLICATION,
		      "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS,
		      "Incorrect run address passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg2_val, 0,
		      "Incorrect NSVTOR value passed to reset_mgr_init_and_boot_processor()");

	RESET_FAKE(reset_mgr_init_and_boot_processor);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_RADIOCORE, TEST_FAKE_RUN_ADDRESS2);

	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_RADIOCORE,
		      "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS2,
		      "Incorrect run address passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg2_val, 0,
		      "Incorrect NSVTOR value passed to reset_mgr_init_and_boot_processor()");

	zassert_equal(vprs_sysctrl_start_fake.call_count, 0,
		      "Incorrect number of vprs_sysctrl_start() calls");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_local_cores_fail)
{
	reset_mgr_init_and_boot_processor_fake.return_val = -1;

	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_APPLICATION, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_ERR_CRASH, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_APPLICATION,
		      "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS,
		      "Incorrect run address passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg2_val, 0,
		      "Incorrect NSVTOR value passed to reset_mgr_init_and_boot_processor()");

	RESET_FAKE(reset_mgr_init_and_boot_processor);

	reset_mgr_init_and_boot_processor_fake.return_val = -1;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_RADIOCORE, TEST_FAKE_RUN_ADDRESS2);

	zassert_equal(ret, SUIT_PLAT_ERR_CRASH, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_RADIOCORE,
		      "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS2,
		      "Incorrect run address passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg2_val, 0,
		      "Incorrect NSVTOR value passed to reset_mgr_init_and_boot_processor()");

	zassert_equal(vprs_sysctrl_start_fake.call_count, 0,
		      "Incorrect number of vprs_sysctrl_start() calls");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_sysctrl_fail)
{
	vprs_sysctrl_start_fake.return_val = -1;
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_SYSCTRL, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_ERR_CRASH, "Unexpected error code: %d", ret);
	zassert_equal(vprs_sysctrl_start_fake.call_count, 1,
		      "Incorrect number of vprs_sysctrl_start() calls");
	zassert_equal(vprs_sysctrl_start_fake.arg0_val, TEST_FAKE_RUN_ADDRESS,
		      "Incorrect run address passed to vprs_sysctrl_start()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 0,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
}


ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_sysctrl_success)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_SYSCTRL, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(vprs_sysctrl_start_fake.call_count, 1,
		      "Incorrect number of vprs_sysctrl_start() calls");
	zassert_equal(vprs_sysctrl_start_fake.arg0_val, TEST_FAKE_RUN_ADDRESS,
		      "Incorrect run address passed to vprs_sysctrl_start()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 0,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_sysctrl_halt_attempt)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_SYSCTRL, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_SYSCTRL);
	zassert_equal(ret, SUIT_PLAT_ERR_UNSUPPORTED, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 0,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_local_cores_halt_success)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_APPLICATION, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_APPLICATION,
		      "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS,
		      "Incorrect run address passed to reset_mgr_init_and_boot_processor()");

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_APPLICATION);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 1,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");

	mocks_sdfw_reset();

	ret = suit_plat_cpu_run(NRF_PROCESSOR_RADIOCORE, TEST_FAKE_RUN_ADDRESS2);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
			  "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_RADIOCORE,
			  "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS2,
			  "Incorrect run address passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg2_val, 0,
			  "Incorrect NSVTOR value passed to reset_mgr_init_and_boot_processor()");

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_RADIOCORE);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 1,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");
	mocks_sdfw_reset();
}


ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_run_local_cores_halt_fail)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_APPLICATION, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
		      "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_APPLICATION,
		      "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS,
		      "Incorrect run address passed to reset_mgr_init_and_boot_processor()");

	reset_mgr_reset_domains_sync_fake.return_val = -1;

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_APPLICATION);
	zassert_equal(ret, SUIT_PLAT_ERR_CRASH, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 1,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");

	mocks_sdfw_reset();
	suit_plat_halt_all_cores_simulate();

	ret = suit_plat_cpu_run(NRF_PROCESSOR_RADIOCORE, TEST_FAKE_RUN_ADDRESS2);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_init_and_boot_processor_fake.call_count, 1,
			  "Incorrect number of reset_mgr_init_and_boot_processor() calls");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg0_val, NRF_PROCESSOR_RADIOCORE,
			  "Incorrect CPU ID passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg1_val, TEST_FAKE_RUN_ADDRESS2,
			  "Incorrect run address passed to reset_mgr_init_and_boot_processor()");
	zassert_equal(reset_mgr_init_and_boot_processor_fake.arg2_val, 0,
			  "Incorrect NSVTOR value passed to reset_mgr_init_and_boot_processor()");

	reset_mgr_reset_domains_sync_fake.return_val = -1;

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_RADIOCORE);
	zassert_equal(ret, SUIT_PLAT_ERR_CRASH, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 1,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");
	mocks_sdfw_reset();
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_halt_application_core_more_running)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_APPLICATION, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_RADIOCORE, TEST_FAKE_RUN_ADDRESS2);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_APPLICATION);
	zassert_equal(ret, SUIT_PLAT_ERR_UNSUPPORTED, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 0,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");
}

ZTEST(suit_run_nrf54h20_tests, test_suit_plat_cpu_halt_radio_core_more_running)
{
	suit_plat_err_t ret;

	ret = suit_plat_cpu_run(NRF_PROCESSOR_APPLICATION, TEST_FAKE_RUN_ADDRESS);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_run(NRF_PROCESSOR_RADIOCORE, TEST_FAKE_RUN_ADDRESS2);
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unexpected error code: %d", ret);

	ret = suit_plat_cpu_halt(NRF_PROCESSOR_RADIOCORE);
	zassert_equal(ret, SUIT_PLAT_ERR_UNSUPPORTED, "Unexpected error code: %d", ret);
	zassert_equal(reset_mgr_reset_domains_sync_fake.call_count, 0,
		      "Incorrect number of reset_mgr_reset_domains_sync() calls");
}

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 #include <ztest.h>
 #include <tfm_ns_interface.h>
 #include <tfm/tfm_ioctl_api.h>
 #include <pm_config.h>

static bool is_secure(intptr_t ptr)
{
	/* We can not use 'arm_cmse_addr_is_secure here since this firmware
	 * is built as non secure.
	 */
	if (ptr >= CONFIG_PM_SRAM_BASE) {
		/* Check if RAM address is secure */
		return ptr < PM_SRAM_ADDRESS;
	}

	/* Check if flash address is secure */
	return ptr < PM_ADDRESS;
}

void test_tfm_read_service(void)
{
	const uint32_t ficr_start = (NRF_FICR_S_BASE + 0x204);
	const uint32_t ficr_end = ficr_start + 0xA1C;

	uint8_t output[4];
	uint8_t data_length = sizeof(output);
	void *secure_ptr = (size_t *)PM_TFM_SRAM_ADDRESS;

	uint32_t err = 0;
	enum tfm_platform_err_t plt_err;

	/* Verify that test objects is stored in non-secure addresses. */
	zassert_true(!is_secure((intptr_t)output),
			 "Test object is not non-secure");

	/* Verify that secure test objects is stored in secure addresses. */
	zassert_true(is_secure((intptr_t)secure_ptr),
						"Test object is not secure");

	/* Verify that the function fails if it is passed secure pointers */
	plt_err = tfm_platform_mem_read(secure_ptr, ficr_start, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_INVALID_PARAM,
						"Did not fail for secure pointer");
	zassert_equal(err, -1, "Did not fail for secure pointer");

	/* Verify that edge addresses in FICR will return expected values */
	/* Normal execution */
	plt_err = tfm_platform_mem_read(output, ficr_start, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS, "Valid address returned an error!");
	zassert_equal(err, 0, "Valid address returned an error!");

	plt_err = tfm_platform_mem_read(output, ficr_end - data_length, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS, "Valid address returned an error!");
	zassert_equal(err, 0, "Valid address returned an error!");

	/* Expect invalid addresses to finish with result -1 */
	plt_err = tfm_platform_mem_read(output, ficr_start - 1, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_INVALID_PARAM,
							"Invalid address returned an unexpected error");
	zassert_equal(err, -1, "Invalid address did not return expected address");

	plt_err = tfm_platform_mem_read(output, ficr_end - data_length + 1, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_INVALID_PARAM,
							"Invalid address returned an unexpected error");
	zassert_equal(err, -1, "Invalid address did not return expected address");
}

void test_main(void)
{
	ztest_test_suite(test_secure_service,
		ztest_unit_test(test_tfm_read_service));
	ztest_run_test_suite(test_secure_service);
}

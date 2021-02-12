/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <secure_services.h>
#include <pm_config.h>

static const struct fw_info ns_fw_info = { .magic = { FIRMWARE_INFO_MAGIC } };
static uint8_t non_secure_buf[128];

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

void test_spm_firmware_info(void)
{
	int err;
	struct fw_info info;

	/* Prove that the struct is stored in non-secure flash. */
	zassert_true(!is_secure((intptr_t)&ns_fw_info),
		     "Test fw-info is not stored in non-secure firmware");

	/* Normal execution */
	err = spm_firmware_info(PM_SPM_ADDRESS, &info);
	zassert_equal(err, 0, "Got unexpected failure");

	/* Verify that the function fails if the address of the firmware info
	 * is not in secure firmware.
	 */
	err = spm_firmware_info((uint32_t)&ns_fw_info, &info);
	zassert_true(err != 0, "Did not fail for fw_address in secure fw");

	/* Verify that the function fails if the output argument for the info
	 * is in secure RAM. First verify that it passes when it is in
	 * non-secure, indicating that a valid fw_info is found at the
	 * specified address, then verify that it fails for an output pointer
	 * in secure RAM.
	 */
	err = spm_firmware_info(PM_SPM_ADDRESS,
				(struct fw_info *)PM_SPM_SRAM_ADDRESS);
	zassert_true(err != 0, "Did not fail when passing invalid output ptr");
}

void test_spm_s0_active(void)
{
	int err;
	bool output;
	bool *secure_ptr = (bool *)PM_SPM_SRAM_ADDRESS;

	/* Prove that test objects is stored in non-secure addresses. */
	zassert_true(!is_secure((intptr_t)&output),
		     "Test object is not non-secure");

	/* Prove that secure test objects is stored in secure addresses. */
	zassert_true(is_secure((intptr_t)secure_ptr),
		     "Test object is not secure");

	/* Normal execution */
	err = spm_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, &output);
	zassert_equal(err, 0, "Got unexpected failure");

	/* Verify that the function fails if it is passed secure pointers */
	err = spm_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, secure_ptr);
	zassert_true(err != 0, "Did not fail for secure pointer");
}

void test_spm_request_random_number(void)
{
	int err;
	size_t olen;
	uint8_t output[144];
	void *secure_ptr =
		(size_t *)(PM_SPM_SRAM_ADDRESS + PM_SPM_SRAM_SIZE - 1024);

	/* Prove that test objects is stored in non-secure addresses. */
	zassert_true(!is_secure((intptr_t)&olen),
		     "Test object is not non-secure");

	/* Prove that test objects is stored in non-secure addresses. */
	zassert_true(!is_secure((intptr_t)output),
		     "Test object is not non-secure");

	/* Prove that secure test objects is stored in secure addresses. */
	zassert_true(is_secure((intptr_t)secure_ptr),
		     "Test object is not secure");

	/* Normal execution */
	err = spm_request_random_number(output, sizeof(output), &olen);
	zassert_equal(err, 0, "Got unexpected failure");

	/* Verify that the function fails if it is passed secure pointers */
	err = spm_request_random_number(secure_ptr, sizeof(output), &olen);
	zassert_true(err != 0, "Did not fail for secure pointer");

	err = spm_request_random_number(output, sizeof(output), secure_ptr);
	zassert_true(err != 0, "Got unexpected failure");
}

void test_spm_request_read(void)
{
	const uint32_t valid_read_addr = (NRF_FICR_S_BASE + 0x204);
	int err;
	uint8_t output[32];
	void *secure_ptr = (size_t *)PM_SPM_SRAM_ADDRESS;

	/* Prove that test objects is stored in non-secure addresses. */
	zassert_true(!is_secure((intptr_t)output),
		     "Test object is not non-secure");

	/* Prove that test objects is stored in non-secure addresses. */
	zassert_true(!is_secure((intptr_t)non_secure_buf),
		     "Test object is not non-secure");

	/* Prove that secure test objects is stored in secure addresses. */
	zassert_true(is_secure((intptr_t)secure_ptr),
		     "Test object is not secure");

	/* Normal execution */
	err = spm_request_read(output, valid_read_addr, sizeof(output));
	zassert_equal(err, 0, "Got unexpected failure");

	/* Verify that the function fails if it is passed secure pointers */
	err = spm_request_read(secure_ptr, valid_read_addr, sizeof(output));
	zassert_true(err != 0, "Did not fail for secure pointer");

	/* Verify that the function fails if it is passed non-secure address */
	err = spm_request_read(output, (intptr_t)&non_secure_buf,
			       sizeof(output));
	zassert_true(err != 0, "Did not fail for non secure read addr");
}

void test_main(void)
{
	ztest_test_suite(test_secure_service,
			 ztest_unit_test(test_spm_firmware_info),
			 ztest_unit_test(test_spm_request_random_number),
			 ztest_unit_test(test_spm_s0_active),
			 ztest_unit_test(test_spm_request_read));
	ztest_run_test_suite(test_secure_service);
}

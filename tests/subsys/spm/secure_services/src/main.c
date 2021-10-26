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

	/* Verify that normal execution with the firmware info in
	 * secure flash and output info in non-secure flash.
	 */
	err = spm_firmware_info(PM_SPM_ADDRESS, &info);
	zassert_equal(err, 0, "Got unexpected failure");

	/* Verify that the function fails if the address of the firmware info
	 * is not in secure firmware.
	 */
	err = spm_firmware_info((uint32_t)&ns_fw_info, &info);
	zassert_equal(err, -EINVAL, "Did not fail for fw_address in secure fw");

	/* Verify that the function fails if the output argument for the info
	 * is in secure firmware.
	 */
	err = spm_firmware_info(PM_SPM_ADDRESS,
				(struct fw_info *)PM_SPM_SRAM_ADDRESS);
	zassert_equal(err, -EINVAL, "Did not fail when passing invalid output ptr");
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

	/* Verify that the function fails if it is passed a secure pointer */
	err = spm_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, secure_ptr);
	zassert_equal(err, -EINVAL, "Did not fail for secure pointer");
}

#ifdef CONFIG_SPM_SERVICE_RNG
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
	zassert_equal(err, -EINVAL, "Did not fail for secure pointer");

	err = spm_request_random_number(output, sizeof(output), secure_ptr);
	zassert_equal(err, -EINVAL, "Got unexpected failure");
}
#else
void test_spm_request_random_number(void)
{
	ztest_test_skip();
}
#endif

void test_spm_request_read(void)
{
	const uint32_t ficr_info_start = NRF_FICR_S_BASE + offsetof(NRF_FICR_Type, INFO);
	const uint32_t ficr_info_end   = ficr_info_start + sizeof(FICR_INFO_Type);

	int err;
	uint8_t output[4];
	uint8_t data_length = sizeof(output);
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

	/* Verify that the function fails if it is passed secure pointers */
	err = spm_request_read(secure_ptr, ficr_info_start, data_length);
	zassert_equal(err, -EINVAL, "Did not fail for secure pointer");

	/* Verify that edge addresses in FICR INFO will return expected values */
	/* Normal execution */
	err = spm_request_read(output, ficr_info_start, data_length);
	zassert_equal(err, 0, "Valid address returned an error!");

	err = spm_request_read(output, ficr_info_end - data_length, data_length);
	zassert_equal(err, 0, "Valid address returned an error!");

	/* Expect invalid addresses to return "operation not permitted" */
	err = spm_request_read(output, ficr_info_start - 1, data_length);
	zassert_equal(err, -EPERM, "Invalid address did not return -EPERM!");

	err = spm_request_read(output, ficr_info_end - data_length + 1, data_length);
	zassert_equal(err, -EPERM, "Invalid address did not return -EPERM!");

#if defined(FICR_NFC_TAGHEADER0_MFGID_Msk)
	const uint32_t ficr_nfc_start  = NRF_FICR_S_BASE + offsetof(NRF_FICR_Type, NFC);
	const uint32_t ficr_nfc_end    = ficr_nfc_start + sizeof(FICR_NFC_Type);

	/* Verify that edge addresses in FICR NFC will return expected values */
	/* Normal execution */
	err = spm_request_read(output, ficr_nfc_start, data_length);
	zassert_equal(err, 0, "Valid address returned an error!");

	err = spm_request_read(output, ficr_nfc_end - data_length, data_length);
	zassert_equal(err, 0, "Valid address returned an error!");

	/* Expect invalid addresses to return "operation not permitted" */
	err = spm_request_read(output, ficr_nfc_start - 1, data_length);
	zassert_equal(err, -EPERM, "Invalid address did not return -EPERM!");

	err = spm_request_read(output, ficr_nfc_end - data_length + 1, data_length);
	zassert_equal(err, -EPERM, "Invalid address did not return -EPERM!");
#endif /* defined(FICR_NFC_TAGHEADER0_MFGID_Msk) */
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

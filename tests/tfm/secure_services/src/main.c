/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 #include <zephyr/ztest.h>
 #include <tfm_ns_interface.h>
 #include <tfm_ioctl_api.h>
 #include <pm_config.h>

 #include <hal/nrf_gpio.h>

ZTEST_SUITE(test_secure_service, NULL, NULL, NULL, NULL, NULL);

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

ZTEST(test_secure_service, test_tfm_read_service)
{
	const uint32_t ficr_info_start = NRF_FICR_S_BASE + offsetof(NRF_FICR_Type, INFO);
	const uint32_t ficr_info_end   = ficr_info_start + sizeof(FICR_INFO_Type);

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
	plt_err = tfm_platform_mem_read(secure_ptr, ficr_info_start, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_INVALID_PARAM,
						"Did not fail for secure pointer");
	zassert_equal(err, -1, "Did not fail for secure pointer");

	/* Verify that edge addresses in FICR INFO will return expected values */
	/* Normal execution */
	plt_err = tfm_platform_mem_read(output, ficr_info_start, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS, "Valid address returned an error!");
	zassert_equal(err, 0, "Valid address returned an error!");

	plt_err = tfm_platform_mem_read(output, ficr_info_end - data_length, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS, "Valid address returned an error!");
	zassert_equal(err, 0, "Valid address returned an error!");

	/* Expect invalid addresses to finish with result -1 */
	plt_err = tfm_platform_mem_read(output, ficr_info_start - 1, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_INVALID_PARAM,
							"Invalid address returned an unexpected error");
	zassert_equal(err, -1, "Invalid address did not return expected address");

	plt_err = tfm_platform_mem_read(output, ficr_info_end - data_length + 1, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_INVALID_PARAM,
							"Invalid address returned an unexpected error");
	zassert_equal(err, -1, "Invalid address did not return expected address");

#if defined(FICR_NFC_TAGHEADER0_MFGID_Msk)
	const uint32_t ficr_nfc_start  = NRF_FICR_S_BASE + offsetof(NRF_FICR_Type, NFC);
	const uint32_t ficr_nfc_end    = ficr_nfc_start + sizeof(FICR_NFC_Type);

	/* Verify that edge addresses in FICR NFC will return expected values */
	/* Normal execution */
	plt_err = tfm_platform_mem_read(output, ficr_nfc_start, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS, "Valid address returned an error!");
	zassert_equal(err, 0, "Valid address returned an error!");

	plt_err = tfm_platform_mem_read(output, ficr_nfc_end - data_length, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS, "Valid address returned an error!");
	zassert_equal(err, 0, "Valid address returned an error!");

	/* Expect invalid addresses to finish with result -1 */
	plt_err = tfm_platform_mem_read(output, ficr_nfc_start - 1, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_INVALID_PARAM,
							"Invalid address returned an unexpected error");
	zassert_equal(err, -1, "Invalid address did not return expected address");

	plt_err = tfm_platform_mem_read(output, ficr_nfc_end - data_length + 1, data_length, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_INVALID_PARAM,
							"Invalid address returned an unexpected error");
	zassert_equal(err, -1, "Invalid address did not return expected address");
#endif /* defined(FICR_NFC_TAGHEADER0_MFGID_Msk) */
}

ZTEST(test_secure_service, test_tfm_gpio_service)
{
	uint32_t err = UINT32_MAX;
	enum tfm_platform_err_t plt_err;

#if NRF_GPIO_HAS_SEL
	plt_err = tfm_platform_gpio_pin_mcu_select(0, NRF_GPIO_PIN_SEL_PERIPHERAL, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS,
		      "Valid GPIO mcu select operation returned an unexpected error");
	zassert_equal(err, 0, "Valid GPIO select arguments returned an unexpected result");

	plt_err = tfm_platform_gpio_pin_mcu_select(0, 2, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS,
		      "Valid GPIO mcu select operation returned an unexpected error");
	zassert_equal(err, -1,
		      "Invalid GPIO select mcu argument returned an unexpected result");

	plt_err = tfm_platform_gpio_pin_mcu_select(66, NRF_GPIO_PIN_SEL_NETWORK, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_SUCCESS,
		      "Valid GPIO mcu select operation returned an unexpected error");
	zassert_equal(err, -1,
		      "Invalid GPIO select pin_number argument returned an unexpected result");
#else
	plt_err = tfm_platform_gpio_pin_mcu_select(0, 0x0 /* APPLICATION */, &err);
	zassert_equal(plt_err, TFM_PLATFORM_ERR_NOT_SUPPORTED,
		      "Not supported GPIO mcu select operation returned an unexpected error");
#endif
}

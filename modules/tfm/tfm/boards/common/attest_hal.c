/*
 * Copyright (c) 2018-2021, Arm Limited. All rights reserved.
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#include <stddef.h>
#include <stdint.h>
#include "tfm_attest_hal.h"
#include "tfm_plat_boot_seed.h"
#include "tfm_plat_device_id.h"
#include "tfm_plat_otp.h"
#include <nrf_cc3xx_platform.h>
#include "tfm_strnlen.h"

static enum tfm_security_lifecycle_t map_otp_lcs_to_tfm_slc(enum plat_otp_lcs_t lcs)
{
	switch (lcs) {
	case PLAT_OTP_LCS_ASSEMBLY_AND_TEST:
		return TFM_SLC_ASSEMBLY_AND_TEST;
	case PLAT_OTP_LCS_PSA_ROT_PROVISIONING:
		return TFM_SLC_PSA_ROT_PROVISIONING;
	case PLAT_OTP_LCS_SECURED:
		return TFM_SLC_SECURED;
	case PLAT_OTP_LCS_DECOMMISSIONED:
		return TFM_SLC_DECOMMISSIONED;
	case PLAT_OTP_LCS_UNKNOWN:
	default:
		return TFM_SLC_UNKNOWN;
	}
}

enum tfm_security_lifecycle_t tfm_attest_hal_get_security_lifecycle(void)
{
	enum plat_otp_lcs_t otp_lcs;
	enum tfm_plat_err_t err;

	err = tfm_plat_otp_read(PLAT_OTP_ID_LCS, sizeof(otp_lcs), (uint8_t *)&otp_lcs);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return TFM_SLC_UNKNOWN;
	}

	return map_otp_lcs_to_tfm_slc(otp_lcs);
}

enum tfm_plat_err_t tfm_attest_hal_get_verification_service(uint32_t *size, uint8_t *buf)
{
	enum tfm_plat_err_t err;
	size_t otp_size;

	err = tfm_plat_otp_read(PLAT_OTP_ID_VERIFICATION_SERVICE_URL, *size, buf);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	err = tfm_plat_otp_get_size(PLAT_OTP_ID_VERIFICATION_SERVICE_URL, &otp_size);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	*size = tfm_strnlen((char *)buf, otp_size);

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_attest_hal_get_profile_definition(uint32_t *size, uint8_t *buf)
{
	enum tfm_plat_err_t err;
	size_t otp_size;

	err = tfm_plat_otp_read(PLAT_OTP_ID_PROFILE_DEFINITION, *size, buf);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	err = tfm_plat_otp_get_size(PLAT_OTP_ID_PROFILE_DEFINITION, &otp_size);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	*size = tfm_strnlen((char *)buf, otp_size);

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_boot_seed(uint32_t size, uint8_t *buf)
{
	int nrf_err;
	if (size != NRF_CC3XX_PLATFORM_TFM_BOOT_SEED_SIZE) {
		return TFM_PLAT_ERR_INVALID_INPUT;
	}

	nrf_err = nrf_cc3xx_platform_get_boot_seed(buf);
	if (nrf_err != NRF_CC3XX_PLATFORM_SUCCESS) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_implementation_id(uint32_t *size, uint8_t *buf)
{
	enum tfm_plat_err_t err;
	size_t otp_size;

	err = tfm_plat_otp_read(PLAT_OTP_ID_IMPLEMENTATION_ID, *size, buf);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	err = tfm_plat_otp_get_size(PLAT_OTP_ID_IMPLEMENTATION_ID, &otp_size);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	*size = otp_size;

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_hw_version(uint32_t *size, uint8_t *buf)
{
	enum tfm_plat_err_t err;
	size_t otp_size;

	err = tfm_plat_otp_read(PLAT_OTP_ID_HW_VERSION, *size, buf);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	err = tfm_plat_otp_get_size(PLAT_OTP_ID_HW_VERSION, &otp_size);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	*size = tfm_strnlen((char *)buf, otp_size);

	return TFM_PLAT_ERR_SUCCESS;
}

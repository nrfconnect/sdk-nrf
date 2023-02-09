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
#include "nrf_provisioning.h"
#include <nrfx_nvmc.h>
#include <bl_storage.h>


static enum tfm_security_lifecycle_t map_bl_storage_lcs_to_tfm_slc(enum lcs lcs){
	switch (lcs) {
	case BL_STORAGE_LCS_ASSEMBLY:
		return TFM_SLC_ASSEMBLY_AND_TEST;
	case BL_STORAGE_LCS_PROVISIONING:
		return TFM_SLC_PSA_ROT_PROVISIONING;
	case BL_STORAGE_LCS_SECURED:
		return TFM_SLC_SECURED;
	case BL_STORAGE_LCS_DECOMMISSIONED:
		return TFM_SLC_DECOMMISSIONED;
	default:
		return TFM_SLC_UNKNOWN;
	}
}

static enum lcs map_tfm_slc_to_bl_storage_lcs(enum tfm_security_lifecycle_t lcs){
	switch (lcs) {
	case TFM_SLC_ASSEMBLY_AND_TEST:
		return BL_STORAGE_LCS_ASSEMBLY;
	case TFM_SLC_PSA_ROT_PROVISIONING:
		return BL_STORAGE_LCS_PROVISIONING;
	case TFM_SLC_SECURED:
		return BL_STORAGE_LCS_SECURED;
	case TFM_SLC_DECOMMISSIONED:
		return BL_STORAGE_LCS_DECOMMISSIONED;
	default:
		return BL_STORAGE_LCS_UNKNOWN;
	}
}

enum tfm_security_lifecycle_t tfm_attest_hal_get_security_lifecycle(void)
{
	int err;
	enum lcs otp_lcs;


	err = read_life_cycle_state(&otp_lcs);
	if (err != 0) {
		return TFM_SLC_UNKNOWN;
	}

	return map_bl_storage_lcs_to_tfm_slc(otp_lcs);
}

int tfm_attest_update_security_lifecycle_otp(enum tfm_security_lifecycle_t slc){
	enum lcs next_lcs;

	next_lcs = map_tfm_slc_to_bl_storage_lcs(slc);

	return update_life_cycle_state(next_lcs);
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
	*size = BL_STORAGE_IMPLEMENTATION_ID_SIZE;
	read_implementation_id_from_otp(buf);

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_cert_ref(uint32_t *size, uint8_t *buf)

{
	enum tfm_plat_err_t err;
	size_t otp_size;

	err = tfm_plat_otp_read(PLAT_OTP_ID_CERT_REF, *size, buf);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	err = tfm_plat_otp_get_size(PLAT_OTP_ID_CERT_REF, &otp_size);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	*size = tfm_strnlen((char *)buf, otp_size);

	return TFM_PLAT_ERR_SUCCESS;
}

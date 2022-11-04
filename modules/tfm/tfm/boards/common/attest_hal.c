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

/* implementation_id is a mandatory IAT claim */
#define CONFIG_SB_IMPLEMENTATION_ID 1

#define PM_PROVISION_ADDRESS (&NRF_UICR_S->OTP)

#include <bl_storage.h>

#include <nrfx_nvmc.h>

static enum tfm_security_lifecycle_t map_bl_storage_lcs_to_tfm_slc(enum lcs lcs){
	switch (lcs) {
	case ASSEMBLY:
		return TFM_SLC_ASSEMBLY_AND_TEST;
	case PROVISION:
		return TFM_SLC_PSA_ROT_PROVISIONING;
	case SECURE:
		return TFM_SLC_SECURED;
	case DECOMMISSIONED:
		return TFM_SLC_DECOMMISSIONED;
	case UNKNOWN:
	default:
		return TFM_SLC_UNKNOWN;
	}
}

static enum lcs map_tfm_slc_to_bl_storage_lcs(enum tfm_security_lifecycle_t lcs){
	switch (lcs) {
	case TFM_SLC_ASSEMBLY_AND_TEST:
		return ASSEMBLY;
	case TFM_SLC_PSA_ROT_PROVISIONING:
		return PROVISION;
	case TFM_SLC_SECURED:
		return SECURE;
	case TFM_SLC_DECOMMISSIONED:
		return DECOMMISSIONED;
	case TFM_SLC_UNKNOWN:
	default:
		return UNKNOWN;
	}
}

/* This is temporary solution until the bl_storage API is available in TF-M */

#define STATE_ENTERED 0x0000
#define STATE_NOT_ENTERED 0xFFFF
#define	EINVAL 22	/* Invalid argument */

/* Can later be replaced by a bl_storage call */
int read_lcs_from_otp(enum lcs *lcs)
{
	return read_life_cycle_state(lcs);
}

int update_lcs_in_otp(enum lcs next_lcs)
{
	return update_life_cycle_state(next_lcs);
}

static void read_implementation_id_from_otp(uint8_t * buf)
{
	otp_copy32(buf, (uint32_t *)&BL_STORAGE->implementation_id, 32);
}

/* End of  temporary solution */

enum tfm_security_lifecycle_t tfm_attest_hal_get_security_lifecycle(void)
{
	int err;
	enum lcs otp_lcs;


	err = read_lcs_from_otp(&otp_lcs);
	if (err != 0) {
		return TFM_SLC_UNKNOWN;
	}

	return map_bl_storage_lcs_to_tfm_slc(otp_lcs);
}

int tfm_attest_update_security_lifecycle_otp(enum tfm_security_lifecycle_t slc){
	enum lcs next_lcs;

	next_lcs = map_tfm_slc_to_bl_storage_lcs(slc);

	return update_lcs_in_otp(next_lcs);
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

	nrf_err = nrf_cc3xx_plaform_get_boot_seed(buf);
	if (nrf_err != NRF_CC3XX_PLATFORM_SUCCESS) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_implementation_id(uint32_t *size, uint8_t *buf)
{
	*size = 32;
	read_implementation_id_from_otp(buf);

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

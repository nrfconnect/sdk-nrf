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

/* 16 is the only supported length */
#define CONFIG_SB_PUBLIC_KEY_HASH_LEN 16

/* implementation_id is a mandatory IAT claim */
#define CONFIG_SB_IMPLEMENTATION_ID 1
#include "bl_storage.h"

#include <nrfx_nvmc.h>


static enum tfm_security_lifecycle_t map_bl_storage_lcs_to_tfm_slc(lcs_t lcs){
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

static lcs_t map_tfm_slc_to_bl_storage_lcs(enum tfm_security_lifecycle_t lcs){
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

/* This is temporary solution until the bl_storage API is available in TF-M
 * This points to the UICR OTP region
 */
static const struct bl_storage_data *p_bl_storage_data =
	(struct bl_storage_data *)PROVISION_PARTITION_ADDRESS;

#define STATE_LEFT 0x0000
#define STATE_NOT_LEFT 0xFFFF


/** Function for reading a half-word (2 byte value) from OTP.
 *
 * \details The flash in OTP supports only aligned 4 byte read operations.
 *          This function reads the encompassing 4 byte word, and returns the
 *          requested half-word.
 *
 * \param[in]  ptr  Address to read.
 *
 * \return value
 */
static uint16_t read_halfword(const uint16_t *ptr)
{
	bool top_half = ((uint32_t)ptr % 4); /* Addr not div by 4 */
	uint32_t target_addr = (uint32_t)ptr & ~3; /* Floor address */
	uint32_t val32 = *(uint32_t *)target_addr;
	__DSB(); /* Because of nRF9160 Erratum 7 */

	return (top_half ? (val32 >> 16) : val32) & 0x0000FFFF;
}

/** Function for writing a half-word (2 byte value) to OTP.
 *
 * \details The flash in OTP supports only aligned 4 byte write operations.
 *          This function writes to the encompassing 4 byte word, masking the
 *          other half-word with 0xFFFF so it is left untouched.
 *
 * \param[in]  ptr  Address to write to.
 * \param[in]  val  Value to write into @p ptr.
 */
static void write_halfword(const uint16_t *ptr, uint16_t val)
{
	bool top_half = (uint32_t)ptr % 4; /* Addr not div by 4 */
	uint32_t target_addr = (uint32_t)ptr & ~3; /* Floor address */

	uint32_t val32 = (uint32_t)val | 0xFFFF0000;
	uint32_t val32_shifted = ((uint32_t)val << 16) | 0x0000FFFF;

	nrfx_nvmc_word_write(target_addr, top_half ? val32_shifted : val32);
}


/* Can later be replaced by a bl_storage call */
int read_lcs_from_otp(lcs_t *lcs)
{
	uint16_t left_assembly     = read_halfword(&p_bl_storage_data->lcs.left_assembly);
	uint16_t left_provisioning = read_halfword(&p_bl_storage_data->lcs.left_provisioning);
	uint16_t left_secure       = read_halfword(&p_bl_storage_data->lcs.left_secure);

	if (left_assembly == STATE_NOT_LEFT) {
		*lcs = ASSEMBLY;
	} else if (left_assembly == STATE_LEFT && left_provisioning == STATE_NOT_LEFT) {
		*lcs = PROVISION;
	} else if (left_provisioning == STATE_LEFT && left_secure == STATE_NOT_LEFT) {
		*lcs = SECURE;
	} else if (left_provisioning == STATE_LEFT) {
		*lcs = DECOMMISSIONED;
	} else {
		/* To reach this the OTP must be corrupted or reading failed */
		return -EREADLCS;
	}

	return 0;
}

int update_lcs_in_otp(lcs_t next_lcs)
{
	int err;
	lcs_t current_lcs = 0;

	if(next_lcs == UNKNOWN){
		return -EINVALIDLCS;
	}

	err = read_lcs_from_otp(&current_lcs);
	if (err != 0) {
		return err;
	}

	if (next_lcs < current_lcs) {
		/* Is is only possible to transition into a higher state */
		return -EINVALIDLCS;
	}

	if (next_lcs == current_lcs) {
		/* The same LCS is a valid argument, but nothing to do so return success */
		return 0;
	}

	/* As the device starts in ASSEMBLY, it is not possible to write it */
	if (current_lcs == ASSEMBLY && next_lcs == PROVISION) {
		write_halfword(&(p_bl_storage_data->lcs.left_assembly), STATE_LEFT);
		return 0;
	}

	if (current_lcs == PROVISION && next_lcs == SECURE) {
		write_halfword(&(p_bl_storage_data->lcs.left_provisioning), STATE_LEFT);
		return 0;
	}

	if (current_lcs == SECURE && next_lcs == DECOMMISSIONED) {
		write_halfword(&(p_bl_storage_data->lcs.left_secure), STATE_LEFT);
		return 0;
	}

	/* This will be the case if any invalid transition is tried */
	return -EINVALIDLCS;
}

static void read_implementation_id_from_otp(uint8_t * buf)
{
	otp_copy32(buf, (uint32_t *)&p_bl_storage_data->implementation_id, 64);
}

/* End of  temporary solution */

enum tfm_security_lifecycle_t tfm_attest_hal_get_security_lifecycle(void)
{
	int err;
	lcs_t otp_lcs;


	err = read_lcs_from_otp(&otp_lcs);
	if (err != 0) {
		return TFM_SLC_UNKNOWN;
	}

	return map_bl_storage_lcs_to_tfm_slc(otp_lcs);
}

int tfm_attest_update_security_lifecycle_otp(enum tfm_security_lifecycle_t slc){
	lcs_t next_lcs;

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
	*size = 64;
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

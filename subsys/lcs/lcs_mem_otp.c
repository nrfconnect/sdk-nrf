/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <lcs_mem.h>
#if defined(CONFIG_NRFX_NVMC)
#include <nrfx_nvmc.h>
#elif defined(CONFIG_NRFX_RRAMC)
#include <nrfx_rramc.h>
#else
#error "No NRFX storage technology supported backend selected"
#endif

#if defined(CONFIG_NRFX_NVMC)
typedef uint16_t lcs_data_t;
typedef uint16_t lcs_reserved_t;
#elif defined(CONFIG_NRFX_RRAMC)
/* nRF54L15 only supports word writes */
typedef uint32_t lcs_data_t;
typedef uint32_t lcs_reserved_t;
#endif

/** Storage for the PRoT Security Lifecycle state, that consists of 4 states:
 *  - Device assembly and test
 *  - PRoT Provisioning
 *  - Secured
 *  - Decommissioned
 *  These states are transitioned top down during the life time of a device.
 *  The Device assembly and test state is implicitly defined by checking if
 *  the provisioning state wasn't entered yet.
 *  This works as ASSEMBLY implies the OTP to be erased.
 */
struct life_cycle_state_data {
	lcs_data_t provisioning;
	lcs_data_t secured;
	/* Pad to end the alignment at a 4-byte boundary as some devices
	 * are only supporting 4-byte UICR->OTP reads. We place the reserved
	 * padding in the middle of the struct in case we ever need to support
	 * another state.
	 */
	lcs_reserved_t reserved_for_padding;
	lcs_data_t decommissioned;
};

#define LCS_OTP_AREA ((const volatile struct life_cycle_state_data *)(0xffd500))


#if defined(CONFIG_NRFX_RRAMC)
static uint32_t index_from_address(uint32_t address)
{
	return ((address - (uint32_t)LCS_OTP_AREA)/sizeof(uint32_t));
}
#endif

static uint16_t bl_storage_lcs_get(uint32_t address)
{
#if defined(CONFIG_NRFX_NVMC)
	return (uint16_t)nrfx_nvmc_otp_halfword_read(address);
#elif defined(CONFIG_NRFX_RRAMC)
	return (uint16_t)nrfx_rramc_otp_word_read(index_from_address(address));
#endif
}

static int bl_storage_lcs_set(uint32_t address, uint16_t value)
{
#if defined(CONFIG_NRFX_NVMC)
	nrfx_nvmc_halfword_write(address, (lcs_data_t)value);
#elif defined(CONFIG_NRFX_RRAMC)
	if (!nrfx_rramc_otp_word_write(index_from_address(address), (lcs_data_t)value)) {
		return -EACCES;
	}
#endif
	return 0;
}

int lcs_mem_provisioning_read(uint16_t *value)
{
	uint16_t provisioning = bl_storage_lcs_get((uint32_t)&LCS_OTP_AREA->provisioning);

	if (value == NULL) {
		return -EINVAL;
	}

	*value = (uint16_t)provisioning;

	return 0;
}

int lcs_mem_provisioning_write(uint16_t value)
{
	return bl_storage_lcs_set((uint32_t)&LCS_OTP_AREA->provisioning, value);
}

int lcs_mem_secured_read(uint16_t *value)
{
	uint16_t secured = bl_storage_lcs_get((uint32_t) &LCS_OTP_AREA->secured);

	if (value == NULL) {
		return -EINVAL;
	}

	*value = (uint16_t)secured;

	return 0;
}

int lcs_mem_secured_write(uint16_t value)
{
	return bl_storage_lcs_set((uint32_t)&LCS_OTP_AREA->secured, value);
}

int lcs_mem_decommissioned_read(uint16_t *value)
{
	uint16_t decommissioned = bl_storage_lcs_get((uint32_t)&LCS_OTP_AREA->decommissioned);

	if (value == NULL) {
		return -EINVAL;
	}

	*value = (uint16_t)decommissioned;

	return 0;
}

int lcs_mem_decommissioned_write(uint16_t value)
{
	return bl_storage_lcs_set((uint32_t)&LCS_OTP_AREA->decommissioned, value);
}

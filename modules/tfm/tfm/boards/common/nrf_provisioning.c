/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "tfm_plat_provisioning.h"
#include "tfm_plat_otp.h"
#include "tfm_hal_platform.h"
#include "tfm_attest_hal.h"
#include "hw_unique_key.h"
#include "nrfx_nvmc.h"
#include <nrfx.h>
#include <nrf_cc3xx_platform.h>
#include <nrf_cc3xx_platform_identity_key.h>
#include "nrf_provisioning.h"
#include <identity_key.h>
#include <tfm_spm_log.h>


static enum tfm_plat_err_t disable_debugging(void)
{
    /* Configure the UICR such that upon the next reset, APPROTECT will be enabled */
    bool approt_writable = nrfx_nvmc_word_writable_check((uint32_t)&NRF_UICR_S->APPROTECT, UICR_APPROTECT_PALL_Protected);
    approt_writable &= nrfx_nvmc_word_writable_check((uint32_t)&NRF_UICR_S->SECUREAPPROTECT, UICR_SECUREAPPROTECT_PALL_Protected);

    if (approt_writable) {
        nrfx_nvmc_word_write((uint32_t)&NRF_UICR_S->APPROTECT, UICR_APPROTECT_PALL_Protected);
        nrfx_nvmc_word_write((uint32_t)&NRF_UICR_S->SECUREAPPROTECT, UICR_SECUREAPPROTECT_PALL_Protected);
    } else {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return TFM_PLAT_ERR_SUCCESS;
}

int tfm_plat_provisioning_is_required(void)
{
    enum tfm_security_lifecycle_t lcs = tfm_attest_hal_get_security_lifecycle();

    return lcs == TFM_SLC_PSA_ROT_PROVISIONING;
}

enum tfm_plat_err_t tfm_plat_provisioning_perform(void)
{
    enum tfm_security_lifecycle_t lcs = tfm_attest_hal_get_security_lifecycle();

    /*
     * Provisioning in NRF defines has two steps, the first step is to execute
     * the provisioning_image sample. When this sample is executed it will
     * always set the lifecycle state to PROVISIONING. This is a requirement for
     * the TF-M provisioning to be completed so we don't accept any other
     * lifecycle state here.
     */

    /* The Hardware Unique Keys should be already written */
    if (!hw_unique_key_are_any_written()) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* The Initial Attestation key should be already written */
    if (!nrf_cc3xx_platform_identity_key_is_stored(NRF_KMU_SLOT_KIDENT)) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /*
     * We don't need to make sure that the validation key is written here since we assume
     * that secure boot is already enabled at this stage
     */

    /* Disable debugging in UICR */
    if (disable_debugging() != TFM_PLAT_ERR_SUCCESS) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }


    /* Transition to the SECURED lifecycle state */
    if (tfm_attest_update_security_lifecycle_otp(TFM_SLC_SECURED) != 0) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    lcs = tfm_attest_hal_get_security_lifecycle();
    if (lcs != TFM_SLC_SECURED) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Perform a mandatory reset since we switch to an attestable LCS state */
    tfm_hal_system_reset();

    /*
     * We should never return from this function, a reset should be triggered
     * before we reach this point. Returning an error to signal that something
     * is wrong if we reached here.
     */
    return TFM_PLAT_ERR_SYSTEM_ERR;
}

static bool dummy_key_is_present(void)
{
	uint8_t key[IDENTITY_KEY_SIZE_BYTES];

	int err = nrf_cc3xx_platform_identity_key_retrieve(NRF_KMU_SLOT_KIDENT, key);

	if (err < 0) {
		/* Unable to read out the key. Then it is likely not present. */
		return false;
	}

	/* The first 8 bytes of the dummy key */
	uint8_t first_8_bytes[8] = {0xA9, 0xB4, 0x54, 0xB2, 0x6D, 0x6F, 0x90, 0xA4};

	/* Check if any bytes differ */
	for(int i = 0; i < 8; i++) {
		if(key[i] != first_8_bytes[i]) {
			return false;
		}
	}

	/* The first 8 bytes matched the dummy key, so it is most likely the dummy key */
	return true;
}


void tfm_plat_provisioning_check_for_dummy_keys(void)
{
	if (dummy_key_is_present()) {
		SPMLOG_ERRMSG("This device was provisioned with dummy keys and is NOT secure.");
	}
}

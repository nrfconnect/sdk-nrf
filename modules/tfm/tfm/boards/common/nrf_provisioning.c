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
#include <nrf_cc3xx_platform.h>
#include <nrf_cc3xx_platform_identity_key.h>

int tfm_plat_provisioning_is_required(void)
{
    enum tfm_security_lifecycle_t lcs = tfm_attest_hal_get_security_lifecycle();

    return lcs != TFM_SLC_SECURED;
}

enum tfm_plat_err_t tfm_plat_provisioning_perform(void)
{
    enum tfm_security_lifecycle_t lcs = tfm_attest_hal_get_security_lifecycle();

    /*
     * Provisioning in NRF defines has two steps, the first step is to execute
     * the identity_key_generation sample. When this sample is executed it will
     * always set the lifecycle state to PROVISIONING. This is a requirement for
     * the TF-M provisioning to be completed so we don't accept any other
     * lifecycle state here.
     */
    if(lcs != TFM_SLC_PSA_ROT_PROVISIONING) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* The Hardware Unique Keys should be already written */
    if(!hw_unique_key_are_any_written()) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* The Initial Attestation key should be already written */
    if(!nrf_cc3xx_platform_identity_key_is_stored(NRF_KMU_SLOT_KIDENT)) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /*
     * We don't need to make sure that the validation key is written here since we expect
     * that secure boot is already enabled at this stage
     */

    /* Disable debugging in the UICR register */
    bool approt_writable = nrfx_nvmc_word_writable_check((uint32_t)&NRF_UICR->APPROTECT, UICR_APPROTECT_PALL_Protected);
    approt_writable &= nrfx_nvmc_word_writable_check((uint32_t)&NRF_UICR->SECUREAPPROTECT, UICR_SECUREAPPROTECT_PALL_Protected);

    if(approt_writable){
        nrfx_nvmc_word_write((uint32_t)&NRF_UICR->APPROTECT, UICR_APPROTECT_PALL_Protected);
        nrfx_nvmc_word_write((uint32_t)&NRF_UICR->SECUREAPPROTECT, UICR_SECUREAPPROTECT_PALL_Protected);
    } else {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Transition to the SECURED lifecycle state */
    if (tfm_attest_update_security_lifecycle_otp(TFM_SLC_SECURED) != 0){
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    lcs = tfm_attest_hal_get_security_lifecycle();
    if (lcs != TFM_SLC_SECURED){
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Perform a mandatory reset since we switch to an attestable LCS state */
    NVIC_SystemReset();

    /*
     * We should never return from this function, a reset should be triggered
     * before we reach this point. Returning an error to signal that something
     * is wrong if we reached here.
     */
    return TFM_PLAT_ERR_SYSTEM_ERR;
}

void tfm_plat_provisioning_check_for_dummy_keys(void)
{
}

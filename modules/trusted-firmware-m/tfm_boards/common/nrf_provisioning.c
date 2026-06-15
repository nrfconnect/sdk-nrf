/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "tfm_plat_provisioning.h"
#include "tfm_plat_otp.h"
#include "tfm_platform_system.h"
#include "tfm_attest_hal.h"
#include "hw_unique_key.h"
#include <nrfx.h>
#include "nrf_provisioning.h"
#if !defined(CONFIG_HAS_HW_NRF_CRACEN)
#include <identity_key.h>
#endif
#include <tfm_log.h>
#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#include <pm_config.h>
#endif
#if defined(NRF53_SERIES) && (defined(PM_CPUNET_APP_ADDRESS) || defined(CONFIG_TFM_HAS_B0N))
#include <dfu/pcd_common.h>
#include <spu.h>
#include <hal/nrf_reset.h>

#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#if defined(PM_PCD_SRAM_ADDRESS)
volatile struct pcd_cmd *pcd_cmd_p = (struct pcd_cmd *)PM_PCD_SRAM_ADDRESS;
#elif defined(PM__PCD_SRAM_ADDRESS)
volatile struct pcd_cmd *pcd_cmd_p = (struct pcd_cmd *)PM__PCD_SRAM_ADDRESS;
#endif
#else
#include "flash_layout.h"
volatile struct pcd_cmd *pcd_cmd_p =
	(struct pcd_cmd *)TFM_DT_REG_ADDR(TFM_DT_NODELABEL(sram0_dfu_shared));
#endif

#define DEBUG_LOCK_TIMEOUT_MS 3000
#define USEC_IN_MSEC 1000
#define USEC_IN_SEC 1000000

static enum tfm_plat_err_t disable_netcore_debug(void)
{
	/* NRF_RESET to secure.
	 * It will be configured to the original value after the provisioning is done.
	 */
	spu_peripheral_config_secure(NRF_RESET_S_BASE, SPU_LOCK_CONF_UNLOCKED);

	/* Ensure that the network core is stopped. */
	nrf_reset_network_force_off(NRF_RESET, true);

	/* Debug lock command will be read in b0n startup. */
	pcd_write_cmd_lock_debug();

	/* Start the network core. */
	nrf_reset_network_force_off(NRF_RESET, false);

	/* Wait 1 second for the network core to start up. */
	NRFX_DELAY_US(USEC_IN_SEC);

	/* Wait for the debug lock to complete. */
	for (int i = 0; i < DEBUG_LOCK_TIMEOUT_MS; i++) {
		if (!pcd_read_cmd_lock_debug()) {
			break;
		}
		NRFX_DELAY_US(USEC_IN_MSEC);
	}

	if (!pcd_read_cmd_done()) {
		ERROR("Failed to lock debug in network core.");
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	return TFM_PLAT_ERR_SUCCESS;
}
#endif /* NRF53_SERIES && PM_CPUNET_APP_ADDRESS */

#if defined(CONFIG_SOC_SERIES_NRF54L) || defined(CONFIG_SOC_SERIES_NRF71)
/* On nRF54L/nRF71 the debug policy lives in TAMPC PROTECT, not UICR.APPROTECT.
 * verify_debug_disabled() only checks the current session; re-locking from reset
 * is done by nrf54l_handle_approtect() in SystemInit, which requires approtect
 * locking to be enabled at build time. Hence the guard below.
 */
#if !defined(ENABLE_APPROTECT) || !defined(ENABLE_SECUREAPPROTECT)
#error "TFM_NRF_PROVISIONING on nRF54L (CRACEN) requires CONFIG_NRF_APPROTECT_LOCK and " \
	"CONFIG_NRF_SECURE_APPROTECT_LOCK"
#endif

#define TAMPC_SIGNAL_CTRL_MASK(sig)                                                         \
	(TAMPC_PROTECT_##sig##_CTRL_LOCK_Msk | TAMPC_PROTECT_##sig##_CTRL_VALUE_Msk)

#define TAMPC_SIGNAL_CTRL_LOCKED_CLOSED(sig)                                                \
	((TAMPC_PROTECT_##sig##_CTRL_LOCK_Enabled << TAMPC_PROTECT_##sig##_CTRL_LOCK_Pos) |  \
	 (TAMPC_PROTECT_##sig##_CTRL_VALUE_Low << TAMPC_PROTECT_##sig##_CTRL_VALUE_Pos))

/* Reads *ctrl once (volatile) and reports whether the signal is locked closed. */
#define TAMPC_SIGNAL_IS_LOCKED_CLOSED(sig, ctrl)                                            \
	((*(ctrl) & TAMPC_SIGNAL_CTRL_MASK(sig)) == TAMPC_SIGNAL_CTRL_LOCKED_CLOSED(sig))

static enum tfm_plat_err_t verify_debug_disabled(void)
{
	/* Each TAMPC PROTECT signal latches once locked. Only advance to SECURED if
	 * every debug signal is locked in the closed (debug-disabled) state.
	 */
	if (!TAMPC_SIGNAL_IS_LOCKED_CLOSED(DOMAIN_DBGEN,
					   &NRF_TAMPC_S->PROTECT.DOMAIN[0].DBGEN.CTRL) ||
	    !TAMPC_SIGNAL_IS_LOCKED_CLOSED(DOMAIN_NIDEN,
					   &NRF_TAMPC_S->PROTECT.DOMAIN[0].NIDEN.CTRL) ||
	    !TAMPC_SIGNAL_IS_LOCKED_CLOSED(DOMAIN_SPIDEN,
					   &NRF_TAMPC_S->PROTECT.DOMAIN[0].SPIDEN.CTRL) ||
	    !TAMPC_SIGNAL_IS_LOCKED_CLOSED(DOMAIN_SPNIDEN,
					   &NRF_TAMPC_S->PROTECT.DOMAIN[0].SPNIDEN.CTRL) ||
	    !TAMPC_SIGNAL_IS_LOCKED_CLOSED(AP_DBGEN, &NRF_TAMPC_S->PROTECT.AP[0].DBGEN.CTRL)) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

#undef TAMPC_SIGNAL_IS_LOCKED_CLOSED
#undef TAMPC_SIGNAL_CTRL_LOCKED_CLOSED
#undef TAMPC_SIGNAL_CTRL_MASK
#else
static enum tfm_plat_err_t verify_debug_disabled(void)
{
	/* Ensures that APPROTECT and SECUREAPPROTECT are enabled upon the next reset */
	if (NRF_UICR_S->APPROTECT != UICR_APPROTECT_PALL_Protected ||
	    NRF_UICR_S->SECUREAPPROTECT != UICR_SECUREAPPROTECT_PALL_Protected) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	return TFM_PLAT_ERR_SUCCESS;
}
#endif

enum tfm_plat_err_t tfm_plat_provisioning_is_required(bool *provisioning_required)
{
	enum tfm_security_lifecycle_t lcs;

	lcs = tfm_attest_hal_get_security_lifecycle();

	if (provisioning_required == NULL) {
		return TFM_PLAT_ERR_INVALID_INPUT;
	}

	*provisioning_required = (lcs == TFM_SLC_PSA_ROT_PROVISIONING);
	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_provisioning_perform(void)
{
	enum tfm_security_lifecycle_t lcs;

	lcs = tfm_attest_hal_get_security_lifecycle();

	/*
	 * Provisioning in NRF defines has two steps, the first step is to execute
	 * the provisioning_image sample. When this sample is executed it will
	 * always set the lifecycle state to PROVISIONING. This is a requirement for
	 * the TF-M provisioning to be completed so we don't accept any other
	 * lifecycle state here.
	 */

	/* The Hardware Unique Keys should be already written */
	if (!hw_unique_key_are_any_written()) {
		ERROR("This device has not been provisioned with Hardware Unique Keys.");
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

#if defined(TFM_PARTITION_INITIAL_ATTESTATION) && !defined(CONFIG_HAS_HW_NRF_CRACEN)
	/* The Initial Attestation key should be already written. */
	if (!identity_key_is_written()) {
		ERROR(
			"This device has not been provisioned with an Initial Attestation Key.");
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
#endif

	/*
	 * We don't need to make sure that the validation key is written here since we assume
	 * that secure boot is already enabled at this stage
	 */

	/* Application debug should already be disabled */
	if (verify_debug_disabled() != TFM_PLAT_ERR_SUCCESS) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

#if defined(NRF53_SERIES) && (defined(PM_CPUNET_APP_ADDRESS) || defined(CONFIG_TFM_HAS_B0N))
	/* Disable network core debug in here */
	if (disable_netcore_debug() != TFM_PLAT_ERR_SUCCESS) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
#endif

	/* Transition to the SECURED lifecycle state */
	if (tfm_attest_update_security_lifecycle_otp(TFM_SLC_SECURED) != 0) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	lcs = tfm_attest_hal_get_security_lifecycle();
	if (lcs != TFM_SLC_SECURED) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	/* Perform a mandatory reset since we switch to an attestable LCS state */
	tfm_platform_hal_system_reset();

	/*
	 * We should never return from this function, a reset should be triggered
	 * before we reach this point. Returning an error to signal that something
	 * is wrong if we reached here.
	 */
	return TFM_PLAT_ERR_SYSTEM_ERR;
}

static bool dummy_key_is_present(void)
{
#if defined(TFM_PARTITION_INITIAL_ATTESTATION) && !defined(CONFIG_HAS_HW_NRF_CRACEN)
	uint8_t key[IDENTITY_KEY_SIZE_BYTES];
	int err;

	err = identity_key_read(key);
	if (err < 0) {
		/* Unable to read out the key. Then it is likely not present. */
		return false;
	}

	/* The first 8 bytes of the dummy key */
	uint8_t first_8_bytes[8] = {0xA9, 0xB4, 0x54, 0xB2, 0x6D, 0x6F, 0x90, 0xA4};

	/* Check if any bytes differ */
	for (int i = 0; i < 8; i++) {
		if (key[i] != first_8_bytes[i]) {
			return false;
		}
	}

	/* The first 8 bytes matched the dummy key, so it is most likely the dummy key */
	return true;
#else
	return false;
#endif
}

void tfm_plat_provisioning_check_for_dummy_keys(void)
{
	if (dummy_key_is_present()) {
		ERROR("This device was provisioned with dummy keys and is NOT secure.");
	}
}

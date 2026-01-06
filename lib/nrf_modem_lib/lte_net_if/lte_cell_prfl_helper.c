/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <modem/lte_lc.h>
#include <zephyr/logging/log.h>

#include "lte_cell_prfl_helper.h"

LOG_MODULE_REGISTER(cell_prfl_helper, CONFIG_NRF_MODEM_LIB_NET_IF_LOG_LEVEL);

/* @brief Configure cellular profiles.
 *
 *  @details Sets up two cellular profiles:
 *           - TN profile (id=0) for LTE-M/NB-IoT
 *           - NTN profile (id=1) for satellite connectivity
 *	     Note: modes +CFUN=45 and +CFUN=46 allowed only when two cellular profiles exists.
 *
 *  @retval 0 on success.
 *  @retval Negative errno on failure.
 */
int cell_prfl_set_profiles(void)
{
	int err;
	enum lte_lc_func_mode mode;

	err = lte_lc_func_mode_get(&mode);
	if (err) {
		LOG_ERR("Failed to get LTE function mode, error: %d", err);

		return err;
	}


	switch (mode) {
	case LTE_LC_FUNC_MODE_OFFLINE:
		__fallthrough;
	case LTE_LC_FUNC_MODE_POWER_OFF:
		break;
	case LTE_LC_FUNC_MODE_OFFLINE_KEEP_REG:
		LOG_DBG("Cellular profiles already set.");
		return 0;
	default:
		LOG_ERR("Setting cellular profiles is allowed only when modem is deactivated.");
		return -EFAULT;
	}

	LOG_INF("Configuring cellular profiles");

	/* Set TN profile for LTE-M/NB-IoT
	 * 0: Cellular profile index
	 * 1: Access technology: LE-UTRAN (WB-S1 mode), LTE-M/NB-IoT
	 * UICC: Physical SIM or SoftSIM based on configuration
	 */
	struct lte_lc_cellular_profile tn_profile = {
		.id = 0,
		.act = LTE_LC_ACT_LTEM | LTE_LC_ACT_NBIOT,
#if defined(CONFIG_NRF_MODEM_LIB_NET_IF_TN_SOFTSIM_SUPPORT)
		.uicc = LTE_LC_UICC_SOFTSIM,
#else
		.uicc = LTE_LC_UICC_PHYSICAL,
#endif
	};

	/* Set NTN SIM profile.
	 * 1: Cellular profile index
	 * 4: Access technology: Satellite E-UTRAN (NB-S1 mode)
	 * 0: SIM slot, physical SIM
	 */
	struct lte_lc_cellular_profile ntn_profile = {
		.id = 1,
		.act = LTE_LC_ACT_NTN,
		.uicc = LTE_LC_UICC_PHYSICAL,
	};

	/* Set TN profile */
	err = lte_lc_cellular_profile_configure(&tn_profile);
	if (err) {
		LOG_ERR("Failed to set TN profile, error: %d", err);

		return err;
	}

	/* Set NTN profile */
	err = lte_lc_cellular_profile_configure(&ntn_profile);
	if (err) {
		LOG_ERR("Failed to set NTN profile, error: %d", err);

		return err;
	}

	return 0;
}

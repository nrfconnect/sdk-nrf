/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

NRF_MODEM_LIB_ON_INIT(lte_lc_init_hook, on_modem_init, NULL);
NRF_MODEM_LIB_ON_SHUTDOWN(lte_lc_shutdown_hook, on_modem_shutdown, NULL);

NRF_MODEM_LIB_ON_CFUN(lte_lc_cfun_hook, on_modem_cfun, NULL);

static void on_modem_init(int err, void *ctx)
{
	extern const enum lte_lc_system_mode lte_lc_sys_mode;
	extern const enum lte_lc_system_mode_preference lte_lc_sys_mode_pref;

	if (err) {
		LOG_ERR("Modem library init error: %d, lte_lc not initialized", err);
		return;
	}

	if (!IS_ENABLED(CONFIG_LTE_NETWORK_MODE_DEFAULT)) {
		err = lte_lc_system_mode_set(lte_lc_sys_mode, lte_lc_sys_mode_pref);
		if (err) {
			LOG_ERR("Failed to set system mode and mode preference, err %d", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_LTE_PSM_REQ_FORMAT_SECONDS)) {
		err = lte_lc_psm_param_set_seconds(CONFIG_LTE_PSM_REQ_RPTAU_SECONDS,
						   CONFIG_LTE_PSM_REQ_RAT_SECONDS);
		if (err) {
			LOG_ERR("Failed to set PSM params, err %d", err);
			return;
		}

		LOG_DBG("PSM configs set from seconds: tau=%ds, rat=%ds",
			CONFIG_LTE_PSM_REQ_RPTAU_SECONDS,
			CONFIG_LTE_PSM_REQ_RAT_SECONDS);
	} else {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_LTE_PSM_REQ_FORMAT_STRING));

		err = lte_lc_psm_param_set(CONFIG_LTE_PSM_REQ_RPTAU, CONFIG_LTE_PSM_REQ_RAT);
		if (err) {
			LOG_ERR("Failed to set PSM params, err %d", err);
			return;
		}

		LOG_DBG("PSM configs set from string: tau=%s, rat=%s",
			CONFIG_LTE_PSM_REQ_RPTAU, CONFIG_LTE_PSM_REQ_RAT);
	}

	/* Request configured PSM and eDRX settings to save power. */
	err = lte_lc_psm_req(IS_ENABLED(CONFIG_LTE_PSM_REQ));
	if (err) {
		LOG_ERR("Failed to configure PSM, err %d", err);
		return;
	}

	if (IS_ENABLED(CONFIG_LTE_PROPRIETARY_PSM_REQ)) {
		err = lte_lc_proprietary_psm_req(true);
		if (err) {
			LOG_ERR("Failed to configure proprietary PSM, err %d", err);
			return;
		}
	} else {
		/* Return value is ignored because this feature is not supported by all MFW
		 * versions.
		 */
		(void)lte_lc_proprietary_psm_req(false);
	}

	err = lte_lc_edrx_req(IS_ENABLED(CONFIG_LTE_EDRX_REQ));
	if (err) {
		LOG_ERR("Failed to configure eDRX, err %d", err);
		return;
	}

#if defined(CONFIG_LTE_LOCK_BANDS)
	/* Set LTE band lock (volatile setting).
	 * Has to be done every time before activating the modem.
	 */
	err = nrf_modem_at_printf("AT%%XBANDLOCK=2,\""CONFIG_LTE_LOCK_BAND_MASK "\"");
	if (err) {
		LOG_ERR("Failed to lock LTE bands, err %d", err);
		return;
	}
#endif

#if defined(CONFIG_LTE_LOCK_PLMN)
	/* Manually select Operator (volatile setting).
	 * Has to be done every time before activating the modem.
	 */
	err = nrf_modem_at_printf("AT+COPS=1,2,\"" CONFIG_LTE_LOCK_PLMN_STRING "\"");
	if (err) {
		LOG_ERR("Failed to lock PLMN, err %d", err);
		return;
	}
#elif defined(CONFIG_LTE_UNLOCK_PLMN)
	/* Automatically select Operator (volatile setting).
	 */
	err = nrf_modem_at_printf("AT+COPS=0");
	if (err) {
		LOG_ERR("Failed to unlock PLMN, err %d", err);
		return;
	}
#endif

	/* Configure Release Assistance Indication (RAI). */
	err = nrf_modem_at_printf("AT%%RAI=%d", IS_ENABLED(CONFIG_LTE_RAI_REQ) ? 1 : 0);
	if (err) {
		LOG_ERR("Failed to configure RAI, err %d", err);
		return;
	}
}

static void on_modem_shutdown(void *ctx)
{
	/* Make sure the Modem library was in normal mode and not in bootloader mode. */
	if (nrf_modem_is_initialized()) {
		(void)lte_lc_power_off();
	}
}

static void on_modem_cfun(int mode, void *ctx)
{
	ARG_UNUSED(ctx);

	STRUCT_SECTION_FOREACH(lte_lc_cfun_cb, e) {
		LOG_DBG("CFUN monitor callback: %p", e->callback);
		e->callback(mode, e->context);
	}
}

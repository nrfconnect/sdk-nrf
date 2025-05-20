/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PSM_H__
#define PSM_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Set modem PSM parameters. */
int psm_param_set(const char *rptau, const char *rat);

/* Set modem PSM parameters. */
int psm_param_set_seconds(int rptau, int rat);

/* Request modem to enable or disable Power Saving Mode (PSM). */
int psm_req(bool enable);

/* Request modem to enable or disable proprietary Power Saving Mode (PSM). */
int psm_proprietary_req(bool enable);

/* Get the current PSM (Power Saving Mode) configuration. */
int psm_get(int *tau, int *active_time);

/* Send PSM configuration event update. */
void psm_evt_update_send(struct lte_lc_psm_cfg *psm_cfg);

/* Parse PSM parameters. */
int psm_parse(const char *active_time_str, const char *tau_ext_str,
	      const char *tau_legacy_str, struct lte_lc_psm_cfg *psm_cfg);

/* Get PSM work task. */
struct k_work *psm_work_get(void);

#ifdef __cplusplus
}
#endif

#endif /* PSM_H__ */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EDRX_H__
#define EDRX_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get the eDRX parameters currently provided by the network. */
int edrx_cfg_get(struct lte_lc_edrx_cfg *edrx_cfg);

/* Set the Paging Time Window (PTW) value to be used with eDRX. */
int edrx_ptw_set(enum lte_lc_lte_mode mode, const char *ptw);

/* Set the requested eDRX value. */
int edrx_param_set(enum lte_lc_lte_mode mode, const char *edrx);

/* Request modem to enable or disable use of eDRX. */
int edrx_request(bool enable);

/* Apply current eDRX configuration. Used after +CFUN=0 to restore the eDRX notification
 * subscription, which is not stored into NVM by the modem.
 */
void edrx_set(void);

#ifdef __cplusplus
}
#endif

#endif /* EDRX_H__ */

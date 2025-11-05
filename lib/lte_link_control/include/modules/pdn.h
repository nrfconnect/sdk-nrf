/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PDN_H__
#define PDN_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal PDN module functions */

int pdn_ctx_create(uint8_t *cid);
int pdn_ctx_configure(uint8_t cid, const char *apn, enum lte_lc_pdn_family family,
		      struct lte_lc_pdn_pdp_context_opts *opt);
int pdn_ctx_auth_set(uint8_t cid, enum lte_lc_pdn_auth method,
		     const char *user, const char *password);
int pdn_ctx_destroy(uint8_t cid);
int pdn_activate(uint8_t cid, int *esm, enum lte_lc_pdn_family *family);
int pdn_deactivate(uint8_t cid);
int pdn_id_get(uint8_t cid);
int pdn_dynamic_info_get(uint8_t cid, struct lte_lc_pdn_dynamic_info *pdn_info);
int pdn_ctx_default_apn_get(char *buf, size_t len);

#if defined(CONFIG_LTE_LC_PDN_ESM_STRERROR) || defined(__DOXYGEN__)
const char *pdn_esm_strerror(int reason);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PDN_H__ */

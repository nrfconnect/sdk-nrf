/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PALL_H__
#define PALL_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Store a PLMN access list with the specified list type (allowed or unallowed). */
int plmn_access_list_write(struct lte_lc_plmn_entry *plmn_list, size_t plmn_list_size,
			   enum lte_lc_plmn_list_type list_type);

/* Read the stored PLMN access list. */
int plmn_access_list_read(struct lte_lc_plmn_entry *plmn_list, size_t *plmn_list_size,
			   enum lte_lc_plmn_list_type *list_type);

/* Clear the stored PLMN access list. */
int plmn_access_list_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* PALL_H__ */

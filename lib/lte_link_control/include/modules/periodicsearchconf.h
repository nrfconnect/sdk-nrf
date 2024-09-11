/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PERIODICSEARCHCONF_H__
#define PERIODICSEARCHCONF_H__

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configure periodic searches. */
int periodicsearchconf_set(const struct lte_lc_periodic_search_cfg *const cfg);

/* Get the configured periodic search parameters. */
int periodicsearchconf_get(struct lte_lc_periodic_search_cfg *const cfg);

/* Clear the configured periodic search parameters. */
int periodicsearchconf_clear(void);

/* Request an extra search. */
int periodicsearchconf_request(void);

#ifdef __cplusplus
}
#endif

#endif /* PERIODICSEARCHCONF_H__ */

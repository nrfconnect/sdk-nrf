/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LINK_H
#define MOSH_LINK_H
#include <modem/lte_lc.h>

#define LINK_APN_STR_MAX_LENGTH 100

#define LINK_FUNMODE_NONE 99

void link_init(void);
void link_ind_handler(const struct lte_lc_evt *const evt);
void link_rsrp_subscribe(bool subscribe);
void link_ncellmeas_start(bool start);
void link_modem_sleep_notifications_subscribe(uint32_t warn_time_ms, uint32_t threshold_ms);
void link_modem_sleep_notifications_unsubscribe(void);
void link_modem_tau_notifications_subscribe(uint32_t warn_time_ms, uint32_t threshold_ms);
void link_modem_tau_notifications_unsubscribe(void);
int link_func_mode_set(enum lte_lc_func_mode fun);
int link_func_mode_get(void);

#endif /* MOSH_LINK_H */

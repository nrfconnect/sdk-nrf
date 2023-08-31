/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LINK_H
#define MOSH_LINK_H
#include <modem/lte_lc.h>

enum link_ncellmeas_modes {
	LINK_NCELLMEAS_MODE_NONE = 0,
	LINK_NCELLMEAS_MODE_SINGLE,
	LINK_NCELLMEAS_MODE_CONTINUOUS,
};

#define LINK_APN_STR_MAX_LENGTH 100

#define LINK_FUNMODE_NONE 99
#define LINK_SYSMODE_NONE 99
#define LINK_REDMOB_NONE 99
#define LTE_LC_FACTORY_RESET_INVALID 99

void link_init(void);
void link_ind_handler(const struct lte_lc_evt *const evt);
void link_rsrp_subscribe(bool subscribe);
void link_ncellmeas_start(bool start, enum link_ncellmeas_modes mode,
			  struct lte_lc_ncellmeas_params ncellmeas_params,
			  int periodic_interval,
			  bool periodic_interval_given);
void link_modem_sleep_notifications_subscribe(uint32_t warn_time_ms, uint32_t threshold_ms);
void link_modem_sleep_notifications_unsubscribe(void);
void link_modem_tau_notifications_subscribe(uint32_t warn_time_ms, uint32_t threshold_ms);
void link_modem_tau_notifications_unsubscribe(void);
int link_func_mode_set(enum lte_lc_func_mode fun, bool rel14_used);
int link_func_mode_get(void);
void link_rai_read(void);
int link_rai_enable(bool enable);
int link_setdnsaddr(const char *ip_address);

#endif /* MOSH_LINK_H */

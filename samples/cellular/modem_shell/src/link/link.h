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

/** Type of factory reset to perform. */
enum link_factory_reset_type {
	/** Reset all modem data to factory settings. */
	LINK_FACTORY_RESET_ALL = 0,

	/** Reset user-configurable data to factory settings. */
	LINK_FACTORY_RESET_USER = 1,

	/** Invalid type. */
	LINK_FACTORY_RESET_INVALID = 99
};

/** Reduced mobility mode. */
enum link_reduced_mobility_mode {
	/** Functionality according to the 3GPP relaxed monitoring feature. */
	LINK_REDUCED_MOBILITY_DEFAULT = 0,

	/** Enable Nordic-proprietary reduced mobility feature. */
	LINK_REDUCED_MOBILITY_NORDIC = 1,

	/**
	 * Full measurements for best possible mobility.
	 *
	 * Disable the 3GPP relaxed monitoring and Nordic-proprietary reduced mobility features.
	 */
	LINK_REDUCED_MOBILITY_DISABLED = 2,

	/** Invalid type. */
	LINK_REDUCED_MOBILITY_NONE = 99
};

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
int link_getifaddrs(void);
void link_propripsm_read(void);

#endif /* MOSH_LINK_H */

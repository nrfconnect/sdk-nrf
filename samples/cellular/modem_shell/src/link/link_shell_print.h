/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LINK_SHELL_PRINT_H
#define MOSH_LINK_SHELL_PRINT_H

#include <zephyr/shell/shell.h>
#include <modem/lte_lc.h>

struct mapping_tbl_item {
	int key;
	char *value_str;
};

void link_shell_print_reg_status(enum lte_lc_nw_reg_status reg_status);
void link_shell_print_modem_sleep_notif(const struct lte_lc_evt *const evt);
void link_shell_print_modem_domain_event(enum lte_lc_modem_evt modem_evt);
const char *link_shell_funmode_to_string(int funmode, char *out_str_buff);
const char *link_shell_sysmode_to_string(int sysmode, char *out_str_buff);
const char *link_shell_sysmode_preferred_to_string(int sysmode_preference, char *out_str_buff);
const char *link_shell_sysmode_currently_active_to_string(int actmode, char *out_str_buff);
const char *link_shell_map_to_string(struct mapping_tbl_item const *mapping_table,
				      int mode, char *out_str_buff);
const char *link_shell_redmob_mode_to_string(int funmode, char *out_str_buff);

#endif /* MOSH_LINK_SHELL_PRINT_H */

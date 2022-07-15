/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LINK_SETTINGS_H
#define MOSH_LINK_SETTINGS_H

int link_sett_init(void);

void link_sett_defaults_set(void);
void link_sett_all_print(void);

int link_sett_save_defcont_enabled(bool enabled);
bool link_sett_is_defcont_enabled(void);
void link_sett_defcont_conf_shell_print(void);
int link_sett_save_defcont_apn(const char *default_apn_str);
char *link_sett_defcont_apn_get(void);
int link_sett_save_defcont_pdn_family(enum pdn_fam family);
enum pdn_fam link_sett_defcont_pdn_family_get(void);

int link_sett_save_defcontauth_enabled(bool enabled);
bool link_sett_is_defcontauth_enabled(void);
void link_sett_defcontauth_conf_shell_print(void);
int link_sett_save_defcontauth_prot(int auth_prot);
enum pdn_auth link_sett_defcontauth_prot_get(void);
int link_sett_save_defcontauth_username(const char *username_str);
char *link_sett_defcontauth_username_get(void);
int link_sett_save_defcontauth_password(const char *password_str);
char *link_sett_defcontauth_password_get(void);

int link_sett_save_dnsaddr_enabled(bool enabled);
bool link_sett_is_dnsaddr_enabled(void);
int link_sett_save_dnsaddr_ip(const char *dnsaddr_ip_str);
const char *link_sett_dnsaddr_ip_get(void);
void link_sett_dnsaddr_conf_shell_print(void);

int link_sett_sysmode_save(enum lte_lc_system_mode mode,
			    enum lte_lc_system_mode_preference lte_pref);
int link_sett_sysmode_get(void);
int link_sett_sysmode_lte_preference_get(void);
int link_sett_sysmode_default_set(void);
void link_sett_sysmode_print(void);

#define LINK_SETT_NMODEAT_MEM_SLOT_INDEX_START 1
#define LINK_SETT_NMODEAT_MEM_SLOT_INDEX_END 3

char *link_sett_normal_mode_at_cmd_str_get(uint8_t mem_slot);
int link_sett_save_normal_mode_at_cmd_str(const char *at_str, uint8_t mem_slot);
int link_sett_clear_normal_mode_at_cmd_str(uint8_t mem_slot);
void link_sett_normal_mode_at_cmds_shell_print(void);

int link_sett_save_normal_mode_autoconn_enabled(bool enabled, bool use_rel14);
bool link_sett_is_normal_mode_autoconn_enabled(void);
bool link_sett_is_normal_mode_autoconn_rel14_used(void);
void link_sett_normal_mode_autoconn_shell_print(void);

void link_sett_modem_factory_reset(enum lte_lc_factory_reset_type type);

#endif /* MOSH_LINK_SETTINGS_H */

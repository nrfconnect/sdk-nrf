/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_LINK_SHELL_PDN_H
#define MOSH_LINK_SHELL_PDN_H

#include <zephyr/shell/shell.h>
#include <modem/pdn.h>

struct link_shell_pdn_auth {
	enum pdn_auth method;
	char *user;
	char *password;
};

void link_shell_pdn_init(void);
void link_shell_pdn_events_subscribe(void);

int link_shell_pdn_connect(const char *apn_name,
			   const char *family_str,
			   const struct link_shell_pdn_auth *auth_params);
int link_shell_pdn_disconnect(int pdn_cid);
int link_shell_pdn_activate(int pdn_cid);

int link_family_str_to_pdn_lib_family(enum pdn_fam *ret_fam, const char *family);
const char *link_pdn_lib_family_to_string(enum pdn_fam pdn_family, char *out_fam_str);

bool link_shell_pdn_info_is_in_list(uint8_t cid);
void link_shell_pdn_info_list_remove(uint8_t cid);

int link_shell_pdn_auth_prot_to_pdn_lib_method_map(int auth_proto, enum pdn_auth *method);

int link_shell_pdn_event_forward_cb_set(pdn_event_handler_t cb);

#endif /* MOSH_LINK_SHELL_PDN_H */

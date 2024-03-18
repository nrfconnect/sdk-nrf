/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/shell/shell.h>

#include <nrf_modem_at.h>
#include <modem/pdn.h>

#if defined(CONFIG_MOSH_PPP)
#include "ppp_ctrl.h"
#endif
#if defined(CONFIG_MOSH_STARTUP_CMDS)
#include "startup_cmd_ctrl.h"
#endif
#include "link_shell_print.h"
#include "link_shell_pdn.h"
#include "mosh_print.h"

static sys_dlist_t pdn_info_list;

/* There cannot be multiple PDN lib event handlers and we need these elsewhere in MoSh.
 * Thus, we need a forwarder for PDN events.
 */
static pdn_event_handler_t pdn_event_forward_callback;
struct link_shell_pdn_info {
	sys_dnode_t dnode;
	int pdn_id;
	uint8_t cid;
};

static const char *const event_str[] = {
	[PDN_EVENT_CNEC_ESM] = "ESM",
	[PDN_EVENT_ACTIVATED] = "activated",
	[PDN_EVENT_DEACTIVATED] = "deactivated",
	[PDN_EVENT_IPV6_UP] = "IPv6 up",
	[PDN_EVENT_IPV6_DOWN] = "IPv6 down",
	[PDN_EVENT_NETWORK_DETACH] = "network detach",
};

void link_pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		mosh_print("PDN event: PDP context %d, %s", cid, pdn_esm_strerror(reason));
		break;
	default:
		mosh_print("PDN event: PDP context %d %s", cid, event_str[event]);
		if (cid == 0) {
#if defined(CONFIG_MOSH_STARTUP_CMDS)
			if (event == PDN_EVENT_ACTIVATED) {
				startup_cmd_ctrl_default_pdn_active();
			}
#endif
#if defined(CONFIG_MOSH_PPP)
			/* Notify PPP side about the default PDN activation status */
			if (event == PDN_EVENT_ACTIVATED) {
				ppp_ctrl_default_pdn_active(true);
			} else if (event == PDN_EVENT_DEACTIVATED ||
				   event == PDN_EVENT_NETWORK_DETACH) {
				/* 3GPP 27.007 ch 10.1.19 for +CGEV: ME DETACH
				 * This implies that all active contexts have been deactivated.
				 * These are not reported separately.
				 */
				ppp_ctrl_default_pdn_active(false);
			}
#endif
		}
		break;
	}

	if (pdn_event_forward_callback) {
		pdn_event_forward_callback(cid, event, reason);
	}
}

int link_family_str_to_pdn_lib_family(enum pdn_fam *ret_fam,
				       const char *family)
{
	if (family != NULL) {
		if (strcmp(family, "ipv4v6") == 0) {
			*ret_fam = PDN_FAM_IPV4V6;
		} else if (strcmp(family, "ipv4") == 0) {
			*ret_fam = PDN_FAM_IPV4;
		} else if (strcmp(family, "ipv6") == 0) {
			*ret_fam = PDN_FAM_IPV6;
		} else if (strcmp(family, "packet") == 0 || strcmp(family, "non-ip") == 0) {
			*ret_fam = PDN_FAM_NONIP;
		} else {
			mosh_error(
				"%s: could not decode PDN address family (%s)",
				__func__,
				family);
			return -EINVAL;
		}
	} else {
		*ret_fam = PDN_FAM_IPV4V6;
	}

	return 0;
}

const char *link_pdn_lib_family_to_string(enum pdn_fam pdn_family,
					   char *out_fam_str)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ PDN_FAM_IPV4, "ipv4" },
		{ PDN_FAM_IPV6, "ipv6" },
		{ PDN_FAM_IPV4V6, "ipv4v6" },
		{ PDN_FAM_NONIP, "non-ip" },
		{ -1, NULL }
	};

	return link_shell_map_to_string(mapping_table, pdn_family,
					 out_fam_str);
}

static struct link_shell_pdn_info *link_shell_pdn_info_list_add(int pdn_id, uint8_t cid)
{
	struct link_shell_pdn_info *new_pdn_info = NULL;
	struct link_shell_pdn_info *iterator = NULL;

	/* Check if already in list, if there; then return an existing one */
	SYS_DLIST_FOR_EACH_CONTAINER(&pdn_info_list, iterator, dnode) {
		if (iterator->cid == cid) {
			return iterator;
		}
	}

	/* Not already at list, let's add it */
	new_pdn_info =
		(struct link_shell_pdn_info *)k_calloc(1, sizeof(struct link_shell_pdn_info));
	new_pdn_info->cid = cid;
	new_pdn_info->pdn_id = pdn_id;

	SYS_DLIST_FOR_EACH_CONTAINER(&pdn_info_list, iterator, dnode) {
		if (new_pdn_info->cid < iterator->cid) {
			sys_dlist_insert(&iterator->dnode, &new_pdn_info->dnode);
			return new_pdn_info;
		}
	}
	sys_dlist_append(&pdn_info_list, &new_pdn_info->dnode);
	return new_pdn_info;
}

static struct link_shell_pdn_info *link_shell_pdn_info_list_find(uint8_t cid)
{
	struct link_shell_pdn_info *iterator = NULL;

	SYS_DLIST_FOR_EACH_CONTAINER(&pdn_info_list, iterator, dnode) {
		if (iterator->cid == cid) {
			break;
		}
	}
	return iterator;
}

void link_shell_pdn_info_list_remove(uint8_t cid)
{
	struct link_shell_pdn_info *found;

	found = link_shell_pdn_info_list_find(cid);
	if (found != NULL) {
		sys_dlist_remove(&found->dnode);
		k_free(found);
	}
}

bool link_shell_pdn_info_is_in_list(uint8_t cid)
{
	struct link_shell_pdn_info *result = NULL;
	bool found = false;

	result = link_shell_pdn_info_list_find(cid);
	if (result != NULL) {
		found = true;
	}
	return found;
}

int link_shell_pdn_connect(const char *apn_name,
			   const char *family_str,
			   const struct link_shell_pdn_auth *auth_params)
{
	struct link_shell_pdn_info *new_pdn_info = NULL;
	enum pdn_fam pdn_lib_family;
	uint8_t cid = -1;
	int pdn_id;
	int ret;

	ret = pdn_ctx_create(&cid, link_pdn_event_handler);
	if (ret) {
		mosh_error("pdn_ctx_create() failed, err %d\n", ret);
		goto cleanup_and_fail;
	}

	ret = link_family_str_to_pdn_lib_family(&pdn_lib_family, family_str);
	if (ret) {
		mosh_error("link_family_str_to_pdn_lib_family() failed, err %d\n", ret);
		goto cleanup_and_fail;
	}

	/* Configure a PDP context with APN and family */
	ret = pdn_ctx_configure(cid, apn_name, pdn_lib_family, NULL);
	if (ret) {
		mosh_error("pdn_ctx_configure() failed, err %d\n", ret);
		goto cleanup_and_fail;
	}

	/* Set authentication params if requested */
	if (auth_params != NULL) {
		ret = pdn_ctx_auth_set(cid, auth_params->method,
				       auth_params->user,
				       auth_params->password);
		if (ret) {
			mosh_error("Failed to set auth params for CID %d, err %d", cid, ret);
			goto cleanup_and_fail;
		}
	}

	/* Activate a PDN connection */
	ret = link_shell_pdn_activate(cid);
	if (ret) {
		goto cleanup_and_fail;
	}

	pdn_id = pdn_id_get(cid);

	/* Add to PDN list */
	new_pdn_info = link_shell_pdn_info_list_add(pdn_id, cid);
	if (new_pdn_info == NULL) {
		mosh_error("ltelc_pdn_init_and_connect: could not add new PDN socket to list\n");
	}

	mosh_print("Created and activated a new PDP context: CID %d, PDN ID %d\n", cid, pdn_id);

	return 0;

cleanup_and_fail:
	if (cid != -1) {
		(void)pdn_ctx_destroy(cid);
	}

	return ret;
}

int link_shell_pdn_disconnect(int pdn_cid)
{
	int ret;

	ret = pdn_deactivate(pdn_cid);
	if (ret) {
		mosh_error("pdn_deactivate() failed, err %d", ret);
	}

	ret = pdn_ctx_destroy(pdn_cid);
	if (ret) {
		mosh_error("pdn_ctx_destroy() failed, err %d", ret);
		return ret;
	}

	link_shell_pdn_info_list_remove(pdn_cid);

	mosh_print("CID %d deactivated and context destroyed\n", pdn_cid);

	return 0;
}

int link_shell_pdn_activate(int pdn_cid)
{
	int ret;
	int esm;

	/* Activate a PDN connection */
	ret = pdn_activate(pdn_cid, &esm, NULL);
	if (ret) {
		mosh_error(
			"pdn_activate() failed, err %d esm %d %s\n",
			ret, esm, pdn_esm_strerror(esm));
	}
	return ret;
}

void link_shell_pdn_events_subscribe(void)
{
	pdn_default_ctx_cb_reg(link_pdn_event_handler);
}

int link_shell_pdn_event_forward_cb_set(pdn_event_handler_t cb)
{
	if (!cb) {
		return -EINVAL;
	}

	pdn_event_forward_callback = cb;

	return 0;
}

void link_shell_pdn_init(void)
{
	link_shell_pdn_events_subscribe();
	sys_dlist_init(&pdn_info_list);
}

int link_shell_pdn_auth_prot_to_pdn_lib_method_map(int auth_proto,
						   enum pdn_auth *method)
{
	*method = PDN_AUTH_NONE;

	if (auth_proto == 0) {
		*method = PDN_AUTH_NONE;
	} else if (auth_proto == 1) {
		*method = PDN_AUTH_PAP;
	} else if (auth_proto == 2) {
		*method = PDN_AUTH_CHAP;
	} else {
		return -EINVAL;
	}
	return 0;
}

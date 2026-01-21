/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/shell/shell.h>

#include <nrf_modem_at.h>
#include <modem/lte_lc.h>

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
static lte_lc_evt_handler_t lte_lc_event_forward_callback;
struct link_shell_pdn_info {
	sys_dnode_t dnode;
	int pdn_id;
	uint8_t cid;
};

const char *link_pdn_event_to_string(enum lte_lc_evt_type event, char *out_str_buf)
{
	struct mapping_tbl_item const mapping_table[] = {
		/* PDN_EVENT_CNEC_ESM is handled separately */
		{ LTE_LC_EVT_PDN_ACTIVATED, "activated" },
		{ LTE_LC_EVT_PDN_DEACTIVATED, "deactivated" },
		{ LTE_LC_EVT_PDN_IPV6_UP, "IPv6 up" },
		{ LTE_LC_EVT_PDN_IPV6_DOWN, "IPv6 down" },
		{ LTE_LC_EVT_PDN_SUSPENDED, "suspended" },
		{ LTE_LC_EVT_PDN_RESUMED, "resumed" },
		{ LTE_LC_EVT_PDN_NETWORK_DETACH, "network detach" },
		{ LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON, "APN rate control on" },
		{ LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF, "APN rate control off" },
		{ LTE_LC_EVT_PDN_CTX_DESTROYED, "context destroyed" },
		{ -1, NULL }
	};

	return link_shell_map_to_string(mapping_table, event, out_str_buf);
}

void link_pdn_event_handler(const struct lte_lc_evt *const evt)
{
	char str_buf[32];

	switch (evt->type) {
	case LTE_LC_EVT_PDN:
		switch (evt->pdn.type) {
		case LTE_LC_EVT_PDN_ESM_ERROR:
			mosh_print("PDN event: PDP context %d, %s", evt->pdn.cid,
			       lte_lc_pdn_esm_strerror(evt->pdn.esm_err));
			break;
		default:
			mosh_print("PDN event: PDP context %d %s", evt->pdn.cid,
				link_pdn_event_to_string(evt->pdn.type, str_buf));

			if (evt->pdn.cid == 0) {
#if defined(CONFIG_MOSH_STARTUP_CMDS)
				if (evt->pdn.type == LTE_LC_EVT_PDN_ACTIVATED) {
					startup_cmd_ctrl_default_pdn_active();
				}
#endif /* CONFIG_MOSH_STARTUP_CMDS */
#if defined(CONFIG_MOSH_PPP)
				/* Notify PPP side about the default PDN activation status */
				if (evt->pdn.type == LTE_LC_EVT_PDN_ACTIVATED) {
					ppp_ctrl_default_pdn_active(true);
				} else if (evt->pdn.type == LTE_LC_EVT_PDN_DEACTIVATED ||
					evt->pdn.type == LTE_LC_EVT_PDN_NETWORK_DETACH) {
					/* 3GPP 27.007 ch 10.1.19 for +CGEV: ME DETACH
					 * This implies that all active contexts have been
					 * deactivated. These are not reported separately.
					 */
					ppp_ctrl_default_pdn_active(false);
				}
#endif /* CONFIG_MOSH_PPP */
			}

			break;
		}
	default:
		break;
	}

	if (lte_lc_event_forward_callback) {
		lte_lc_event_forward_callback(evt);
	}
}

int link_family_str_to_pdn_lib_family(enum lte_lc_pdn_family *ret_fam,
				       const char *family)
{
	if (family != NULL) {
		if (strcmp(family, "ipv4v6") == 0) {
			*ret_fam = LTE_LC_PDN_FAM_IPV4V6;
		} else if (strcmp(family, "ipv4") == 0) {
			*ret_fam = LTE_LC_PDN_FAM_IPV4;
		} else if (strcmp(family, "ipv6") == 0) {
			*ret_fam = LTE_LC_PDN_FAM_IPV6;
		} else if (strcmp(family, "packet") == 0 || strcmp(family, "non-ip") == 0) {
			*ret_fam = LTE_LC_PDN_FAM_NONIP;
		} else {
			mosh_error(
				"%s: could not decode PDN address family (%s)",
				__func__,
				family);
			return -EINVAL;
		}
	} else {
		*ret_fam = LTE_LC_PDN_FAM_IPV4V6;
	}

	return 0;
}

const char *link_pdn_lib_family_to_string(enum lte_lc_pdn_family pdn_family,
					   char *out_fam_str)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ LTE_LC_PDN_FAM_IPV4, "ipv4" },
		{ LTE_LC_PDN_FAM_IPV6, "ipv6" },
		{ LTE_LC_PDN_FAM_IPV4V6, "ipv4v6" },
		{ LTE_LC_PDN_FAM_NONIP, "non-ip" },
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
	enum lte_lc_pdn_family pdn_lib_family;
	uint8_t cid = -1;
	int pdn_id;
	int ret;

	ret = lte_lc_pdn_ctx_create(&cid);
	if (ret) {
		mosh_error("lte_lc_pdn_ctx_create() failed, err %d\n", ret);
		goto cleanup_and_fail;
	}

	ret = link_family_str_to_pdn_lib_family(&pdn_lib_family, family_str);
	if (ret) {
		mosh_error("link_family_str_to_pdn_lib_family() failed, err %d\n", ret);
		goto cleanup_and_fail;
	}

	/* Configure a PDP context with APN and family */
	ret = lte_lc_pdn_ctx_configure(cid, apn_name, pdn_lib_family, NULL);
	if (ret) {
		mosh_error("lte_lc_pdn_ctx_configure() failed, err %d\n", ret);
		goto cleanup_and_fail;
	}

	/* Set authentication params if requested */
	if (auth_params != NULL) {
		ret = lte_lc_pdn_ctx_auth_set(cid, auth_params->method,
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

	pdn_id = lte_lc_pdn_id_get(cid);

	/* Add to PDN list */
	new_pdn_info = link_shell_pdn_info_list_add(pdn_id, cid);
	if (new_pdn_info == NULL) {
		mosh_error("ltelc_pdn_init_and_connect: could not add new PDN socket to list\n");
	}

	mosh_print("Created and activated a new PDP context: CID %d, PDN ID %d\n", cid, pdn_id);

	return 0;

cleanup_and_fail:
	if (cid != -1) {
		(void)lte_lc_pdn_ctx_destroy(cid);
	}

	return ret;
}

int link_shell_pdn_disconnect(int pdn_cid)
{
	int ret;

	ret = lte_lc_pdn_deactivate(pdn_cid);
	if (ret) {
		mosh_error("lte_lc_pdn_deactivate() failed, err %d", ret);
	}

	ret = lte_lc_pdn_ctx_destroy(pdn_cid);
	if (ret) {
		mosh_error("lte_lc_pdn_ctx_destroy() failed, err %d", ret);
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
	ret = lte_lc_pdn_activate(pdn_cid, &esm, NULL);
	if (ret) {
		mosh_error(
			"lte_lc_pdn_activate() failed, err %d esm %d %s\n",
			ret, esm, lte_lc_pdn_esm_strerror(esm));
	}
	return ret;
}

void link_shell_pdn_events_subscribe(void)
{
	lte_lc_register_handler(link_pdn_event_handler);
}

int link_shell_pdn_event_forward_cb_set(lte_lc_evt_handler_t cb)
{
	if (!cb) {
		return -EINVAL;
	}

	lte_lc_event_forward_callback = cb;

	return 0;
}

void link_shell_pdn_init(void)
{
	link_shell_pdn_events_subscribe();
	sys_dlist_init(&pdn_info_list);
}

int link_shell_pdn_auth_prot_to_pdn_lib_method_map(int auth_proto,
						   enum lte_lc_pdn_auth *method)
{
	*method = LTE_LC_PDN_AUTH_NONE;

	if (auth_proto == 0) {
		*method = LTE_LC_PDN_AUTH_NONE;
	} else if (auth_proto == 1) {
		*method = LTE_LC_PDN_AUTH_PAP;
	} else if (auth_proto == 2) {
		*method = LTE_LC_PDN_AUTH_CHAP;
	} else {
		return -EINVAL;
	}
	return 0;
}

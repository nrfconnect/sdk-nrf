/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <shell/shell.h>

#include <nrf_modem_at.h>
#include <modem/pdn.h>

#if defined(CONFIG_MOSH_PPP)
#include "ppp_ctrl.h"
#endif

#include "link_shell_print.h"
#include "link_shell_pdn.h"
#include "mosh_print.h"

static sys_dlist_t pdn_info_list;

struct link_shell_pdn_info {
	sys_dnode_t dnode;
	int pdn_id;
	uint8_t cid;
};

/* From the specification:
 * 3GPP TS 24.301 version 8.7.0
 * EMM cause, 9.9.3.9
 */

static const struct {
	int reason;
	const char *str;
} esm_table[] = {
	{ 0x08, "Operator Determined Barring" },
	{ 0x1a, "Insufficient resources" },
	{ 0x1b, "Missing or unknown APN" },
	{ 0x1c, "Unknown PDN type" },
	{ 0x1d, "User authentication failed" },
	{ 0x1e, "Request rejected by Serving GW or PDN GW" },
	{ 0x1f, "Request rejected, unspecified" },
	{ 0x20, "Service option not supported" },
	{ 0x21, "Requested service option not subscribed" },
	{ 0x22, "Service option temporarily out of order" },
	{ 0x23, "PTI already in use" },
	{ 0x24, "Regular deactivation" },
	{ 0x25, "EPS QoS not accepted" },
	{ 0x26, "Network failure" },
	{ 0x27, "Reactivation requested" },
	{ 0x29, "Semantic error in the TFT operation" },
	{ 0x2a, "Syntactical error in the TFT operation" },
	{ 0x2b, "Invalid EPS bearer identity" },
	{ 0x2c, "Semantic errors in packet filter(s)" },
	{ 0x2d, "Syntactical errors in packet filter(s)" },
	{ 0x2e, "Unused" },
	{ 0x2f, "PTI mismatch" },
	{ 0x31, "Last PDN disconnection not allowed" },
	{ 0x32, "PDN type IPv4 only allowed" },
	{ 0x33, "PDN type IPv6 only allowed" },
	{ 0x34, "Single address bearers only allowed" },
	{ 0x35, "ESM information not received" },
	{ 0x36, "PDN connection does not exist" },
	{ 0x37, "Multiple PDN connections for a given APN not allowed" },
	{ 0x38, "Collision with network initiated request" },
	{ 0x39, "PDN type IPv4v6 only allowed" },
	{ 0x3a, "PDN type non IP only allowed" },
	{ 0x3b, "Unsupported QCI value" },
	{ 0x3c, "Bearer handling not supported" },
	{ 0x3d, "PDN type Ethernet only allowed" },
	{ 0x41, "Maximum number of EPS bearers reached" },
	{ 0x42,
	  "Requested APN not supported in current RAT and PLMN combination" },
	{ 0x51, "Invalid PTI value" },
	{ 0x5f, "Semantically incorrect message" },
	{ 0x60, "Invalid mandatory information" },
	{ 0x61, "Message type non-existent or not implemented" },
	{ 0x62, "Message type not compatible with the protocol state" },
	{ 0x63, "Information element non-existent or not implemented" },
	{ 0x64, "Conditional IE error" },
	{ 0x65, "Message not compatible with the protocol state" },
	{ 0x6f, "Protocol error, unspecified" },
	{ 0x70,
	  "APN restriction value incompatible with active EPS bearer context" },
	{ 0x71, "Multiple accesses to a PDN connection not allowed" },
};

static const char *esm_strerr(int reason)
{
	for (size_t i = 0; i < ARRAY_SIZE(esm_table); i++) {
		if ((esm_table[i].reason == reason) &&
		    (esm_table[i].str != NULL)) {
			return esm_table[i].str;
		}
	}

	return "<unknown>";
}

static const char *const event_str[] = {
	[PDN_EVENT_CNEC_ESM] = "ESM",
	[PDN_EVENT_ACTIVATED] = "activated",
	[PDN_EVENT_DEACTIVATED] = "deactivated",
	[PDN_EVENT_IPV6_UP] = "IPv6 up",
	[PDN_EVENT_IPV6_DOWN] = "IPv6 down",
};

void link_pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		mosh_print("PDN event: PDP context %d, %s", cid, esm_strerr(reason));
		break;
	default:
		mosh_print("PDN event: PDP context %d %s", cid, event_str[event]);
#if defined(CONFIG_MOSH_PPP)
		if (cid == 0) {
			/* Notify PPP side about the default PDN activation status */
			if (event == PDN_EVENT_ACTIVATED) {
				ppp_ctrl_default_pdn_active(true);
			} else if (event == PDN_EVENT_DEACTIVATED) {
				ppp_ctrl_default_pdn_active(false);
			}
		}
#endif
		break;
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
			ret, esm, esm_strerr(esm));
	}
	return ret;
}

void link_shell_pdn_events_subscribe(void)
{
	int err;

	/* Register to the necessary packet domain AT notifications */
	err = nrf_modem_at_printf("AT+CNEC=16");
	if (err) {
		mosh_error("AT+CNEC=16 failed, err %d\n", err);
		return;
	}

	err = nrf_modem_at_printf("AT+CGEREP=1");
	if (err) {
		mosh_error("AT+CGEREP=1 failed, err %d\n", err);
		return;
	}
	pdn_default_callback_set(link_pdn_event_handler);
}

void link_shell_pdn_init(void)
{

	pdn_init();
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

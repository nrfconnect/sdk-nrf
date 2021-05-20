/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <shell/shell.h>

#include <modem/pdn.h>
#include <modem/at_cmd.h>

#include "link_shell_print.h"
#include "link_shell_pdn.h"

extern const struct shell *shell_global;

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
		shell_print(shell_global, "PDN event: PDP context %d, %s", cid,
			    esm_strerr(reason));
		break;
	default:
		shell_print(shell_global, "PDN event: PDP context %d %s", cid,
			    event_str[event]);
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
			shell_error(
				shell_global,
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

int link_shell_pdn_connect(const struct shell *shell, const char *apn_name,
			    const char *family_str)
{
	enum pdn_fam pdn_lib_family;
	uint8_t cid = -1;
	int ret;
	int esm;

	ret = pdn_ctx_create(&cid, link_pdn_event_handler);
	if (ret) {
		shell_error(shell, "pdn_ctx_create() failed, err %d\n", ret);
		goto cleanup_and_fail;
	}

	ret = link_family_str_to_pdn_lib_family(&pdn_lib_family, family_str);
	if (ret) {
		shell_error(
			shell,
			"link_family_str_to_pdn_lib_family() failed, err %d\n",
			ret);
		goto cleanup_and_fail;
	}

	/* Configure a PDP context with APN and Family */
	ret = pdn_ctx_configure(cid, apn_name, pdn_lib_family, NULL);
	if (ret) {
		shell_error(shell, "pdn_ctx_configure() failed, err %d\n", ret);
		goto cleanup_and_fail;
	}

	/* TODO: Set authentication params if requested by using pdn_ctx_auth_set() */

	/* Activate a PDN connection */
	ret = pdn_activate(cid, &esm);
	if (ret) {
		shell_error(shell, "pdn_activate() failed, err %d esm %d %s\n",
			    ret, esm, esm_strerr(esm));
		goto cleanup_and_fail;
	}

	shell_print(
		shell,
		"Created and activated a new PDP context: CID %d, PDN ID %d\n",
		cid, pdn_id_get(cid));

	return 0;

cleanup_and_fail:
	if (cid != -1) {
		(void)pdn_ctx_destroy(cid);
	}

	return ret;
}

int link_shell_pdn_disconnect(const struct shell *shell, int pdn_cid)
{
	int ret;

	ret = pdn_deactivate(pdn_cid);
	if (ret) {
		shell_error(shell, "pdn_deactivate() failed, err %d", ret);
	}

	ret = pdn_ctx_destroy(pdn_cid);
	if (ret) {
		shell_error(shell, "pdn_ctx_destroy() failed, err %d", ret);
		return ret;
	}

	shell_print(shell, "CID %d deactivated and context destroyed\n",
		    pdn_cid);

	return 0;
}

void link_shell_pdn_init(const struct shell *shell)
{
	int err;

	/* Register to the necessary packet domain AT notifications */
	err = at_cmd_write("AT+CNEC=16", NULL, 0, NULL);
	if (err) {
		shell_error(shell, "AT+CNEC=16 failed, err %d\n", err);
		return;
	}

	err = at_cmd_write("AT+CGEREP=1", NULL, 0, NULL);
	if (err) {
		shell_error(shell, "AT+CGEREP=1 failed, err %d\n", err);
		return;
	}

	pdn_init();
	pdn_default_callback_set(link_pdn_event_handler);
}

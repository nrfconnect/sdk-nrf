/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

/* From the specification:
 * 3GPP TS 24.301 version 8.7.0
 * EMM cause, 9.9.3.9
 */

static const struct {
	const int reason;
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
	{ 0x42, "Requested APN not supported in current RAT and PLMN combination" },
	{ 0x51, "Invalid PTI value" },
	{ 0x5f, "Semantically incorrect message" },
	{ 0x60, "Invalid mandatory information" },
	{ 0x61, "Message type non-existent or not implemented" },
	{ 0x62, "Message type not compatible with the protocol state" },
	{ 0x63, "Information element non-existent or not implemented" },
	{ 0x64, "Conditional IE error" },
	{ 0x65, "Message not compatible with the protocol state" },
	{ 0x6f, "Protocol error, unspecified" },
	{ 0x70, "APN restriction value incompatible with active EPS bearer context" },
	{ 0x71, "Multiple accesses to a PDN connection not allowed" },
};

const char *pdn_esm_strerror(int reason)
{
	for (size_t i = 0; i < ARRAY_SIZE(esm_table); i++) {
		if (esm_table[i].reason == reason) {
			return esm_table[i].str;
		}
	}

	return "<unknown>";
}

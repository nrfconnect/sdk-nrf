/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

/* From the specification:
 * 3GPP TS 24.301 version 17.8.0
 */

static const struct {
	const int reason;
	const char *str;
} emm_table[] = {
	{ 2, "IMSI unknown in HSS" },
	{ 3, "Illegal UE" },
	{ 5, "IMEI not accepted" },
	{ 6, "Illegal ME" },
	{ 7, "EPS services not allowed" },
	{ 8, "EPS services and non-EPS services not allowed" },
	{ 9, "UE identity cannot be derived by the network" },
	{ 10, "Implicitly detached" },
	{ 11, "PLMN not allowed" },
	{ 12, "Tracking Area not allowed" },
	{ 13, "Roaming not allowed in this tracking area" },
	{ 14, "EPS services not allowed in this PLMN" },
	{ 15, "No Suitable Cells In tracking area" },
	{ 16, "MSC temporarily not reachable" },
	{ 17, "Network failure" },
	{ 18, "CS domain not available" },
	{ 19, "ESM failure" },
	{ 20, "MAC failure" },
	{ 21, "Synch failure" },
	{ 22, "Congestion" },
	{ 23, "UE security capabilities mismatch" },
	{ 24, "Security mode rejected, unspecified" },
	{ 25, "Not authorized for this CSG" },
	{ 26, "Non-EPS authentication unacceptable" },
	{ 31, "Redirection to 5GCN required" },
	{ 35, "Requested service option not authorized in this PLMN" },
	{ 39, "CS service temporarily not available" },
	{ 40, "No EPS bearer context activated" },
	{ 42, "Severe network failure" },
	{ 78, "PLMN not allowed to operate at the present UE location" },
	{ 95, "Semantically incorrect message" },
	{ 96, "Invalid mandatory information" },
	{ 97, "Message type non-existent or not implemented" },
	{ 98, "Message type not compatible with the protocol state" },
	{ 99, "Information element non-existent or not implemented" },
	{ 100, "Conditional IE error" },
	{ 101, "Message not compatible with the protocol state" },
	{ 111, "Protocol error, unspecified" }
};

const char *pdn_emm_strerror(int reason)
{
	for (size_t i = 0; i < ARRAY_SIZE(emm_table); i++) {
		if (emm_table[i].reason == reason) {
			return emm_table[i].str;
		}
	}

	return "<unknown>";
}

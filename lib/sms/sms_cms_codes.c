/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

/* From the specifications:
 * 3GPP TS 24.011 version 14.3.0 clause E.2
 * 3GPP TS 23.040 version 14.0.0 clause 9.2.3.22
 * 3GPP TS 27.005 version 14.0.0 clause 3.2.5
 */

static const struct {
	const int reason;
	const char *str;
} cms_table[] = {
	{ 1, "Unassigned (unallocated) number" },
	{ 8, "Operator determined barring" },
	{ 10, "Call barred" },
	{ 17, "Network failure" },
	{ 21, "Short message transfer rejected" },
	{ 22, "Congestion/Memory capacity exceeded" },
	{ 27, "Destination out of service" },
	{ 28, "Unidentified subscriber" },
	{ 29, "Facility rejected" },
	{ 30, "Unknown subscriber" },
	{ 38, "Network out of order" },
	{ 41, "Temporary failure" },
	{ 42, "Congestion" },
	{ 47, "Resources unavailable, unspecified" },
	{ 50, "Requested facility not subscribed" },
	{ 69, "Requested facility not implemented" },
	{ 81, "Invalid short message transfer reference value" },
	{ 95, "Semantically incorrect message" },
	{ 96, "Invalid mandatory information" },
	{ 97, "Message type non-existent or not implemented" },
	{ 98, "Message not compatible with short message protocol state" },
	{ 99, "Information element non-existent or not implemented" },
	{ 111, "Protocol error, unspecified" },
	{ 127, "Interworking, unspecified" },
	{ 128, "Telematic interworking not supported" },
	{ 129, "Short message Type 0 not supported" },
	{ 130, "Cannot replace short message" },
	{ 143, "Unspecified TP-PID error" },
	{ 144, "Data coding scheme (alphabet) not supported" },
	{ 145, "Message class not supported" },
	{ 159, "Unspecified TP-DCS error" },
	{ 160, "Command cannot be actioned" },
	{ 161, "Command unsupported" },
	{ 175, "Unspecified TP-Command error" },
	{ 176, "TPDU not supported" },
	{ 192, "SC busy" },
	{ 193, "No SC subscription" },
	{ 194, "SC system failure" },
	{ 195, "Invalid SME address" },
	{ 196, "Destination SME barred" },
	{ 197, "SM Rejected-Duplicate SM" },
	{ 198, "TP-VPF not supported" },
	{ 199, "TP-VP not supported" },
	{ 208, "(U)SIM SMS storage full" },
	{ 209, "No SMS storage capability in (U)SIM" },
	{ 210, "Error in MS" },
	{ 211, "Memory Capacity Exceeded" },
	{ 212, "(U)SIM Application Toolkit Busy" },
	{ 213, "(U)SIM data download error" },
	{ 255, "Unspecified error cause" },
	{ 300, "ME failure" },
	{ 301, "SMS service of ME reserved" },
	{ 302, "operation not allowed" },
	{ 303, "operation not supported" },
	{ 304, "invalid PDU mode parameter" },
	{ 305, "invalid text mode parameter" },
	{ 310, "(U)SIM not inserted" },
	{ 311, "(U)SIM PIN required" },
	{ 312, "PH-(U)SIM PIN required" },
	{ 313, "(U)SIM failure" },
	{ 314, "(U)SIM busy" },
	{ 315, "(U)SIM wrong" },
	{ 316, "(U)SIM PUK required" },
	{ 317, "(U)SIM PIN2 required" },
	{ 318, "(U)SIM PUK2 required" },
	{ 320, "memory failure" },
	{ 321, "invalid memory index" },
	{ 322, "memory full" },
	{ 330, "SMSC address unknown" },
	{ 331, "no network service" },
	{ 332, "network timeout" },
	{ 340, "no +CNMA acknowledgment expected" },
	{ 500, "unknown error" },
	{ 513, "Not found" },
	{ 514, "Not allowed" },
	{ 515, "No memory" },
	{ 524, "SMS client unregistered" },
};

const char *sms_cms_error_str(int reason)
{
	for (size_t i = 0; i < ARRAY_SIZE(cms_table); i++) {
		if (cms_table[i].reason == reason) {
			return cms_table[i].str;
		}
	}

	return "<unknown>";
}

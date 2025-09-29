/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef GROUND_FIX_ENCODE_TYPES_H__
#define GROUND_FIX_ENCODE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 10

struct cell_earfcn {
	uint32_t cell_earfcn;
};

struct cell_adv {
	uint32_t cell_adv;
};

struct ncell_rsrp {
	int32_t ncell_rsrp;
};

struct ncell_rsrq_r {
	union {
		float ncell_rsrq_float32;
		int32_t ncell_rsrq_int;
	};
	enum {
		ncell_rsrq_float32_c,
		ncell_rsrq_int_c,
	} ncell_rsrq_choice;
};

struct ncell_timeDiff {
	int32_t ncell_timeDiff;
};

struct ncell {
	uint32_t ncell_earfcn;
	uint32_t ncell_pci;
	struct ncell_rsrp ncell_rsrp;
	bool ncell_rsrp_present;
	struct ncell_rsrq_r ncell_rsrq;
	bool ncell_rsrq_present;
	struct ncell_timeDiff ncell_timeDiff;
	bool ncell_timeDiff_present;
};

struct cell_nmr_r {
	struct ncell cell_nmr_ncells[5];
	size_t cell_nmr_ncells_count;
};

struct cell_rsrp {
	int32_t cell_rsrp;
};

struct cell_rsrq_r {
	union {
		float cell_rsrq_float32;
		int32_t cell_rsrq_int;
	};
	enum {
		cell_rsrq_float32_c,
		cell_rsrq_int_c,
	} cell_rsrq_choice;
};

struct cell {
	uint32_t cell_mcc;
	uint32_t cell_mnc;
	uint32_t cell_eci;
	uint32_t cell_tac;
	struct cell_earfcn cell_earfcn;
	bool cell_earfcn_present;
	struct cell_adv cell_adv;
	bool cell_adv_present;
	struct cell_nmr_r cell_nmr;
	bool cell_nmr_present;
	struct cell_rsrp cell_rsrp;
	bool cell_rsrp_present;
	struct cell_rsrq_r cell_rsrq;
	bool cell_rsrq_present;
};

struct lte_ar {
	struct cell lte_ar_cell_m[5];
	size_t lte_ar_cell_m_count;
};

struct ground_fix_req_lte {
	struct lte_ar ground_fix_req_lte;
};

struct ap_age {
	uint32_t ap_age;
};

struct ap_signalStrength {
	int32_t ap_signalStrength;
};

struct ap_channel {
	uint32_t ap_channel;
};

struct ap_frequency {
	uint32_t ap_frequency;
};

struct ap_ssid {
	struct zcbor_string ap_ssid;
};

struct ap {
	struct zcbor_string ap_macAddress;
	struct ap_age ap_age;
	bool ap_age_present;
	struct ap_signalStrength ap_signalStrength;
	bool ap_signalStrength_present;
	struct ap_channel ap_channel;
	bool ap_channel_present;
	struct ap_frequency ap_frequency;
	bool ap_frequency_present;
	struct ap_ssid ap_ssid;
	bool ap_ssid_present;
};

struct wifi_ob {
	struct ap wifi_ob_accessPoints_ap_m[20];
	size_t wifi_ob_accessPoints_ap_m_count;
};

struct ground_fix_req_wifi {
	struct wifi_ob ground_fix_req_wifi;
};

struct ground_fix_req {
	struct ground_fix_req_lte ground_fix_req_lte;
	bool ground_fix_req_lte_present;
	struct ground_fix_req_wifi ground_fix_req_wifi;
	bool ground_fix_req_wifi_present;
};

#ifdef __cplusplus
}
#endif

#endif /* GROUND_FIX_ENCODE_TYPES_H__ */

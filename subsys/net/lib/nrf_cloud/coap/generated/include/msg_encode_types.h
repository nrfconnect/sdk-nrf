/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef MSG_ENCODE_TYPES_H__
#define MSG_ENCODE_TYPES_H__

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

struct pvt_spd {
	double pvt_spd;
};

struct pvt_hdg {
	double pvt_hdg;
};

struct pvt_alt {
	double pvt_alt;
};

struct pvt {
	double pvt_lat;
	double pvt_lng;
	double pvt_acc;
	struct pvt_spd pvt_spd;
	bool pvt_spd_present;
	struct pvt_hdg pvt_hdg;
	bool pvt_hdg_present;
	struct pvt_alt pvt_alt;
	bool pvt_alt_present;
};

struct message_out_ts {
	uint64_t message_out_ts;
};

struct message_out {
	struct zcbor_string message_out_appId;
	union {
		struct zcbor_string message_out_data_tstr;
		double message_out_data_float;
		int32_t message_out_data_int;
		struct pvt message_out_data_pvt_m;
	};
	enum {
		message_out_data_tstr_c,
		message_out_data_float_c,
		message_out_data_int_c,
		message_out_data_pvt_m_c,
	} message_out_data_choice;
	struct message_out_ts message_out_ts;
	bool message_out_ts_present;
};

#ifdef __cplusplus
}
#endif

#endif /* MSG_ENCODE_TYPES_H__ */

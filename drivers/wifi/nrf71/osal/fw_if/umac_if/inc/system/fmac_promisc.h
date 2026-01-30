/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing code to support promiscuous mode
 * for the FMAC IF Layer of the Wi-Fi driver.
 */
#ifndef __FMAC_PROMISC_H__
#define __FMAC_PROMISC_H__
#include "system/fmac_structs.h"
#include "common/pack_def.h"

/** 802.11 Packet-type */
enum nrf_wifi_fmac_frame_type {
	/** 802.11 management packet type */
	NRF_WIFI_MGMT_PKT_TYPE = 0x0,
	/** 802.11 control packet type */
	NRF_WIFI_CTRL_PKT_TYPE,
	/** 802.11 data packet type */
	NRF_WIFI_DATA_PKT_TYPE,
};

/** Packet filter settings in driver */
enum nrf_wifi_packet_filter {
	/** Filter management packets only */
	FILTER_MGMT_ONLY = 0x2,
	/** Filter data packets only */
	FILTER_DATA_ONLY = 0x4,
	/** Filter data and management packets */
	FILTER_DATA_MGMT = 0x6,
	/** Filter control packets only */
	FILTER_CTRL_ONLY = 0x8,
	/** Filter control and management packets */
	FILTER_CTRL_MGMT = 0xa,
	/** Filter data and control packets */
	FILTER_DATA_CTRL = 0xc
};

/** Frame Control structure which
 *  depends on the endianness in the system.
 *  For example 0x8842 in big endian would
 *  become 0x4288 in little endian. The bytes
 *  are swapped.
 **/
#if defined(CONFIG_LITTLE_ENDIAN)
struct nrf_wifi_fmac_frame_ctrl {
	unsigned short protocolVersion : 2;
	unsigned short type            : 2;
	unsigned short subtype         : 4;
	unsigned short toDS            : 1;
	unsigned short fromDS          : 1;
	unsigned short moreFragments   : 1;
	unsigned short retry           : 1;
	unsigned short powerManagement : 1;
	unsigned short moreData        : 1;
	unsigned short protectedFrame  : 1;
	unsigned short order           : 1;
} __NRF_WIFI_PKD;
#else
struct nrf_wifi_fmac_frame_ctrl {
	unsigned short toDS            : 1;
	unsigned short fromDS          : 1;
	unsigned short moreFragments   : 1;
	unsigned short retry           : 1;
	unsigned short powerManagement : 1;
	unsigned short moreData        : 1;
	unsigned short protectedFrame  : 1;
	unsigned short order           : 1;
	unsigned short protocolVersion : 2;
	unsigned short type            : 2;
	unsigned short subtype         : 4;
} __NRF_WIFI_PKD;
#endif

bool nrf_wifi_util_check_filt_setting(struct nrf_wifi_fmac_vif_ctx *vif,
				      unsigned short *frame_control);
#endif /* __FMAC_PROMISC_H__ */

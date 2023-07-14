/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Common interface between host and RPU
 *
 */
#ifndef __NRF_WIFI_HOST_RPU_COMMON_IFACE_H__
#define __NRF_WIFI_HOST_RPU_COMMON_IFACE_H__

#include "rpu_if.h"

#include "pack_def.h"

#define NRF_WIFI_UMAC_VER(version) (((version)&0xFF000000) >> 24)
#define NRF_WIFI_UMAC_VER_MAJ(version) (((version)&0x00FF0000) >> 16)
#define NRF_WIFI_UMAC_VER_MIN(version) (((version)&0x0000FF00) >> 8)
#define NRF_WIFI_UMAC_VER_EXTRA(version) (((version)&0x000000FF) >> 0)
#define RPU_MEM_UMAC_BOOT_SIG 0xB0000000
#define RPU_MEM_UMAC_VER 0xB0000004
#define RPU_MEM_UMAC_PEND_Q_BMP 0xB0000008
#define RPU_MEM_UMAC_CMD_ADDRESS 0xB00007A8
#define RPU_MEM_UMAC_EVENT_ADDRESS 0xB0000E28
#define RPU_MEM_UMAC_PATCH_BIN 0x8008C000
#define RPU_MEM_UMAC_PATCH_BIMG 0x80099400

#define NRF_WIFI_UMAC_BOOT_SIG 0x5A5A5A5A
#define NRF_WIFI_UMAC_ROM_PATCH_OFFSET (RPU_MEM_UMAC_PATCH_BIMG - RPU_ADDR_UMAC_CORE_RET_START)
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_0 0x3c1a8000
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_1 0x275a0000
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_2 0x03400008
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_3 0x00000000

/**
 * @brief This enum defines the different categories of messages that can be exchanged between
 *  the Host and the RPU.
 *
 */
enum nrf_wifi_host_rpu_msg_type {
	/** System interface messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
	/** Unused */
	NRF_WIFI_HOST_RPU_MSG_TYPE_SUPPLICANT,
	/** Data path messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_DATA,
	/** Control path messages */
	NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC
};
/**
 * @brief This structure defines the common message header used to encapsulate each message
 *  exchanged between the Host and UMAC.
 *
 */

struct host_rpu_msg {
	/** Header */
	struct host_rpu_msg_hdr hdr;
	/** Type of the RPU message see &enum nrf_wifi_host_rpu_msg_type */
	signed int type;
	/** Actual message */
	signed char msg[0];
} __NRF_WIFI_PKD;

#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_VO (1 << 0)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_VI (1 << 1)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_BE (1 << 2)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_BK (1 << 3)

/**
 * @brief This structure represents the bitmap of STA (Station) pending frames in
 *  SoftAP power save mode.
 *
 */

struct sap_pend_frames_bitmap {
	/** STA MAC address */
	unsigned char mac_addr[6];
	/** Pending frames bitmap for each access category */
	unsigned char pend_frames_bitmap;
} __NRF_WIFI_PKD;

/**
 * @brief This structure represents the information related to UMAC.
 *
 */
struct host_rpu_umac_info {
	/** Boot status signature */
	unsigned int boot_status;
	/** UMAC version */
	unsigned int version;
	/** @ref sap_pend_frames_bitmap */
	struct sap_pend_frames_bitmap sap_bitmap[4];
	/** Hardware queues info &enum host_rpu_hpqm_info */
	struct host_rpu_hpqm_info hpqm_info;
	/** OTP params */
	unsigned int info_part;
	/** OTP params */
	unsigned int info_variant;
	/** OTP params */
	unsigned int info_lromversion;
	/** OTP params */
	unsigned int info_uromversion;
	/** OTP params */
	unsigned int info_uuid[4];
	/** OTP params */
	unsigned int info_spare0;
	/** OTP params */
	unsigned int info_spare1;
	/** OTP params */
	unsigned int mac_address0[2];
	/** OTP params */
	unsigned int mac_address1[2];
	/** OTP params */
	unsigned int calib[9];
} __NRF_WIFI_PKD;
#endif /* __NRF_WIFI_HOST_RPU_IFACE_H__ */

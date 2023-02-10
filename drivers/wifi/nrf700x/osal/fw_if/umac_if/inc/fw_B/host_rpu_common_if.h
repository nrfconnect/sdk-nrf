/*
 *
 *Copyright (c) 2022 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *
 *@brief <Common interface between host and RPU>
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
#define RPU_MEM_UMAC_PATCH_BIMG 0x80094400

#define NRF_WIFI_UMAC_BOOT_SIG 0x5A5A5A5A
#define NRF_WIFI_UMAC_ROM_PATCH_OFFSET (RPU_MEM_UMAC_PATCH_BIMG - RPU_ADDR_UMAC_CORE_RET_START)
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_0 0x3c1a8000
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_1 0x275a0000
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_2 0x03400008
#define NRF_WIFI_UMAC_BOOT_EXCP_VECT_3 0x00000000

/**
 * enum nrf_wifi_host_rpu_msg_type - RPU message type
 * @NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM: Unused
 * @NRF_WIFI_HOST_RPU_MSG_TYPE_SUPPLICANT: Unused
 * @NRF_WIFI_HOST_RPU_MSG_TYPE_DATA: Data path and System messages
 * @NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC: Control path messages
 *
 * Different categories of messages that can passed between the Host and
 * the RPU.
 */
enum nrf_wifi_host_rpu_msg_type {
/* INIT command may send using system type */
	NRF_WIFI_HOST_RPU_MSG_TYPE_SYSTEM,
	NRF_WIFI_HOST_RPU_MSG_TYPE_SUPPLICANT,
	NRF_WIFI_HOST_RPU_MSG_TYPE_DATA,
	NRF_WIFI_HOST_RPU_MSG_TYPE_UMAC
};
/**
 * struct host_rpu_msg - Message header for HOST-RPU interaction
 * @hdr: Message header
 * @type: Type of the RPU message
 * @msg: Actual message
 *
 * The common message header that encapsulates each message passed between the
 * Host and UMAC.
 */

struct host_rpu_msg {
	struct host_rpu_msg_hdr hdr;
	signed int type;
	signed char msg[0]; /* actual message */
} __NRF_WIFI_PKD;

#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_VO (1 << 0)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_VI (1 << 1)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_BE (1 << 2)
#define NRF_WIFI_PENDING_FRAMES_BITMAP_AC_BK (1 << 3)

/**
 * struct sta_pend_frames_bitmap - STA pending frames bitmap in SoftAP power save mode.
 * @mac_addr: STA MAC address
 * @pend_frames_bitmap: Pending frames bitmap for each access category
 */

struct sap_pend_frames_bitmap {
	unsigned char mac_addr[6];
	unsigned char pend_frames_bitmap;
} __NRF_WIFI_PKD;

struct host_rpu_umac_info {
	unsigned int boot_status;
	unsigned int version;
	struct sap_pend_frames_bitmap sap_bitmap[4];
	struct host_rpu_hpqm_info hpqm_info;
	unsigned int info_part;
	unsigned int info_variant;
	unsigned int info_lromversion;
	unsigned int info_uromversion;
	unsigned int info_uuid[4];
	unsigned int info_spare0;
	unsigned int info_spare1;
	unsigned int mac_address0[2];
	unsigned int mac_address1[2];
	unsigned int calib[9];
} __NRF_WIFI_PKD;
#endif /* __NRF_WIFI_HOST_RPU_IFACE_H__ */

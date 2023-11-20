/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TFM_READ_RANGES_H__
#define TFM_READ_RANGES_H__

#include <pm_config.h>

#include <tfm_ioctl_core_api.h>

#include "nrf.h"

#ifdef NRF_FICR_S_BASE

#define FICR_BASE               NRF_FICR_S_BASE

#define FICR_INFO_ADDR          (FICR_BASE + offsetof(NRF_FICR_Type, INFO))
#define FICR_INFO_SIZE          (sizeof(FICR_INFO_Type))

#if defined(FICR_NFC_TAGHEADER0_MFGID_Msk)
#define FICR_NFC_ADDR           (FICR_BASE + offsetof(NRF_FICR_Type, NFC))
#define FICR_NFC_SIZE           (sizeof(FICR_NFC_Type))
#endif

#if defined(FICR_XOSC32MTRIM_SLOPE_Msk)
#define FICR_XOSC32MTRIM_ADDR   (FICR_BASE + offsetof(NRF_FICR_Type, XOSC32MTRIM))
#define FICR_XOSC32MTRIM_SIZE   (sizeof(uint32_t))
#endif

/* Used by nrf_erratas.h */
#define FICR_RESTRICTED_ADDR    (FICR_BASE + 0x130)
#define FICR_RESTRICTED_SIZE    0x8

#if defined(FICR_SIPINFO_PARTNO_PARTNO_Pos)
#define FICR_SIPINFO_ADDR       (FICR_BASE + offsetof(NRF_FICR_Type, SIPINFO))
#define FICR_SIPINFO_SIZE       (sizeof(FICR_SIPINFO_Type))
#endif

#endif /* NRF_FICR_S_BASE */

static const struct tfm_read_service_range ranges[] = {
#ifdef PM_MCUBOOT_ADDRESS
	/* Allow reads of mcuboot metadata */
	{ .start = PM_MCUBOOT_PAD_ADDRESS, .size = PM_MCUBOOT_PAD_SIZE },
#endif
#if defined(FICR_INFO_ADDR)
	{ .start = FICR_INFO_ADDR, .size = FICR_INFO_SIZE },
#endif
#if defined(FICR_NFC_ADDR)
	{ .start = FICR_NFC_ADDR, .size = FICR_NFC_SIZE },
#endif
#if defined(FICR_RESTRICTED_ADDR)
	{ .start = FICR_RESTRICTED_ADDR, .size = FICR_RESTRICTED_SIZE },
#endif
#if defined(FICR_XOSC32MTRIM_ADDR)
	{ .start = FICR_XOSC32MTRIM_ADDR, .size = FICR_XOSC32MTRIM_SIZE },
#endif
#if defined(FICR_SIPINFO_ADDR)
	{ .start = FICR_SIPINFO_ADDR, .size = FICR_SIPINFO_SIZE },
#endif
};

#endif /* TFM_READ_RANGES_H__ */

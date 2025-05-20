/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TFM_PLATFORM_USER_MEMORY_RANGES_H__
#define TFM_PLATFORM_USER_MEMORY_RANGES_H__

#include <pm_config.h>

#include <tfm_ioctl_core_api.h>

#include "nrf.h"

/*
 * On platforms like nrf53 we provide a service for reading out
 * FICR. But on platforms like nrf54l, FICR is hardware fixed to NS so
 * the non-secure image can just read it directly.
 */
#ifdef NRF_FICR_S_BASE

#define FICR_BASE NRF_FICR_S_BASE

#define FICR_INFO_ADDR (FICR_BASE + offsetof(NRF_FICR_Type, INFO))
#define FICR_INFO_SIZE (sizeof(FICR_INFO_Type))

#if defined(FICR_NFC_TAGHEADER0_MFGID_Msk)
#define FICR_NFC_ADDR (FICR_BASE + offsetof(NRF_FICR_Type, NFC))
#define FICR_NFC_SIZE (sizeof(FICR_NFC_Type))
#endif

#if defined(FICR_XOSC32MTRIM_SLOPE_Msk)
#define FICR_XOSC32MTRIM_ADDR (FICR_BASE + offsetof(NRF_FICR_Type, XOSC32MTRIM))
#define FICR_XOSC32MTRIM_SIZE (sizeof(uint32_t))
#endif

/* Used by nrf_erratas.h */
#define FICR_RESTRICTED_ADDR (FICR_BASE + 0x130)
#define FICR_RESTRICTED_SIZE 0x8

#if defined(FICR_SIPINFO_PARTNO_PARTNO_Pos)
#define FICR_SIPINFO_ADDR (FICR_BASE + offsetof(NRF_FICR_Type, SIPINFO))
#define FICR_SIPINFO_SIZE (sizeof(FICR_SIPINFO_Type))
#endif

#endif /* NRF_FICR_S_BASE */

#ifdef NRF_APPLICATION_CPUC_S_BASE
#define CPUC_CPUID_ADDR (NRF_APPLICATION_CPUC_S_BASE + offsetof(NRF_CPUC_Type, CPUID))
#define CPUC_CPUID_SIZE (sizeof(uint32_t))
#endif /* NRF_APPLICATION_CPUC_S_BASE */

#if defined(NRF91_SERIES) || defined(NRF53_SERIES)
#define UICR_OTP_ADDR (NRF_UICR_S_BASE + offsetof(NRF_UICR_Type, OTP))
#define UICR_OTP_SIZE (sizeof(NRF_UICR_S->OTP))
#endif

static const struct tfm_read_service_range ranges[] = {
#ifdef PM_MCUBOOT_ADDRESS
	/* Allow reads of mcuboot metadata */
	{.start = PM_MCUBOOT_PAD_ADDRESS, .size = PM_MCUBOOT_PAD_SIZE},
#endif
#if defined(FICR_INFO_ADDR)
	{.start = FICR_INFO_ADDR, .size = FICR_INFO_SIZE},
#endif
#if defined(FICR_NFC_ADDR)
	{.start = FICR_NFC_ADDR, .size = FICR_NFC_SIZE},
#endif
#if defined(FICR_RESTRICTED_ADDR)
	{.start = FICR_RESTRICTED_ADDR, .size = FICR_RESTRICTED_SIZE},
#endif
#if defined(FICR_XOSC32MTRIM_ADDR)
	{.start = FICR_XOSC32MTRIM_ADDR, .size = FICR_XOSC32MTRIM_SIZE},
#endif
#if defined(FICR_SIPINFO_ADDR)
	{.start = FICR_SIPINFO_ADDR, .size = FICR_SIPINFO_SIZE},
#endif
#if defined(NRF_APPLICATION_CPUC_S)
	{.start = CPUC_CPUID_ADDR, .size = CPUC_CPUID_SIZE},
#endif

#if defined(UICR_OTP_ADDR)
	{.start = UICR_OTP_ADDR, .size = UICR_OTP_SIZE},
#endif

};

#if defined(NRF_RRAMC_S)
/**
 * 0x518 is the offset for the low power configuration, currently not in MDK,
 * the first two bits control the lower power mode of RRAMC.
 */
#define NRF_RRAMC_LOWPOWER_CONFIG_ADDR (NRF_RRAMC_S_BASE + 0x518)

/* Values: 0x0 = Power down mode, 0x1 = Standby mode */
static const uint32_t rramc_lowpower_config_allowed[] = {0x0, 0x1};
#endif /* NRF_RRAMC_S */

static const struct tfm_write32_service_address tfm_write32_service_addresses[] = {
#if defined(NRF_RRAMC_S)
	{.addr = NRF_RRAMC_LOWPOWER_CONFIG_ADDR,
	 .mask = 0x3,
	 .allowed_values = rramc_lowpower_config_allowed,
	 .allowed_values_array_size = 2 },
#else
	/* This is a dummy value because this table cannot be empty */
	{.addr = 0xFFFFFFFF, .mask = 0x0, .allowed_values = NULL, .allowed_values_array_size = 0},
#endif
};

#endif /* TFM_PLATFORM_USER_MEMORY_RANGES_H__ */

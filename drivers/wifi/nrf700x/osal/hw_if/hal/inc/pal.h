/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing SoC specific declarations for the
 * HAL Layer of the Wi-Fi driver.
 */

#ifndef __PAL_H__
#define __PAL_H__

#include "hal_api.h"

#define SOC_BOOT_SUCCESS 0
#define SOC_BOOT_FAIL 1
#define SOC_BOOT_ERRORS 2

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#define SOC_MMAP_ADDR_RPU_PS_CTRL 0x3FFFFC
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

#define DEFAULT_IMGPCI_VENDOR_ID 0x0700
#define DEFAULT_IMGPCI_DEVICE_ID PCI_ANY_ID
#define PCIE_BAR_OFFSET_WLAN_RPU 0x0
#define PCIE_DMA_MASK 0xFFFFFFFF


#define SOC_MMAP_ADDR_OFFSET_PKTRAM_HOST_VIEW 0x0C0000
#define SOC_MMAP_ADDR_OFFSET_PKTRAM_RPU_VIEW 0x380000

#ifdef RPU_CONFIG_72
#define SOC_MMAP_ADDR_OFFSET_GRAM_PKD 0xC00000
#define SOC_MMAP_ADDR_OFFSET_SYSBUS 0xE00000
#define SOC_MMAP_ADDR_OFFSET_PBUS 0xE40000
#else
#define SOC_MMAP_ADDR_OFFSET_GRAM_PKD 0x80000
#define SOC_MMAP_ADDR_OFFSET_SYSBUS 0x00000
#define SOC_MMAP_ADDR_OFFSET_PBUS 0x40000

static const unsigned int SOC_MMAP_ADDR_OFFSETS_MCU[] = {
	0x100000,
	0x200000
};

#endif /* RPU_CONFIG_72 */

#define RPU_MCU_CORE_INDIRECT_BASE 0xC0000000

#define NRF_WIFI_FW_LMAC_PATCH_LOC_PRI "img/wlan/nrf_wifi_lmac_patch_pri.bimg"
#define NRF_WIFI_FW_LMAC_PATCH_LOC_SEC "img/wlan/nrf_wifi_lmac_patch_sec.bin"
#define NRF_WIFI_FW_UMAC_PATCH_LOC_PRI "img/wlan/nrf_wifi_umac_patch_pri.bimg"
#define NRF_WIFI_FW_UMAC_PATCH_LOC_SEC "img/wlan/nrf_wifi_umac_patch_sec.bin"

enum nrf_wifi_fw_type {
	NRF_WIFI_FW_TYPE_LMAC_PATCH,
	NRF_WIFI_FW_TYPE_UMAC_PATCH,
	NRF_WIFI_FW_TYPE_MAX
};

enum nrf_wifi_fw_subtype {
	NRF_WIFI_FW_SUBTYPE_PRI,
	NRF_WIFI_FW_SUBTYPE_SEC,
	NRF_WIFI_FW_SUBTYPE_MAX
};

bool pal_check_rpu_mcu_regions(enum RPU_PROC_TYPE proc, unsigned int addr_val);

static inline enum RPU_MCU_ADDR_REGIONS pal_mem_type_to_region(enum HAL_RPU_MEM_TYPE mem_type)
{
	switch (mem_type) {
	case HAL_RPU_MEM_TYPE_CORE_ROM:
		return RPU_MCU_ADDR_REGION_ROM;
	case HAL_RPU_MEM_TYPE_CORE_RET:
		return RPU_MCU_ADDR_REGION_RETENTION;
	case HAL_RPU_MEM_TYPE_CORE_SCRATCH:
		return RPU_MCU_ADDR_REGION_SCRATCH;
	default:
		return RPU_MCU_ADDR_REGION_MAX;
	}
}

enum nrf_wifi_status pal_rpu_addr_offset_get(struct nrf_wifi_osal_priv *opriv,
					     unsigned int rpu_addr,
					     unsigned long *addr_offset,
						 enum RPU_PROC_TYPE proc);


#ifdef CONFIG_NRF_WIFI_LOW_POWER
unsigned long pal_rpu_ps_ctrl_reg_addr_get(void);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

char *pal_ops_get_fw_loc(struct nrf_wifi_osal_priv *opriv,
			 enum nrf_wifi_fw_type fw_type,
			 enum nrf_wifi_fw_subtype fw_subtype);

#endif /* __PAL_H__ */

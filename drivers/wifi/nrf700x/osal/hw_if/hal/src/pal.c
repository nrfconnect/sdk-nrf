/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing SoC specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "pal.h"
#include "hal_api.h"

enum wifi_nrf_status pal_rpu_addr_offset_get(struct wifi_nrf_osal_priv *opriv,
					     unsigned int rpu_addr,
					     unsigned long *addr)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int addr_base = (rpu_addr & RPU_ADDR_MASK_BASE);
	unsigned long region_offset = 0;

	if (addr_base == RPU_ADDR_SBUS_START) {
		region_offset = SOC_MMAP_ADDR_OFFSET_SYSBUS;
	} else if ((rpu_addr >= RPU_ADDR_GRAM_START) &&
		   (rpu_addr <= RPU_ADDR_GRAM_END)) {
		region_offset = SOC_MMAP_ADDR_OFFSET_GRAM_PKD;
	} else if (addr_base == RPU_ADDR_PBUS_START) {
		region_offset = SOC_MMAP_ADDR_OFFSET_PBUS;
	} else if (addr_base == RPU_ADDR_PKTRAM_START) {
		region_offset = SOC_MMAP_ADDR_OFFSET_PKTRAM_HOST_VIEW;
	} else {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Invalid rpu_addr 0x%X\n",
				      __func__,
				      rpu_addr);
		goto out;
	}

	*addr = region_offset + (rpu_addr & RPU_ADDR_MASK_OFFSET);

	status = WIFI_NRF_STATUS_SUCCESS;
out:
	return status;
}



#ifdef CONFIG_NRF_WIFI_LOW_POWER
unsigned long pal_rpu_ps_ctrl_reg_addr_get(void)
{
	return SOC_MMAP_ADDR_RPU_PS_CTRL;
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

char *pal_ops_get_fw_loc(struct wifi_nrf_osal_priv *opriv,
			 enum wifi_nrf_fw_type fw_type,
			 enum wifi_nrf_fw_subtype fw_subtype)
{
	char *fw_loc = NULL;

	switch (fw_type) {
	case WIFI_NRF_FW_TYPE_LMAC_PATCH:
		if (fw_subtype == WIFI_NRF_FW_SUBTYPE_PRI) {
			fw_loc = WIFI_NRF_FW_LMAC_PATCH_LOC_PRI;
		} else if (fw_subtype == WIFI_NRF_FW_SUBTYPE_SEC) {
			fw_loc = WIFI_NRF_FW_LMAC_PATCH_LOC_SEC;
		} else {
			wifi_nrf_osal_log_err(opriv,
					      "%s: Invalid LMAC FW sub-type = %d\n",
					      __func__,
					      fw_subtype);
			goto out;
		}
		break;
	case WIFI_NRF_FW_TYPE_UMAC_PATCH:
		if (fw_subtype == WIFI_NRF_FW_SUBTYPE_PRI) {
			fw_loc = WIFI_NRF_FW_UMAC_PATCH_LOC_PRI;
		} else if (fw_subtype == WIFI_NRF_FW_SUBTYPE_SEC) {
			fw_loc = WIFI_NRF_FW_UMAC_PATCH_LOC_SEC;
		} else {
			wifi_nrf_osal_log_err(opriv,
					      "%s: Invalid UMAC FW sub-type = %d\n",
					      __func__,
					      fw_subtype);
			goto out;
		}
		break;
	default:
		wifi_nrf_osal_log_err(opriv,
				      "%s: Invalid FW type = %d\n",
				      __func__,
				      fw_type);
		goto out;
	}

out:
	return fw_loc;
}

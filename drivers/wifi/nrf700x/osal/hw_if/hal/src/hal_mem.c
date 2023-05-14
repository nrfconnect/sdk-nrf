/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing memory read/write specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "pal.h"
#include "hal_api.h"
#include "hal_common.h"
#include "hal_reg.h"
#include "hal_mem.h"


static bool hal_rpu_is_mem_ram(enum RPU_PROC_TYPE proc, unsigned int addr_val)
{
	if (((addr_val >= RPU_ADDR_GRAM_START) &&
	     (addr_val <= RPU_ADDR_GRAM_END)) ||
	    ((addr_val >= RPU_ADDR_PKTRAM_START) &&
	     (addr_val <= RPU_ADDR_GRAM_END)) ||
	    ((addr_val >= RPU_ADDR_PKTRAM_START) &&
	     (addr_val <= RPU_ADDR_PKTRAM_END))) {
		return true;
	} else {
		return false;
	}
}


static bool hal_rpu_is_mem_bev(unsigned int addr_val)
{
	if (((addr_val >= RPU_ADDR_BEV_START) &&
	     (addr_val <= RPU_ADDR_BEV_END))) {
		return true;
	} else {
		return false;
	}
}

static bool hal_rpu_is_mem_core_direct(enum RPU_PROC_TYPE proc,
				unsigned int addr_val)
{
	return pal_check_rpu_mcu_regions(proc, addr_val);
}


static bool hal_rpu_is_mem_core_indirect(enum RPU_PROC_TYPE proc,
				unsigned int addr_val)
{
	return ((addr_val & 0xFF000000) == RPU_MCU_CORE_INDIRECT_BASE);
}


static bool hal_rpu_is_mem_readable(enum RPU_PROC_TYPE proc, unsigned int addr)
{
	return hal_rpu_is_mem_ram(proc, addr);
}


static bool hal_rpu_is_mem_writable(enum RPU_PROC_TYPE proc,
				    unsigned int addr)
{
	if (hal_rpu_is_mem_ram(proc, addr) ||
	    hal_rpu_is_mem_core_indirect(proc, addr) ||
	    hal_rpu_is_mem_core_direct(proc, addr) ||
	    hal_rpu_is_mem_bev(addr)) {
		return true;
	}

	return false;
}


static enum nrf_wifi_status rpu_mem_read_ram(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					     void *src_addr,
					     unsigned int ram_addr_val,
					     unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned long addr_offset = 0;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	unsigned long flags = 0;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = pal_rpu_addr_offset_get(hal_dev_ctx->hpriv->opriv,
					 ram_addr_val,
					 &addr_offset,
					 hal_dev_ctx->curr_proc);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: pal_rpu_addr_offset_get failed\n",
				      __func__);
		return status;
	}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->hpriv->opriv,
					hal_dev_ctx->rpu_ps_lock,
					&flags);

	status = hal_rpu_ps_wake(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: RPU wake failed\n",
				      __func__);
		goto out;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	nrf_wifi_bal_read_block(hal_dev_ctx->bal_dev_ctx,
				src_addr,
				addr_offset,
				len);

	status = NRF_WIFI_STATUS_SUCCESS;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
out:
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	return status;
}


static enum nrf_wifi_status rpu_mem_write_ram(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					      unsigned int ram_addr_val,
					      void *src_addr,
					      unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned long addr_offset = 0;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	unsigned long flags = 0;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = pal_rpu_addr_offset_get(hal_dev_ctx->hpriv->opriv,
					 ram_addr_val,
					 &addr_offset,
					 hal_dev_ctx->curr_proc);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: pal_rpu_addr_offset_get failed\n",
				      __func__);
		return status;
	}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->hpriv->opriv,
					hal_dev_ctx->rpu_ps_lock,
					&flags);

	status = hal_rpu_ps_wake(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: RPU wake failed\n",
				      __func__);
		goto out;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	nrf_wifi_bal_write_block(hal_dev_ctx->bal_dev_ctx,
				 addr_offset,
				 src_addr,
				 len);

	status = NRF_WIFI_STATUS_SUCCESS;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
out:
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	return status;
}


static enum nrf_wifi_status rpu_mem_write_core(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					       unsigned int core_addr_val,
					       void *src_addr,
					       unsigned int len)
{
	int status = NRF_WIFI_STATUS_FAIL;
	unsigned int addr_reg = 0;
	unsigned int data_reg = 0;
	unsigned int addr = 0;
	unsigned int data = 0;
	unsigned int i = 0;

	/* The RPU core address is expected to be in multiples of 4 bytes (word
	 * size). If not then something is amiss.
	 */
	if (!hal_rpu_is_mem_core_indirect(hal_dev_ctx->curr_proc,
				 core_addr_val)) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid memory address\n",
				      __func__);
		goto out;
	}

	if (core_addr_val % 4 != 0) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Address not multiple of 4 bytes\n",
				      __func__);
		goto out;
	}

	/* The register expects the word address offset to be programmed
	 * (i.e. it will write 4 bytes at once to the given address).
	 * whereas we receive the address in byte address offset.
	 */
	addr = (core_addr_val & RPU_ADDR_MASK_OFFSET) / 4;

	addr_reg = RPU_REG_MIPS_MCU_SYS_CORE_MEM_CTRL;
	data_reg = RPU_REG_MIPS_MCU_SYS_CORE_MEM_WDATA;

	if (hal_dev_ctx->curr_proc == RPU_PROC_TYPE_MCU_UMAC) {
		addr_reg = RPU_REG_MIPS_MCU2_SYS_CORE_MEM_CTRL;
		data_reg = RPU_REG_MIPS_MCU2_SYS_CORE_MEM_WDATA;
	}

	status = hal_rpu_reg_write(hal_dev_ctx,
				   addr_reg,
				   addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Writing to address reg failed\n",
				      __func__);
		goto out;
	}

	for (i = 0; i < (len / sizeof(int)); i++) {
		data = *((unsigned int *)src_addr + i);

		status = hal_rpu_reg_write(hal_dev_ctx,
					   data_reg,
					   data);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Writing to data reg failed\n",
					      __func__);
			goto out;
		}
	}
out:
	return status;
}


static unsigned int rpu_get_bev_addr_remap(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					   unsigned int bev_addr_val)
{
	unsigned int addr = 0;
	unsigned int offset = 0;

	offset = bev_addr_val & RPU_ADDR_MASK_BEV_OFFSET;

	/* Base of the Boot Exception Vector 0xBFC00000 maps to 0xA4000050 */
	addr = RPU_REG_MIPS_MCU_BOOT_EXCP_INSTR_0 + offset;

	if (hal_dev_ctx->curr_proc == RPU_PROC_TYPE_MCU_UMAC) {
		addr = RPU_REG_MIPS_MCU2_BOOT_EXCP_INSTR_0 + offset;
	}

	return addr;
}


static enum nrf_wifi_status rpu_mem_write_bev(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
					      unsigned int bev_addr_val,
					      void *src_addr,
					      unsigned int len)
{
	int status = NRF_WIFI_STATUS_FAIL;
	unsigned int addr = 0;
	unsigned int data = 0;
	unsigned int i = 0;

	/* The RPU BEV address is expected to be in multiples of 4 bytes (word
	 * size). If not then something is amiss.
	 */
	if ((bev_addr_val < RPU_ADDR_BEV_START) ||
	    (bev_addr_val > RPU_ADDR_BEV_END) ||
	    (bev_addr_val % 4 != 0)) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Address not in range or not a multiple of 4 bytes\n",
				      __func__);
		goto out;
	}

	for (i = 0; i < (len / sizeof(int)); i++) {
		/* The BEV addresses need remapping
		 * to an address on the SYSBUS.
		 */
		addr = rpu_get_bev_addr_remap(hal_dev_ctx,
					      bev_addr_val +
					      (i * sizeof(int)));

		data = *((unsigned int *)src_addr + i);

		status = hal_rpu_reg_write(hal_dev_ctx,
					   addr,
					   data);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Writing to BEV reg failed\n",
					      __func__);
			goto out;
		}
	}

out:
	return status;
}


enum nrf_wifi_status hal_rpu_mem_read(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				      void *src_addr,
				      unsigned int rpu_mem_addr_val,
				      unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!hal_dev_ctx) {
		goto out;
	}

	if (!src_addr) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		goto out;
	}

	if (!hal_rpu_is_mem_readable(hal_dev_ctx->curr_proc, rpu_mem_addr_val)) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid memory address 0x%X\n",
				      __func__,
				      rpu_mem_addr_val);
		goto out;
	}

	status = rpu_mem_read_ram(hal_dev_ctx,
				  src_addr,
				  rpu_mem_addr_val,
				  len);
out:
	return status;
}


enum nrf_wifi_status hal_rpu_mem_write(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				       unsigned int rpu_mem_addr_val,
				       void *src_addr,
				       unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	if (!hal_dev_ctx) {
		return status;
	}

	if (!src_addr) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		return status;
	}

	if (!hal_rpu_is_mem_writable(hal_dev_ctx->curr_proc,
				     rpu_mem_addr_val)) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid memory address 0x%X\n",
				      __func__,
				      rpu_mem_addr_val);
		return status;
	}

	if (hal_rpu_is_mem_core_indirect(hal_dev_ctx->curr_proc,
				rpu_mem_addr_val)) {
		status = rpu_mem_write_core(hal_dev_ctx,
					    rpu_mem_addr_val,
					    src_addr,
					    len);
	} else if (hal_rpu_is_mem_core_direct(hal_dev_ctx->curr_proc, rpu_mem_addr_val) ||
		hal_rpu_is_mem_ram(hal_dev_ctx->curr_proc, rpu_mem_addr_val)) {
		status = rpu_mem_write_ram(hal_dev_ctx,
					   rpu_mem_addr_val,
					   src_addr,
					   len);
	} else if (hal_rpu_is_mem_bev(rpu_mem_addr_val)) {
		status = rpu_mem_write_bev(hal_dev_ctx,
					   rpu_mem_addr_val,
					   src_addr,
					   len);
	} else {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid memory address 0x%X\n",
				      __func__,
				      rpu_mem_addr_val);
		goto out;
	}

out:
	return status;
}


enum nrf_wifi_status hal_rpu_mem_clr(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				     enum RPU_PROC_TYPE proc,
				     enum HAL_RPU_MEM_TYPE mem_type)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned int mem_addr = 0;
	unsigned int start_addr = 0;
	unsigned int end_addr = 0;
	unsigned int mem_val = 0;
	enum RPU_MCU_ADDR_REGIONS mcu_region = pal_mem_type_to_region(mem_type);

	if (!hal_dev_ctx) {
		goto out;
	}

	if (mem_type == HAL_RPU_MEM_TYPE_GRAM) {
		start_addr = RPU_ADDR_GRAM_START;
		end_addr = RPU_ADDR_GRAM_END;
	} else if (mem_type == HAL_RPU_MEM_TYPE_PKTRAM) {
		start_addr = RPU_ADDR_PKTRAM_START;
		end_addr = RPU_ADDR_PKTRAM_END;
	} else if (mcu_region != RPU_MCU_ADDR_REGION_MAX) {
		const struct rpu_addr_map *map = &RPU_ADDR_MAP_MCU[proc];
		const struct rpu_addr_region *region = &map->regions[mcu_region];

		start_addr = region->start;
		end_addr = region->end;
	} else {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
			"%s: Invalid mem_type(%d)\n",
			__func__,
			mem_type);
		goto out;
	}

	for (mem_addr = start_addr;
	     mem_addr <= end_addr;
	     mem_addr += sizeof(mem_val)) {
		status = hal_rpu_mem_write(hal_dev_ctx,
					   mem_addr,
					   &mem_val,
					   sizeof(mem_val));

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: hal_rpu_mem_write failed\n",
					      __func__);
			goto out;
		}
	}



	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

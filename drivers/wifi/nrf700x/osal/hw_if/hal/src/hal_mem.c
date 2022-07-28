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


static bool hal_rpu_is_mem_ram(unsigned int addr_val)
{
	if (((addr_val >= RPU_ADDR_GRAM_START) &&
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


static bool hal_rpu_is_mem_core(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
				unsigned int addr_val)
{
	bool is_mem = false;

	if (hal_dev_ctx->curr_proc == RPU_PROC_TYPE_MCU_LMAC) {
		if (((addr_val >= RPU_ADDR_MCU1_CORE_ROM_START) &&
		     (addr_val <= RPU_ADDR_MCU1_CORE_ROM_END)) ||
		    ((addr_val >= RPU_ADDR_MCU1_CORE_RET_START) &&
		     (addr_val <= RPU_ADDR_MCU1_CORE_RET_END)) ||
		    ((addr_val >= RPU_ADDR_MCU1_CORE_SCRATCH_START) &&
		     (addr_val <= RPU_ADDR_MCU1_CORE_SCRATCH_END))) {
			is_mem = true;
		}
	} else if (hal_dev_ctx->curr_proc == RPU_PROC_TYPE_MCU_UMAC) {
		if (((addr_val >= RPU_ADDR_MCU2_CORE_ROM_START) &&
		     (addr_val <= RPU_ADDR_MCU2_CORE_ROM_END)) ||
		    ((addr_val >= RPU_ADDR_MCU2_CORE_RET_START) &&
		     (addr_val <= RPU_ADDR_MCU2_CORE_RET_END)) ||
		    ((addr_val >= RPU_ADDR_MCU2_CORE_SCRATCH_START) &&
		     (addr_val <= RPU_ADDR_MCU2_CORE_SCRATCH_END))) {
			is_mem = true;
		}
	}

	return is_mem;
}


static bool hal_rpu_is_mem_readable(unsigned int addr)
{
	if (hal_rpu_is_mem_ram(addr)) {
		return true;
	}

	return false;
}


static bool hal_rpu_is_mem_writable(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
				    unsigned int addr)
{
	if (hal_rpu_is_mem_ram(addr) ||
	    hal_rpu_is_mem_core(hal_dev_ctx, addr) ||
	    hal_rpu_is_mem_bev(addr)) {
		return true;
	}

	return false;
}


static enum wifi_nrf_status rpu_mem_read_ram(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
					     void *src_addr,
					     unsigned int ram_addr_val,
					     unsigned int len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned long addr_offset = 0;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	unsigned long flags = 0;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = pal_rpu_addr_offset_get(hal_dev_ctx->hpriv->opriv,
					 ram_addr_val,
					 &addr_offset);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: pal_rpu_addr_offset_get failed\n",
				      __func__);
		return status;
	}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	wifi_nrf_osal_spinlock_irq_take(hal_dev_ctx->hpriv->opriv,
					hal_dev_ctx->rpu_ps_lock,
					&flags);

	status = hal_rpu_ps_wake(hal_dev_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: RPU wake failed\n",
				      __func__);
		goto out;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	wifi_nrf_bal_read_block(hal_dev_ctx->bal_dev_ctx,
				src_addr,
				addr_offset,
				len);

	status = WIFI_NRF_STATUS_SUCCESS;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
out:
	wifi_nrf_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	return status;
}


static enum wifi_nrf_status rpu_mem_write_ram(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
					      unsigned int ram_addr_val,
					      void *src_addr,
					      unsigned int len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned long addr_offset = 0;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	unsigned long flags = 0;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = pal_rpu_addr_offset_get(hal_dev_ctx->hpriv->opriv,
					 ram_addr_val,
					 &addr_offset);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: pal_rpu_addr_offset_get failed\n",
				      __func__);
		return status;
	}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	wifi_nrf_osal_spinlock_irq_take(hal_dev_ctx->hpriv->opriv,
					hal_dev_ctx->rpu_ps_lock,
					&flags);

	status = hal_rpu_ps_wake(hal_dev_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: RPU wake failed\n",
				      __func__);
		goto out;
	}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	wifi_nrf_bal_write_block(hal_dev_ctx->bal_dev_ctx,
				 addr_offset,
				 src_addr,
				 len);

	status = WIFI_NRF_STATUS_SUCCESS;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
out:
	wifi_nrf_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	return status;
}


static enum wifi_nrf_status rpu_mem_write_core(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
					       unsigned int core_addr_val,
					       void *src_addr,
					       unsigned int len)
{
	int status = WIFI_NRF_STATUS_FAIL;
	unsigned int addr_reg = 0;
	unsigned int data_reg = 0;
	unsigned int addr = 0;
	unsigned int data = 0;
	unsigned int i = 0;

	/* The RPU core address is expected to be in multiples of 4 bytes (word
	 * size). If not then something is amiss.
	 */
	if (!hal_rpu_is_mem_core(hal_dev_ctx,
				 core_addr_val)) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid memory address\n",
				      __func__);
		goto out;
	}

	if (core_addr_val % 4 != 0) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
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

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Writing to address reg failed\n",
				      __func__);
		goto out;
	}

	for (i = 0; i < (len / sizeof(int)); i++) {
		data = *((unsigned int *)src_addr + i);

		status = hal_rpu_reg_write(hal_dev_ctx,
					   data_reg,
					   data);

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Writing to data reg failed\n",
					      __func__);
			goto out;
		}
	}
out:
	return status;
}


static unsigned int rpu_get_bev_addr_remap(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
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


static enum wifi_nrf_status rpu_mem_write_bev(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
					      unsigned int bev_addr_val,
					      void *src_addr,
					      unsigned int len)
{
	int status = WIFI_NRF_STATUS_FAIL;
	unsigned int addr = 0;
	unsigned int data = 0;
	unsigned int i = 0;

	/* The RPU BEV address is expected to be in multiples of 4 bytes (word
	 * size). If not then something is amiss.
	 */
	if ((bev_addr_val < RPU_ADDR_BEV_START) ||
	    (bev_addr_val > RPU_ADDR_BEV_END) ||
	    (bev_addr_val % 4 != 0)) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
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

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Writing to BEV reg failed\n",
					      __func__);
			goto out;
		}
	}

out:
	return status;
}


enum wifi_nrf_status hal_rpu_mem_read(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
				      void *src_addr,
				      unsigned int rpu_mem_addr_val,
				      unsigned int len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	if (!hal_dev_ctx) {
		goto out;
	}

	if (!src_addr) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		goto out;
	}

	if (!hal_rpu_is_mem_readable(rpu_mem_addr_val)) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
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


enum wifi_nrf_status hal_rpu_mem_write(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
				       unsigned int rpu_mem_addr_val,
				       void *src_addr,
				       unsigned int len)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	if (!hal_dev_ctx) {
		return status;
	}

	if (!src_addr) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid params\n",
				      __func__);
		return status;
	}

	if (!hal_rpu_is_mem_writable(hal_dev_ctx,
				     rpu_mem_addr_val)) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid memory address 0x%X\n",
				      __func__,
				      rpu_mem_addr_val);
		return status;
	}

	if (hal_rpu_is_mem_core(hal_dev_ctx,
				rpu_mem_addr_val)) {
		status = rpu_mem_write_core(hal_dev_ctx,
					    rpu_mem_addr_val,
					    src_addr,
					    len);
	} else if (hal_rpu_is_mem_ram(rpu_mem_addr_val)) {
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
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid memory address 0x%X\n",
				      __func__,
				      rpu_mem_addr_val);
		goto out;
	}

out:
	return status;
}


enum wifi_nrf_status hal_rpu_mem_clr(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
				     enum RPU_PROC_TYPE proc,
				     enum HAL_RPU_MEM_TYPE mem_type)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned int mem_addr = 0;
	unsigned int start_addr = 0;
	unsigned int end_addr = 0;
	unsigned int mem_val = 0;

	if (!hal_dev_ctx) {
		goto out;
	}

	if (mem_type == HAL_RPU_MEM_TYPE_GRAM) {
		start_addr = RPU_ADDR_GRAM_START;
		end_addr = RPU_ADDR_GRAM_END;
	} else if (mem_type == HAL_RPU_MEM_TYPE_PKTRAM) {
		start_addr = RPU_ADDR_PKTRAM_START;
		end_addr = RPU_ADDR_PKTRAM_END;
	} else if (mem_type == HAL_RPU_MEM_TYPE_CORE_ROM) {
		if (proc == RPU_PROC_TYPE_MCU_LMAC) {
			start_addr = RPU_ADDR_MCU1_CORE_ROM_START;
			end_addr = RPU_ADDR_MCU1_CORE_ROM_END;
		} else if (proc == RPU_PROC_TYPE_MCU_UMAC) {
			start_addr = RPU_ADDR_MCU2_CORE_ROM_START;
			end_addr = RPU_ADDR_MCU2_CORE_ROM_END;
		} else {
			wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Invalid proc(%d)\n",
					      __func__,
					      hal_dev_ctx->curr_proc);
			goto out;
		}
	} else if (mem_type == HAL_RPU_MEM_TYPE_CORE_RET) {
		if (proc == RPU_PROC_TYPE_MCU_LMAC) {
			start_addr = RPU_ADDR_MCU1_CORE_RET_START;
			end_addr = RPU_ADDR_MCU1_CORE_RET_END;
		} else if (proc == RPU_PROC_TYPE_MCU_UMAC) {
			start_addr = RPU_ADDR_MCU2_CORE_RET_START;
			end_addr = RPU_ADDR_MCU2_CORE_RET_END;
		} else {
			wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Invalid proc(%d)\n",
					      __func__,
					      hal_dev_ctx->curr_proc);
			goto out;
		}
	} else if (mem_type == HAL_RPU_MEM_TYPE_CORE_SCRATCH) {
		if (proc == RPU_PROC_TYPE_MCU_LMAC) {
			start_addr = RPU_ADDR_MCU1_CORE_SCRATCH_START;
			end_addr = RPU_ADDR_MCU1_CORE_SCRATCH_END;
		} else if (proc == RPU_PROC_TYPE_MCU_UMAC) {
			start_addr = RPU_ADDR_MCU2_CORE_SCRATCH_START;
			end_addr = RPU_ADDR_MCU2_CORE_SCRATCH_END;
		} else {
			wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: Invalid proc(%d)\n",
					      __func__,
					      hal_dev_ctx->curr_proc);
			goto out;
		}
	} else {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
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

		if (status != WIFI_NRF_STATUS_SUCCESS) {
			wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
					      "%s: hal_rpu_mem_write failed\n",
					      __func__);
			goto out;
		}
	}

	status = WIFI_NRF_STATUS_SUCCESS;
out:
	return status;
}

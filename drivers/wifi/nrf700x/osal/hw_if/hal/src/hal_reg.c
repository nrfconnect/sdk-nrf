/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing register read/write specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "pal.h"
#include "hal_api.h"
#include "hal_common.h"


static bool hal_rpu_is_reg(unsigned int addr_val)
{
	unsigned int addr_base = (addr_val & RPU_ADDR_MASK_BASE);

	if ((addr_base == RPU_ADDR_SBUS_START) ||
	    (addr_base == RPU_ADDR_PBUS_START)) {
		return true;
	} else {
		return false;
	}
}


enum wifi_nrf_status hal_rpu_reg_read(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
				      unsigned int *val,
				      unsigned int rpu_reg_addr)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned long addr_offset = 0;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	unsigned long flags = 0;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	if (!hal_dev_ctx) {
		return status;
	}

	if ((val == NULL) ||
	    !hal_rpu_is_reg(rpu_reg_addr)) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid params, val = %p, rpu_reg (0x%x)\n",
				      __func__,
				      val,
				      rpu_reg_addr);
		return status;
	}

	status = pal_rpu_addr_offset_get(hal_dev_ctx->hpriv->opriv,
					 rpu_reg_addr,
					 &addr_offset,
					 hal_dev_ctx->curr_proc);

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

	*val = wifi_nrf_bal_read_word(hal_dev_ctx->bal_dev_ctx,
				      addr_offset);

	if (*val == 0xFFFFFFFF) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Error !! Value read at addr_offset = %lx is = %X\n",
				      __func__,
				      addr_offset,
				      *val);
		status = WIFI_NRF_STATUS_FAIL;
		goto out;
	}

	status = WIFI_NRF_STATUS_SUCCESS;
out:
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	wifi_nrf_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	return status;
}

enum wifi_nrf_status hal_rpu_reg_write(struct wifi_nrf_hal_dev_ctx *hal_dev_ctx,
				       unsigned int rpu_reg_addr,
				       unsigned int val)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	unsigned long addr_offset = 0;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	unsigned long flags = 0;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	if (!hal_dev_ctx) {
		return status;
	}

	if (!hal_rpu_is_reg(rpu_reg_addr)) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid params, rpu_reg_addr (0x%X)\n",
				      __func__,
				      rpu_reg_addr);
		return status;
	}

	status = pal_rpu_addr_offset_get(hal_dev_ctx->hpriv->opriv,
					 rpu_reg_addr,
					 &addr_offset,
					 hal_dev_ctx->curr_proc);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: pal_rpu_get_region_offset failed\n",
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

	wifi_nrf_bal_write_word(hal_dev_ctx->bal_dev_ctx,
				addr_offset,
				val);

	status = WIFI_NRF_STATUS_SUCCESS;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
out:
	wifi_nrf_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	return status;
}

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


enum nrf_wifi_status hal_rpu_reg_read(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				      unsigned int *val,
				      unsigned int rpu_reg_addr)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned long addr_offset = 0;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	unsigned long flags = 0;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	if (!hal_dev_ctx) {
		return status;
	}

	if ((val == NULL) ||
	    !hal_rpu_is_reg(rpu_reg_addr)) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
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

	*val = nrf_wifi_bal_read_word(hal_dev_ctx->bal_dev_ctx,
				      addr_offset);

	if (*val == 0xFFFFFFFF) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Error !! Value read at addr_offset = %lx is = %X\n",
				      __func__,
				      addr_offset,
				      *val);
		status = NRF_WIFI_STATUS_FAIL;
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	return status;
}

enum nrf_wifi_status hal_rpu_reg_write(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
				       unsigned int rpu_reg_addr,
				       unsigned int val)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	unsigned long addr_offset = 0;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	unsigned long flags = 0;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	if (!hal_dev_ctx) {
		return status;
	}

	if (!hal_rpu_is_reg(rpu_reg_addr)) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: Invalid params, rpu_reg_addr (0x%X)\n",
				      __func__,
				      rpu_reg_addr);
		return status;
	}

	status = pal_rpu_addr_offset_get(hal_dev_ctx->hpriv->opriv,
					 rpu_reg_addr,
					 &addr_offset,
					 hal_dev_ctx->curr_proc);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_dev_ctx->hpriv->opriv,
				      "%s: pal_rpu_get_region_offset failed\n",
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

	nrf_wifi_bal_write_word(hal_dev_ctx->bal_dev_ctx,
				addr_offset,
				val);

	status = NRF_WIFI_STATUS_SUCCESS;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
out:
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->hpriv->opriv,
				       hal_dev_ctx->rpu_ps_lock,
				       &flags);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	return status;
}

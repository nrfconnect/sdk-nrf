/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief API definitions for the Bus Abstraction Layer (BAL) of the Wi-Fi
 * driver.
 */


#include "bal_api.h"

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
#include "pal.h"

static void wifi_nrf_rpu_bal_sleep_chk(struct wifi_nrf_bal_dev_ctx *bal_ctx,
				       unsigned long addr)
{
	unsigned int sleep_reg_val = 0;
	unsigned int rpu_ps_state_mask = 0;
	unsigned long sleep_reg_addr = 0;

	if (!bal_ctx->rpu_fw_booted)
		return;

	sleep_reg_addr = pal_rpu_ps_ctrl_reg_addr_get();

	if (sleep_reg_addr == addr)
		return;

	sleep_reg_val = bal_ctx->bpriv->ops->read_word(bal_ctx->bus_dev_ctx,
						       sleep_reg_addr);

	rpu_ps_state_mask = ((1 << RPU_REG_BIT_PS_STATE) |
			     (1 << RPU_REG_BIT_READY_STATE));

	if ((sleep_reg_val & rpu_ps_state_mask) != rpu_ps_state_mask) {
		wifi_nrf_osal_log_err(bal_ctx->bpriv->opriv,
				      "%s:RPU is being accessed when it is not ready !!! (Reg val = 0x%X)\n",
				      __func__,
				      sleep_reg_val);
	}
}
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */


struct wifi_nrf_bal_dev_ctx *wifi_nrf_bal_dev_add(struct wifi_nrf_bal_priv *bpriv,
						  void *hal_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = wifi_nrf_osal_mem_zalloc(bpriv->opriv,
					       sizeof(*bal_dev_ctx));

	if (!bal_dev_ctx) {
		wifi_nrf_osal_log_err(bpriv->opriv,
				      "%s: Unable to allocate bal_dev_ctx\n", __func__);
		goto out;
	}

	bal_dev_ctx->bpriv = bpriv;
	bal_dev_ctx->hal_dev_ctx = hal_dev_ctx;

	bal_dev_ctx->bus_dev_ctx = bpriv->ops->dev_add(bpriv->bus_priv,
						       bal_dev_ctx);

	if (!bal_dev_ctx->bus_dev_ctx) {
		wifi_nrf_osal_log_err(bpriv->opriv,
				      "%s: Bus dev_add failed\n", __func__);
		goto out;
	}

	status = WIFI_NRF_STATUS_SUCCESS;
out:
	if (status != WIFI_NRF_STATUS_SUCCESS) {
		if (bal_dev_ctx) {
			wifi_nrf_osal_mem_free(bpriv->opriv,
					       bal_dev_ctx);
			bal_dev_ctx = NULL;
		}
	}

	return bal_dev_ctx;
}


void wifi_nrf_bal_dev_rem(struct wifi_nrf_bal_dev_ctx *bal_dev_ctx)
{
	bal_dev_ctx->bpriv->ops->dev_rem(bal_dev_ctx->bus_dev_ctx);

	wifi_nrf_osal_mem_free(bal_dev_ctx->bpriv->opriv,
			       bal_dev_ctx);
}


enum wifi_nrf_status wifi_nrf_bal_dev_init(struct wifi_nrf_bal_dev_ctx *bal_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	bal_dev_ctx->rpu_fw_booted = true;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = bal_dev_ctx->bpriv->ops->dev_init(bal_dev_ctx->bus_dev_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(bal_dev_ctx->bpriv->opriv,
				      "%s: dev_init failed\n", __func__);
		goto out;
	}
out:
	return status;
}


void wifi_nrf_bal_dev_deinit(struct wifi_nrf_bal_dev_ctx *bal_dev_ctx)
{
	bal_dev_ctx->bpriv->ops->dev_deinit(bal_dev_ctx->bus_dev_ctx);
}


static enum wifi_nrf_status wifi_nrf_bal_isr(void *ctx)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

	status = bal_dev_ctx->bpriv->intr_callbk_fn(bal_dev_ctx->hal_dev_ctx);

	return status;
}


struct wifi_nrf_bal_priv *
wifi_nrf_bal_init(struct wifi_nrf_osal_priv *opriv,
		  struct wifi_nrf_bal_cfg_params *cfg_params,
		  enum wifi_nrf_status (*intr_callbk_fn)(void *hal_dev_ctx))
{
	struct wifi_nrf_bal_priv *bpriv = NULL;

	bpriv = wifi_nrf_osal_mem_zalloc(opriv,
					 sizeof(*bpriv));

	if (!bpriv) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to allocate memory for bpriv\n", __func__);
		goto out;
	}

	bpriv->opriv = opriv;

	bpriv->intr_callbk_fn = intr_callbk_fn;

	bpriv->ops = get_bus_ops();

	bpriv->bus_priv = bpriv->ops->init(opriv,
					   cfg_params,
					   &wifi_nrf_bal_isr);

	if (!bpriv->bus_priv) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Failed\n", __func__);
		wifi_nrf_osal_mem_free(opriv,
				       bpriv);
		bpriv = NULL;
	}

out:
	return bpriv;
}


void wifi_nrf_bal_deinit(struct wifi_nrf_bal_priv *bpriv)
{
	bpriv->ops->deinit(bpriv->bus_priv);

	wifi_nrf_osal_mem_free(bpriv->opriv,
			       bpriv);
}


unsigned int wifi_nrf_bal_read_word(void *ctx,
				    unsigned long addr_offset)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;
	unsigned int val = 0;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	wifi_nrf_rpu_bal_sleep_chk(bal_dev_ctx,
				   addr_offset);
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */

	val = bal_dev_ctx->bpriv->ops->read_word(bal_dev_ctx->bus_dev_ctx,
						 addr_offset);

	return val;
}


void wifi_nrf_bal_write_word(void *ctx,
			     unsigned long addr_offset,
			     unsigned int val)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	wifi_nrf_rpu_bal_sleep_chk(bal_dev_ctx,
				   addr_offset);
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */

	bal_dev_ctx->bpriv->ops->write_word(bal_dev_ctx->bus_dev_ctx,
					    addr_offset,
					    val);
}


void wifi_nrf_bal_read_block(void *ctx,
			     void *dest_addr,
			     unsigned long src_addr_offset,
			     size_t len)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	wifi_nrf_rpu_bal_sleep_chk(bal_dev_ctx,
				   src_addr_offset);
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */

	bal_dev_ctx->bpriv->ops->read_block(bal_dev_ctx->bus_dev_ctx,
					    dest_addr,
					    src_addr_offset,
					    len);
}


void wifi_nrf_bal_write_block(void *ctx,
			      unsigned long dest_addr_offset,
			      const void *src_addr,
			      size_t len)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	wifi_nrf_rpu_bal_sleep_chk(bal_dev_ctx,
				   dest_addr_offset);
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */

	bal_dev_ctx->bpriv->ops->write_block(bal_dev_ctx->bus_dev_ctx,
					     dest_addr_offset,
					     src_addr,
					     len);
}


unsigned long wifi_nrf_bal_dma_map(void *ctx,
				   unsigned long virt_addr,
				   size_t len,
				   enum wifi_nrf_osal_dma_dir dma_dir)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;
	unsigned long phy_addr = 0;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

	phy_addr = bal_dev_ctx->bpriv->ops->dma_map(bal_dev_ctx->bus_dev_ctx,
						    virt_addr,
						    len,
						    dma_dir);

	return phy_addr;
}


unsigned long wifi_nrf_bal_dma_unmap(void *ctx,
				     unsigned long phy_addr,
				     size_t len,
				     enum wifi_nrf_osal_dma_dir dma_dir)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;
	unsigned long virt_addr = 0;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

	virt_addr = bal_dev_ctx->bpriv->ops->dma_unmap(bal_dev_ctx->bus_dev_ctx,
						       phy_addr,
						       len,
						       dma_dir);

	return virt_addr;
}


#ifdef CONFIG_NRF_WIFI_LOW_POWER
void wifi_nrf_bal_rpu_ps_sleep(void *ctx)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

	bal_dev_ctx->bpriv->ops->rpu_ps_sleep(bal_dev_ctx->bus_dev_ctx);
}


void wifi_nrf_bal_rpu_ps_wake(void *ctx)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

	bal_dev_ctx->bpriv->ops->rpu_ps_wake(bal_dev_ctx->bus_dev_ctx);
}


int wifi_nrf_bal_rpu_ps_status(void *ctx)
{
	struct wifi_nrf_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct wifi_nrf_bal_dev_ctx *)ctx;

	return bal_dev_ctx->bpriv->ops->rpu_ps_status(bal_dev_ctx->bus_dev_ctx);
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

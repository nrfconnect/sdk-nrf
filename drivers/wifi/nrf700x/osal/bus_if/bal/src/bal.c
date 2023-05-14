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

static void nrf_wifi_rpu_bal_sleep_chk(struct nrf_wifi_bal_dev_ctx *bal_ctx,
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
		nrf_wifi_osal_log_err(bal_ctx->bpriv->opriv,
				      "%s:RPU is being accessed when it is not ready !!! (Reg val = 0x%X)\n",
				      __func__,
				      sleep_reg_val);
	}
}
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */


struct nrf_wifi_bal_dev_ctx *nrf_wifi_bal_dev_add(struct nrf_wifi_bal_priv *bpriv,
						  void *hal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = nrf_wifi_osal_mem_zalloc(bpriv->opriv,
					       sizeof(*bal_dev_ctx));

	if (!bal_dev_ctx) {
		nrf_wifi_osal_log_err(bpriv->opriv,
				      "%s: Unable to allocate bal_dev_ctx\n", __func__);
		goto out;
	}

	bal_dev_ctx->bpriv = bpriv;
	bal_dev_ctx->hal_dev_ctx = hal_dev_ctx;

	bal_dev_ctx->bus_dev_ctx = bpriv->ops->dev_add(bpriv->bus_priv,
						       bal_dev_ctx);

	if (!bal_dev_ctx->bus_dev_ctx) {
		nrf_wifi_osal_log_err(bpriv->opriv,
				      "%s: Bus dev_add failed\n", __func__);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		if (bal_dev_ctx) {
			nrf_wifi_osal_mem_free(bpriv->opriv,
					       bal_dev_ctx);
			bal_dev_ctx = NULL;
		}
	}

	return bal_dev_ctx;
}


void nrf_wifi_bal_dev_rem(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx)
{
	bal_dev_ctx->bpriv->ops->dev_rem(bal_dev_ctx->bus_dev_ctx);

	nrf_wifi_osal_mem_free(bal_dev_ctx->bpriv->opriv,
			       bal_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_bal_dev_init(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	bal_dev_ctx->rpu_fw_booted = true;
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	status = bal_dev_ctx->bpriv->ops->dev_init(bal_dev_ctx->bus_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(bal_dev_ctx->bpriv->opriv,
				      "%s: dev_init failed\n", __func__);
		goto out;
	}
out:
	return status;
}


void nrf_wifi_bal_dev_deinit(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx)
{
	bal_dev_ctx->bpriv->ops->dev_deinit(bal_dev_ctx->bus_dev_ctx);
}


static enum nrf_wifi_status nrf_wifi_bal_isr(void *ctx)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

	status = bal_dev_ctx->bpriv->intr_callbk_fn(bal_dev_ctx->hal_dev_ctx);

	return status;
}


struct nrf_wifi_bal_priv *
nrf_wifi_bal_init(struct nrf_wifi_osal_priv *opriv,
		  struct nrf_wifi_bal_cfg_params *cfg_params,
		  enum nrf_wifi_status (*intr_callbk_fn)(void *hal_dev_ctx))
{
	struct nrf_wifi_bal_priv *bpriv = NULL;

	bpriv = nrf_wifi_osal_mem_zalloc(opriv,
					 sizeof(*bpriv));

	if (!bpriv) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Unable to allocate memory for bpriv\n", __func__);
		goto out;
	}

	bpriv->opriv = opriv;

	bpriv->intr_callbk_fn = intr_callbk_fn;

	bpriv->ops = get_bus_ops();

	bpriv->bus_priv = bpriv->ops->init(opriv,
					   cfg_params,
					   &nrf_wifi_bal_isr);

	if (!bpriv->bus_priv) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Failed\n", __func__);
		nrf_wifi_osal_mem_free(opriv,
				       bpriv);
		bpriv = NULL;
	}

out:
	return bpriv;
}


void nrf_wifi_bal_deinit(struct nrf_wifi_bal_priv *bpriv)
{
	bpriv->ops->deinit(bpriv->bus_priv);

	nrf_wifi_osal_mem_free(bpriv->opriv,
			       bpriv);
}


unsigned int nrf_wifi_bal_read_word(void *ctx,
				    unsigned long addr_offset)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;
	unsigned int val = 0;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	nrf_wifi_rpu_bal_sleep_chk(bal_dev_ctx,
				   addr_offset);
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */

	val = bal_dev_ctx->bpriv->ops->read_word(bal_dev_ctx->bus_dev_ctx,
						 addr_offset);

	return val;
}


void nrf_wifi_bal_write_word(void *ctx,
			     unsigned long addr_offset,
			     unsigned int val)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	nrf_wifi_rpu_bal_sleep_chk(bal_dev_ctx,
				   addr_offset);
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */

	bal_dev_ctx->bpriv->ops->write_word(bal_dev_ctx->bus_dev_ctx,
					    addr_offset,
					    val);
}


void nrf_wifi_bal_read_block(void *ctx,
			     void *dest_addr,
			     unsigned long src_addr_offset,
			     size_t len)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	nrf_wifi_rpu_bal_sleep_chk(bal_dev_ctx,
				   src_addr_offset);
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */

	bal_dev_ctx->bpriv->ops->read_block(bal_dev_ctx->bus_dev_ctx,
					    dest_addr,
					    src_addr_offset,
					    len);
}


void nrf_wifi_bal_write_block(void *ctx,
			      unsigned long dest_addr_offset,
			      const void *src_addr,
			      size_t len)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

#ifdef CONFIG_NRF_WIFI_LOW_POWER
#ifdef CONFIG_NRF_WIFI_LOW_POWER_DBG
	nrf_wifi_rpu_bal_sleep_chk(bal_dev_ctx,
				   dest_addr_offset);
#endif	/* CONFIG_NRF_WIFI_LOW_POWER_DBG */
#endif  /* CONFIG_NRF_WIFI_LOW_POWER */

	bal_dev_ctx->bpriv->ops->write_block(bal_dev_ctx->bus_dev_ctx,
					     dest_addr_offset,
					     src_addr,
					     len);
}


unsigned long nrf_wifi_bal_dma_map(void *ctx,
				   unsigned long virt_addr,
				   size_t len,
				   enum nrf_wifi_osal_dma_dir dma_dir)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;
	unsigned long phy_addr = 0;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

	phy_addr = bal_dev_ctx->bpriv->ops->dma_map(bal_dev_ctx->bus_dev_ctx,
						    virt_addr,
						    len,
						    dma_dir);

	return phy_addr;
}


unsigned long nrf_wifi_bal_dma_unmap(void *ctx,
				     unsigned long phy_addr,
				     size_t len,
				     enum nrf_wifi_osal_dma_dir dma_dir)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;
	unsigned long virt_addr = 0;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

	virt_addr = bal_dev_ctx->bpriv->ops->dma_unmap(bal_dev_ctx->bus_dev_ctx,
						       phy_addr,
						       len,
						       dma_dir);

	return virt_addr;
}


#ifdef CONFIG_NRF_WIFI_LOW_POWER
void nrf_wifi_bal_rpu_ps_sleep(void *ctx)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

	bal_dev_ctx->bpriv->ops->rpu_ps_sleep(bal_dev_ctx->bus_dev_ctx);
}


void nrf_wifi_bal_rpu_ps_wake(void *ctx)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

	bal_dev_ctx->bpriv->ops->rpu_ps_wake(bal_dev_ctx->bus_dev_ctx);
}


int nrf_wifi_bal_rpu_ps_status(void *ctx)
{
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;

	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)ctx;

	return bal_dev_ctx->bpriv->ops->rpu_ps_status(bal_dev_ctx->bus_dev_ctx);
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

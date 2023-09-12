/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing QSPI Bus Layer specific function definitions of the
 * Wi-Fi driver.
 */

#include "bal_structs.h"
#include "spi.h"
#include "pal.h"


static int wifi_nrf_bus_spi_irq_handler(void *data)
{
	struct wifi_nrf_bus_spi_dev_ctx *dev_ctx = NULL;
	struct wifi_nrf_bus_spi_priv *spi_priv = NULL;
	int ret = 0;

	dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)data;
	spi_priv = dev_ctx->spi_priv;

	ret = spi_priv->intr_callbk_fn(dev_ctx->bal_dev_ctx);

	return ret;
}


static void *wifi_nrf_bus_spi_dev_add(void *bus_priv,
				       void *bal_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_bus_spi_priv *spi_priv = NULL;
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;
	struct wifi_nrf_osal_host_map host_map;

	spi_priv = bus_priv;

	spi_dev_ctx = wifi_nrf_osal_mem_zalloc(spi_priv->opriv,
						sizeof(*spi_dev_ctx));

	if (!spi_dev_ctx) {
		wifi_nrf_osal_log_err(spi_priv->opriv,
				      "%s: Unable to allocate spi_dev_ctx\n", __func__);
		goto out;
	}

	spi_dev_ctx->spi_priv = spi_priv;
	spi_dev_ctx->bal_dev_ctx = bal_dev_ctx;

	spi_dev_ctx->os_spi_dev_ctx = wifi_nrf_osal_bus_spi_dev_add(spi_priv->opriv,
								       spi_priv->os_spi_priv,
								       spi_dev_ctx);

	if (!spi_dev_ctx->os_spi_dev_ctx) {
		wifi_nrf_osal_log_err(spi_priv->opriv,
				      "%s: wifi_nrf_osal_bus_spi_dev_add failed\n", __func__);

		wifi_nrf_osal_mem_free(spi_priv->opriv,
				       spi_dev_ctx);

		spi_dev_ctx = NULL;

		goto out;
	}

	wifi_nrf_osal_bus_spi_dev_host_map_get(spi_priv->opriv,
						spi_dev_ctx->os_spi_dev_ctx,
						&host_map);

	spi_dev_ctx->host_addr_base = host_map.addr;

	spi_dev_ctx->addr_pktram_base = spi_dev_ctx->host_addr_base +
		spi_priv->cfg_params.addr_pktram_base;

	status = wifi_nrf_osal_bus_spi_dev_intr_reg(spi_dev_ctx->spi_priv->opriv,
						     spi_dev_ctx->os_spi_dev_ctx,
						     spi_dev_ctx,
						     &wifi_nrf_bus_spi_irq_handler);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(spi_dev_ctx->spi_priv->opriv,
				      "%s: Unable to register interrupt to the OS\n",
				      __func__);

		wifi_nrf_osal_bus_spi_dev_intr_unreg(spi_dev_ctx->spi_priv->opriv,
						      spi_dev_ctx->os_spi_dev_ctx);

		wifi_nrf_osal_bus_spi_dev_rem(spi_dev_ctx->spi_priv->opriv,
					       spi_dev_ctx->os_spi_dev_ctx);

		wifi_nrf_osal_mem_free(spi_priv->opriv,
				       spi_dev_ctx);

		spi_dev_ctx = NULL;

		goto out;
	}

out:
	return spi_dev_ctx;
}


static void wifi_nrf_bus_spi_dev_rem(void *bus_dev_ctx)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = bus_dev_ctx;

	wifi_nrf_osal_bus_spi_dev_intr_unreg(spi_dev_ctx->spi_priv->opriv,
					      spi_dev_ctx->os_spi_dev_ctx);

	wifi_nrf_osal_mem_free(spi_dev_ctx->spi_priv->opriv,
			       spi_dev_ctx);
}


static enum wifi_nrf_status wifi_nrf_bus_spi_dev_init(void *bus_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = bus_dev_ctx;

	status = wifi_nrf_osal_bus_spi_dev_init(spi_dev_ctx->spi_priv->opriv,
						 spi_dev_ctx->os_spi_dev_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(spi_dev_ctx->spi_priv->opriv,
				      "%s: wifi_nrf_osal_spi_dev_init failed\n", __func__);

		goto out;
	}
out:
	return status;
}


static void wifi_nrf_bus_spi_dev_deinit(void *bus_dev_ctx)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = bus_dev_ctx;

	wifi_nrf_osal_bus_spi_dev_deinit(spi_dev_ctx->spi_priv->opriv,
					  spi_dev_ctx->os_spi_dev_ctx);
}


static void *wifi_nrf_bus_spi_init(struct wifi_nrf_osal_priv *opriv,
				    void *params,
				    enum wifi_nrf_status (*intr_callbk_fn)(void *bal_dev_ctx))
{
	struct wifi_nrf_bus_spi_priv *spi_priv = NULL;

	spi_priv = wifi_nrf_osal_mem_zalloc(opriv,
					     sizeof(*spi_priv));

	if (!spi_priv) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to allocate memory for spi_priv\n",
				      __func__);
		goto out;
	}

	spi_priv->opriv = opriv;

	wifi_nrf_osal_mem_cpy(opriv,
			      &spi_priv->cfg_params,
			      params,
			      sizeof(spi_priv->cfg_params));

	spi_priv->intr_callbk_fn = intr_callbk_fn;

	spi_priv->os_spi_priv = wifi_nrf_osal_bus_spi_init(opriv);
	if (!spi_priv->os_spi_priv) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to register QSPI driver\n",
				      __func__);

		wifi_nrf_osal_mem_free(opriv,
				       spi_priv);

		spi_priv = NULL;

		goto out;
	}
out:
	return spi_priv;
}


static void wifi_nrf_bus_spi_deinit(void *bus_priv)
{
	struct wifi_nrf_bus_spi_priv *spi_priv = NULL;

	spi_priv = bus_priv;

	wifi_nrf_osal_bus_spi_deinit(spi_priv->opriv,
				      spi_priv->os_spi_priv);

	wifi_nrf_osal_mem_free(spi_priv->opriv,
			       spi_priv);
}


static unsigned int wifi_nrf_bus_spi_read_word(void *dev_ctx,
						unsigned long addr_offset)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;
	unsigned int val = 0;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	val = wifi_nrf_osal_spi_read_reg32(spi_dev_ctx->spi_priv->opriv,
					    spi_dev_ctx->os_spi_dev_ctx,
					    spi_dev_ctx->host_addr_base + addr_offset);
	return val;
}


static void wifi_nrf_bus_spi_write_word(void *dev_ctx,
					 unsigned long addr_offset,
					 unsigned int val)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_spi_write_reg32(spi_dev_ctx->spi_priv->opriv,
				       spi_dev_ctx->os_spi_dev_ctx,
				       spi_dev_ctx->host_addr_base + addr_offset,
				       val);
}


static void wifi_nrf_bus_spi_read_block(void *dev_ctx,
					 void *dest_addr,
					 unsigned long src_addr_offset,
					 size_t len)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_spi_cpy_from(spi_dev_ctx->spi_priv->opriv,
				    spi_dev_ctx->os_spi_dev_ctx,
				    dest_addr,
				    spi_dev_ctx->host_addr_base + src_addr_offset,
				    len);
}


static void wifi_nrf_bus_spi_write_block(void *dev_ctx,
					  unsigned long dest_addr_offset,
					  const void *src_addr,
					  size_t len)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_spi_cpy_to(spi_dev_ctx->spi_priv->opriv,
				  spi_dev_ctx->os_spi_dev_ctx,
				  spi_dev_ctx->host_addr_base + dest_addr_offset,
				  src_addr,
				  len);
}


static unsigned long wifi_nrf_bus_spi_dma_map(void *dev_ctx,
					       unsigned long virt_addr,
					       size_t len,
					       enum wifi_nrf_osal_dma_dir dma_dir)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;
	unsigned long phy_addr = 0;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	phy_addr = spi_dev_ctx->host_addr_base + (virt_addr - spi_dev_ctx->addr_pktram_base);


	return phy_addr;
}


static unsigned long wifi_nrf_bus_spi_dma_unmap(void *dev_ctx,
						 unsigned long phy_addr,
						 size_t len,
						 enum wifi_nrf_osal_dma_dir dma_dir)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;
	unsigned long virt_addr = 0;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	virt_addr = spi_dev_ctx->addr_pktram_base + (phy_addr - spi_dev_ctx->host_addr_base);

	return virt_addr;
}


#ifdef CONFIG_wifi_nrf_LOW_POWER
static void wifi_nrf_bus_spi_ps_sleep(void *dev_ctx)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_bus_spi_ps_sleep(spi_dev_ctx->spi_priv->opriv,
					spi_dev_ctx->os_spi_dev_ctx);
}


static void wifi_nrf_bus_spi_ps_wake(void *dev_ctx)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_bus_spi_ps_wake(spi_dev_ctx->spi_priv->opriv,
				       spi_dev_ctx->os_spi_dev_ctx);
}


static int wifi_nrf_bus_spi_ps_status(void *dev_ctx)
{
	struct wifi_nrf_bus_spi_dev_ctx *spi_dev_ctx = NULL;

	spi_dev_ctx = (struct wifi_nrf_bus_spi_dev_ctx *)dev_ctx;

	return wifi_nrf_osal_bus_spi_ps_status(spi_dev_ctx->spi_priv->opriv,
						spi_dev_ctx->os_spi_dev_ctx);
}
#endif /* CONFIG_wifi_nrf_LOW_POWER */


static struct wifi_nrf_bal_ops wifi_nrf_bus_spi_ops = {
	.init = &wifi_nrf_bus_spi_init,
	.deinit = &wifi_nrf_bus_spi_deinit,
	.dev_add = &wifi_nrf_bus_spi_dev_add,
	.dev_rem = &wifi_nrf_bus_spi_dev_rem,
	.dev_init = &wifi_nrf_bus_spi_dev_init,
	.dev_deinit = &wifi_nrf_bus_spi_dev_deinit,
	.read_word = &wifi_nrf_bus_spi_read_word,
	.write_word = &wifi_nrf_bus_spi_write_word,
	.read_block = &wifi_nrf_bus_spi_read_block,
	.write_block = &wifi_nrf_bus_spi_write_block,
	.dma_map = &wifi_nrf_bus_spi_dma_map,
	.dma_unmap = &wifi_nrf_bus_spi_dma_unmap,
#ifdef CONFIG_wifi_nrf_LOW_POWER
	.rpu_ps_sleep = &wifi_nrf_bus_spi_ps_sleep,
	.rpu_ps_wake = &wifi_nrf_bus_spi_ps_wake,
	.rpu_ps_status = &wifi_nrf_bus_spi_ps_status,
#endif /* CONFIG_wifi_nrf_LOW_POWER */
};


struct wifi_nrf_bal_ops *get_bus_ops(void)
{
	return &wifi_nrf_bus_spi_ops;
}

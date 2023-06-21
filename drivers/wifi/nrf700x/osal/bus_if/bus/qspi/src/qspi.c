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
#include "qspi.h"
#include "pal.h"


static int wifi_nrf_bus_qspi_irq_handler(void *data)
{
	struct wifi_nrf_bus_qspi_dev_ctx *dev_ctx = NULL;
	struct wifi_nrf_bus_qspi_priv *qspi_priv = NULL;
	int ret = 0;

	dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)data;
	qspi_priv = dev_ctx->qspi_priv;

	ret = qspi_priv->intr_callbk_fn(dev_ctx->bal_dev_ctx);

	return ret;
}


static void *wifi_nrf_bus_qspi_dev_add(void *bus_priv,
				       void *bal_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_bus_qspi_priv *qspi_priv = NULL;
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;
	struct wifi_nrf_osal_host_map host_map;

	qspi_priv = bus_priv;

	qspi_dev_ctx = wifi_nrf_osal_mem_zalloc(qspi_priv->opriv,
						sizeof(*qspi_dev_ctx));

	if (!qspi_dev_ctx) {
		wifi_nrf_osal_log_err(qspi_priv->opriv,
				      "%s: Unable to allocate qspi_dev_ctx\n", __func__);
		goto out;
	}

	qspi_dev_ctx->qspi_priv = qspi_priv;
	qspi_dev_ctx->bal_dev_ctx = bal_dev_ctx;

	qspi_dev_ctx->os_qspi_dev_ctx = wifi_nrf_osal_bus_qspi_dev_add(qspi_priv->opriv,
								       qspi_priv->os_qspi_priv,
								       qspi_dev_ctx);

	if (!qspi_dev_ctx->os_qspi_dev_ctx) {
		wifi_nrf_osal_log_err(qspi_priv->opriv,
				      "%s: wifi_nrf_osal_bus_qspi_dev_add failed\n", __func__);

		wifi_nrf_osal_mem_free(qspi_priv->opriv,
				       qspi_dev_ctx);

		qspi_dev_ctx = NULL;

		goto out;
	}

	wifi_nrf_osal_bus_qspi_dev_host_map_get(qspi_priv->opriv,
						qspi_dev_ctx->os_qspi_dev_ctx,
						&host_map);

	qspi_dev_ctx->host_addr_base = host_map.addr;

	qspi_dev_ctx->addr_pktram_base = qspi_dev_ctx->host_addr_base +
		qspi_priv->cfg_params.addr_pktram_base;

	status = wifi_nrf_osal_bus_qspi_dev_intr_reg(qspi_dev_ctx->qspi_priv->opriv,
						     qspi_dev_ctx->os_qspi_dev_ctx,
						     qspi_dev_ctx,
						     &wifi_nrf_bus_qspi_irq_handler);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(qspi_dev_ctx->qspi_priv->opriv,
				      "%s: Unable to register interrupt to the OS\n",
				      __func__);

		wifi_nrf_osal_bus_qspi_dev_intr_unreg(qspi_dev_ctx->qspi_priv->opriv,
						      qspi_dev_ctx->os_qspi_dev_ctx);

		wifi_nrf_osal_bus_qspi_dev_rem(qspi_dev_ctx->qspi_priv->opriv,
					       qspi_dev_ctx->os_qspi_dev_ctx);

		wifi_nrf_osal_mem_free(qspi_priv->opriv,
				       qspi_dev_ctx);

		qspi_dev_ctx = NULL;

		goto out;
	}

out:
	return qspi_dev_ctx;
}


static void wifi_nrf_bus_qspi_dev_rem(void *bus_dev_ctx)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = bus_dev_ctx;

	wifi_nrf_osal_bus_qspi_dev_intr_unreg(qspi_dev_ctx->qspi_priv->opriv,
					      qspi_dev_ctx->os_qspi_dev_ctx);

	wifi_nrf_osal_bus_qspi_dev_rem(qspi_dev_ctx->qspi_priv->opriv,
					       qspi_dev_ctx->os_qspi_dev_ctx);

	wifi_nrf_osal_mem_free(qspi_dev_ctx->qspi_priv->opriv,
			       qspi_dev_ctx);
}


static enum wifi_nrf_status wifi_nrf_bus_qspi_dev_init(void *bus_dev_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = bus_dev_ctx;

	status = wifi_nrf_osal_bus_qspi_dev_init(qspi_dev_ctx->qspi_priv->opriv,
						 qspi_dev_ctx->os_qspi_dev_ctx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		wifi_nrf_osal_log_err(qspi_dev_ctx->qspi_priv->opriv,
				      "%s: wifi_nrf_osal_qspi_dev_init failed\n", __func__);

		goto out;
	}
out:
	return status;
}


static void wifi_nrf_bus_qspi_dev_deinit(void *bus_dev_ctx)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = bus_dev_ctx;

	wifi_nrf_osal_bus_qspi_dev_deinit(qspi_dev_ctx->qspi_priv->opriv,
					  qspi_dev_ctx->os_qspi_dev_ctx);
}


static void *wifi_nrf_bus_qspi_init(struct wifi_nrf_osal_priv *opriv,
				    void *params,
				    enum wifi_nrf_status (*intr_callbk_fn)(void *bal_dev_ctx))
{
	struct wifi_nrf_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = wifi_nrf_osal_mem_zalloc(opriv,
					     sizeof(*qspi_priv));

	if (!qspi_priv) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to allocate memory for qspi_priv\n",
				      __func__);
		goto out;
	}

	qspi_priv->opriv = opriv;

	wifi_nrf_osal_mem_cpy(opriv,
			      &qspi_priv->cfg_params,
			      params,
			      sizeof(qspi_priv->cfg_params));

	qspi_priv->intr_callbk_fn = intr_callbk_fn;

	qspi_priv->os_qspi_priv = wifi_nrf_osal_bus_qspi_init(opriv);

	if (!qspi_priv->os_qspi_priv) {
		wifi_nrf_osal_log_err(opriv,
				      "%s: Unable to register QSPI driver\n",
				      __func__);

		wifi_nrf_osal_mem_free(opriv,
				       qspi_priv);

		qspi_priv = NULL;

		goto out;
	}
out:
	return qspi_priv;
}


static void wifi_nrf_bus_qspi_deinit(void *bus_priv)
{
	struct wifi_nrf_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = bus_priv;

	wifi_nrf_osal_bus_qspi_deinit(qspi_priv->opriv,
				      qspi_priv->os_qspi_priv);

	wifi_nrf_osal_mem_free(qspi_priv->opriv,
			       qspi_priv);
}


static unsigned int wifi_nrf_bus_qspi_read_word(void *dev_ctx,
						unsigned long addr_offset)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;
	unsigned int val = 0;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	val = wifi_nrf_osal_qspi_read_reg32(qspi_dev_ctx->qspi_priv->opriv,
					    qspi_dev_ctx->os_qspi_dev_ctx,
					    qspi_dev_ctx->host_addr_base + addr_offset);

	return val;
}


static void wifi_nrf_bus_qspi_write_word(void *dev_ctx,
					 unsigned long addr_offset,
					 unsigned int val)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_qspi_write_reg32(qspi_dev_ctx->qspi_priv->opriv,
				       qspi_dev_ctx->os_qspi_dev_ctx,
				       qspi_dev_ctx->host_addr_base + addr_offset,
				       val);
}


static void wifi_nrf_bus_qspi_read_block(void *dev_ctx,
					 void *dest_addr,
					 unsigned long src_addr_offset,
					 size_t len)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_qspi_cpy_from(qspi_dev_ctx->qspi_priv->opriv,
				    qspi_dev_ctx->os_qspi_dev_ctx,
				    dest_addr,
				    qspi_dev_ctx->host_addr_base + src_addr_offset,
				    len);
}


static void wifi_nrf_bus_qspi_write_block(void *dev_ctx,
					  unsigned long dest_addr_offset,
					  const void *src_addr,
					  size_t len)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_qspi_cpy_to(qspi_dev_ctx->qspi_priv->opriv,
				  qspi_dev_ctx->os_qspi_dev_ctx,
				  qspi_dev_ctx->host_addr_base + dest_addr_offset,
				  src_addr,
				  len);
}


static unsigned long wifi_nrf_bus_qspi_dma_map(void *dev_ctx,
					       unsigned long virt_addr,
					       size_t len,
					       enum wifi_nrf_osal_dma_dir dma_dir)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;
	unsigned long phy_addr = 0;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	phy_addr = qspi_dev_ctx->host_addr_base + (virt_addr - qspi_dev_ctx->addr_pktram_base);

	return phy_addr;
}


static unsigned long wifi_nrf_bus_qspi_dma_unmap(void *dev_ctx,
						 unsigned long phy_addr,
						 size_t len,
						 enum wifi_nrf_osal_dma_dir dma_dir)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;
	unsigned long virt_addr = 0;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	virt_addr = qspi_dev_ctx->addr_pktram_base + (phy_addr - qspi_dev_ctx->host_addr_base);

	return virt_addr;
}


#ifdef CONFIG_NRF_WIFI_LOW_POWER
static void wifi_nrf_bus_qspi_ps_sleep(void *dev_ctx)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_bus_qspi_ps_sleep(qspi_dev_ctx->qspi_priv->opriv,
					qspi_dev_ctx->os_qspi_dev_ctx);
}


static void wifi_nrf_bus_qspi_ps_wake(void *dev_ctx)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	wifi_nrf_osal_bus_qspi_ps_wake(qspi_dev_ctx->qspi_priv->opriv,
				       qspi_dev_ctx->os_qspi_dev_ctx);
}


static int wifi_nrf_bus_qspi_ps_status(void *dev_ctx)
{
	struct wifi_nrf_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct wifi_nrf_bus_qspi_dev_ctx *)dev_ctx;

	return wifi_nrf_osal_bus_qspi_ps_status(qspi_dev_ctx->qspi_priv->opriv,
						qspi_dev_ctx->os_qspi_dev_ctx);
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */


static struct wifi_nrf_bal_ops wifi_nrf_bus_qspi_ops = {
	.init = &wifi_nrf_bus_qspi_init,
	.deinit = &wifi_nrf_bus_qspi_deinit,
	.dev_add = &wifi_nrf_bus_qspi_dev_add,
	.dev_rem = &wifi_nrf_bus_qspi_dev_rem,
	.dev_init = &wifi_nrf_bus_qspi_dev_init,
	.dev_deinit = &wifi_nrf_bus_qspi_dev_deinit,
	.read_word = &wifi_nrf_bus_qspi_read_word,
	.write_word = &wifi_nrf_bus_qspi_write_word,
	.read_block = &wifi_nrf_bus_qspi_read_block,
	.write_block = &wifi_nrf_bus_qspi_write_block,
	.dma_map = &wifi_nrf_bus_qspi_dma_map,
	.dma_unmap = &wifi_nrf_bus_qspi_dma_unmap,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	.rpu_ps_sleep = &wifi_nrf_bus_qspi_ps_sleep,
	.rpu_ps_wake = &wifi_nrf_bus_qspi_ps_wake,
	.rpu_ps_status = &wifi_nrf_bus_qspi_ps_status,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
};


struct wifi_nrf_bal_ops *get_bus_ops(void)
{
	return &wifi_nrf_bus_qspi_ops;
}

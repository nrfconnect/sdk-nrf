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


static int nrf_wifi_bus_qspi_irq_handler(void *data)
{
	struct nrf_wifi_bus_qspi_dev_ctx *dev_ctx = NULL;
	struct nrf_wifi_bus_qspi_priv *qspi_priv = NULL;
	int ret = 0;

	dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)data;
	qspi_priv = dev_ctx->qspi_priv;

	ret = qspi_priv->intr_callbk_fn(dev_ctx->bal_dev_ctx);

	return ret;
}


static void *nrf_wifi_bus_qspi_dev_add(void *bus_priv,
				       void *bal_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_bus_qspi_priv *qspi_priv = NULL;
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;
	struct nrf_wifi_osal_host_map host_map;

	qspi_priv = bus_priv;

	qspi_dev_ctx = nrf_wifi_osal_mem_zalloc(qspi_priv->opriv,
						sizeof(*qspi_dev_ctx));

	if (!qspi_dev_ctx) {
		nrf_wifi_osal_log_err(qspi_priv->opriv,
				      "%s: Unable to allocate qspi_dev_ctx\n", __func__);
		goto out;
	}

	qspi_dev_ctx->qspi_priv = qspi_priv;
	qspi_dev_ctx->bal_dev_ctx = bal_dev_ctx;

	qspi_dev_ctx->os_qspi_dev_ctx = nrf_wifi_osal_bus_qspi_dev_add(qspi_priv->opriv,
								       qspi_priv->os_qspi_priv,
								       qspi_dev_ctx);

	if (!qspi_dev_ctx->os_qspi_dev_ctx) {
		nrf_wifi_osal_log_err(qspi_priv->opriv,
				      "%s: nrf_wifi_osal_bus_qspi_dev_add failed\n", __func__);

		nrf_wifi_osal_mem_free(qspi_priv->opriv,
				       qspi_dev_ctx);

		qspi_dev_ctx = NULL;

		goto out;
	}

	nrf_wifi_osal_bus_qspi_dev_host_map_get(qspi_priv->opriv,
						qspi_dev_ctx->os_qspi_dev_ctx,
						&host_map);

	qspi_dev_ctx->host_addr_base = host_map.addr;

	qspi_dev_ctx->addr_pktram_base = qspi_dev_ctx->host_addr_base +
		qspi_priv->cfg_params.addr_pktram_base;

	status = nrf_wifi_osal_bus_qspi_dev_intr_reg(qspi_dev_ctx->qspi_priv->opriv,
						     qspi_dev_ctx->os_qspi_dev_ctx,
						     qspi_dev_ctx,
						     &nrf_wifi_bus_qspi_irq_handler);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(qspi_dev_ctx->qspi_priv->opriv,
				      "%s: Unable to register interrupt to the OS\n",
				      __func__);

		nrf_wifi_osal_bus_qspi_dev_intr_unreg(qspi_dev_ctx->qspi_priv->opriv,
						      qspi_dev_ctx->os_qspi_dev_ctx);

		nrf_wifi_osal_bus_qspi_dev_rem(qspi_dev_ctx->qspi_priv->opriv,
					       qspi_dev_ctx->os_qspi_dev_ctx);

		nrf_wifi_osal_mem_free(qspi_priv->opriv,
				       qspi_dev_ctx);

		qspi_dev_ctx = NULL;

		goto out;
	}

out:
	return qspi_dev_ctx;
}


static void nrf_wifi_bus_qspi_dev_rem(void *bus_dev_ctx)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = bus_dev_ctx;

	nrf_wifi_osal_bus_qspi_dev_intr_unreg(qspi_dev_ctx->qspi_priv->opriv,
					      qspi_dev_ctx->os_qspi_dev_ctx);

	nrf_wifi_osal_bus_qspi_dev_rem(qspi_dev_ctx->qspi_priv->opriv,
					       qspi_dev_ctx->os_qspi_dev_ctx);

	nrf_wifi_osal_mem_free(qspi_dev_ctx->qspi_priv->opriv,
			       qspi_dev_ctx);
}


static enum nrf_wifi_status nrf_wifi_bus_qspi_dev_init(void *bus_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = bus_dev_ctx;

	status = nrf_wifi_osal_bus_qspi_dev_init(qspi_dev_ctx->qspi_priv->opriv,
						 qspi_dev_ctx->os_qspi_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(qspi_dev_ctx->qspi_priv->opriv,
				      "%s: nrf_wifi_osal_qspi_dev_init failed\n", __func__);

		goto out;
	}
out:
	return status;
}


static void nrf_wifi_bus_qspi_dev_deinit(void *bus_dev_ctx)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = bus_dev_ctx;

	nrf_wifi_osal_bus_qspi_dev_deinit(qspi_dev_ctx->qspi_priv->opriv,
					  qspi_dev_ctx->os_qspi_dev_ctx);
}


static void *nrf_wifi_bus_qspi_init(struct nrf_wifi_osal_priv *opriv,
				    void *params,
				    enum nrf_wifi_status (*intr_callbk_fn)(void *bal_dev_ctx))
{
	struct nrf_wifi_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = nrf_wifi_osal_mem_zalloc(opriv,
					     sizeof(*qspi_priv));

	if (!qspi_priv) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Unable to allocate memory for qspi_priv\n",
				      __func__);
		goto out;
	}

	qspi_priv->opriv = opriv;

	nrf_wifi_osal_mem_cpy(opriv,
			      &qspi_priv->cfg_params,
			      params,
			      sizeof(qspi_priv->cfg_params));

	qspi_priv->intr_callbk_fn = intr_callbk_fn;

	qspi_priv->os_qspi_priv = nrf_wifi_osal_bus_qspi_init(opriv);

	if (!qspi_priv->os_qspi_priv) {
		nrf_wifi_osal_log_err(opriv,
				      "%s: Unable to register QSPI driver\n",
				      __func__);

		nrf_wifi_osal_mem_free(opriv,
				       qspi_priv);

		qspi_priv = NULL;

		goto out;
	}
out:
	return qspi_priv;
}


static void nrf_wifi_bus_qspi_deinit(void *bus_priv)
{
	struct nrf_wifi_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = bus_priv;

	nrf_wifi_osal_bus_qspi_deinit(qspi_priv->opriv,
				      qspi_priv->os_qspi_priv);

	nrf_wifi_osal_mem_free(qspi_priv->opriv,
			       qspi_priv);
}


static unsigned int nrf_wifi_bus_qspi_read_word(void *dev_ctx,
						unsigned long addr_offset)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;
	unsigned int val = 0;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	val = nrf_wifi_osal_qspi_read_reg32(qspi_dev_ctx->qspi_priv->opriv,
					    qspi_dev_ctx->os_qspi_dev_ctx,
					    qspi_dev_ctx->host_addr_base + addr_offset);

	return val;
}


static void nrf_wifi_bus_qspi_write_word(void *dev_ctx,
					 unsigned long addr_offset,
					 unsigned int val)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	nrf_wifi_osal_qspi_write_reg32(qspi_dev_ctx->qspi_priv->opriv,
				       qspi_dev_ctx->os_qspi_dev_ctx,
				       qspi_dev_ctx->host_addr_base + addr_offset,
				       val);
}


static void nrf_wifi_bus_qspi_read_block(void *dev_ctx,
					 void *dest_addr,
					 unsigned long src_addr_offset,
					 size_t len)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	nrf_wifi_osal_qspi_cpy_from(qspi_dev_ctx->qspi_priv->opriv,
				    qspi_dev_ctx->os_qspi_dev_ctx,
				    dest_addr,
				    qspi_dev_ctx->host_addr_base + src_addr_offset,
				    len);
}


static void nrf_wifi_bus_qspi_write_block(void *dev_ctx,
					  unsigned long dest_addr_offset,
					  const void *src_addr,
					  size_t len)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	nrf_wifi_osal_qspi_cpy_to(qspi_dev_ctx->qspi_priv->opriv,
				  qspi_dev_ctx->os_qspi_dev_ctx,
				  qspi_dev_ctx->host_addr_base + dest_addr_offset,
				  src_addr,
				  len);
}


static unsigned long nrf_wifi_bus_qspi_dma_map(void *dev_ctx,
					       unsigned long virt_addr,
					       size_t len,
					       enum nrf_wifi_osal_dma_dir dma_dir)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;
	unsigned long phy_addr = 0;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	phy_addr = qspi_dev_ctx->host_addr_base + (virt_addr - qspi_dev_ctx->addr_pktram_base);

	return phy_addr;
}


static unsigned long nrf_wifi_bus_qspi_dma_unmap(void *dev_ctx,
						 unsigned long phy_addr,
						 size_t len,
						 enum nrf_wifi_osal_dma_dir dma_dir)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;
	unsigned long virt_addr = 0;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	virt_addr = qspi_dev_ctx->addr_pktram_base + (phy_addr - qspi_dev_ctx->host_addr_base);

	return virt_addr;
}


#ifdef CONFIG_NRF_WIFI_LOW_POWER
static void nrf_wifi_bus_qspi_ps_sleep(void *dev_ctx)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	nrf_wifi_osal_bus_qspi_ps_sleep(qspi_dev_ctx->qspi_priv->opriv,
					qspi_dev_ctx->os_qspi_dev_ctx);
}


static void nrf_wifi_bus_qspi_ps_wake(void *dev_ctx)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	nrf_wifi_osal_bus_qspi_ps_wake(qspi_dev_ctx->qspi_priv->opriv,
				       qspi_dev_ctx->os_qspi_dev_ctx);
}


static int nrf_wifi_bus_qspi_ps_status(void *dev_ctx)
{
	struct nrf_wifi_bus_qspi_dev_ctx *qspi_dev_ctx = NULL;

	qspi_dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)dev_ctx;

	return nrf_wifi_osal_bus_qspi_ps_status(qspi_dev_ctx->qspi_priv->opriv,
						qspi_dev_ctx->os_qspi_dev_ctx);
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */


static struct nrf_wifi_bal_ops nrf_wifi_bus_qspi_ops = {
	.init = &nrf_wifi_bus_qspi_init,
	.deinit = &nrf_wifi_bus_qspi_deinit,
	.dev_add = &nrf_wifi_bus_qspi_dev_add,
	.dev_rem = &nrf_wifi_bus_qspi_dev_rem,
	.dev_init = &nrf_wifi_bus_qspi_dev_init,
	.dev_deinit = &nrf_wifi_bus_qspi_dev_deinit,
	.read_word = &nrf_wifi_bus_qspi_read_word,
	.write_word = &nrf_wifi_bus_qspi_write_word,
	.read_block = &nrf_wifi_bus_qspi_read_block,
	.write_block = &nrf_wifi_bus_qspi_write_block,
	.dma_map = &nrf_wifi_bus_qspi_dma_map,
	.dma_unmap = &nrf_wifi_bus_qspi_dma_unmap,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	.rpu_ps_sleep = &nrf_wifi_bus_qspi_ps_sleep,
	.rpu_ps_wake = &nrf_wifi_bus_qspi_ps_wake,
	.rpu_ps_status = &nrf_wifi_bus_qspi_ps_status,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
};


struct nrf_wifi_bal_ops *get_bus_ops(void)
{
	return &nrf_wifi_bus_qspi_ops;
}

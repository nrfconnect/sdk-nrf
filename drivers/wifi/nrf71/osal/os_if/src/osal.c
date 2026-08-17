/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Implements OSAL APIs to abstract OS primitives.
 */

#include "osal_api.h"
#include "osal_ops.h"

const struct nrf_wifi_osal_ops *os_ops;

void nrf_wifi_osal_init(const struct nrf_wifi_osal_ops *ops)
{
	os_ops = ops;
}


void nrf_wifi_osal_deinit(void)
{
	os_ops = NULL;
}


void *nrf_wifi_osal_iomem_mmap(unsigned long addr,
			       unsigned long size)
{
	return os_ops->iomem_mmap(addr,
				  size);
}


void nrf_wifi_osal_iomem_unmap(volatile void *addr)
{
	os_ops->iomem_unmap(addr);
}


unsigned int nrf_wifi_osal_iomem_read_reg32(const volatile void *addr)
{
	return os_ops->iomem_read_reg32(addr);
}


void nrf_wifi_osal_iomem_write_reg32(volatile void *addr,
				     unsigned int val)
{
	os_ops->iomem_write_reg32(addr,
				  val);
}


void nrf_wifi_osal_iomem_cpy_from(void *dest,
				  const volatile void *src,
				  size_t count)
{
	os_ops->iomem_cpy_from(dest,
			       src,
			       count);
}


void nrf_wifi_osal_iomem_cpy_to(volatile void *dest,
				const void *src,
				size_t count)
{
	os_ops->iomem_cpy_to(dest,
			     src,
			     count);
}


void *nrf_wifi_osal_bus_pcie_init(const char *dev_name,
				  unsigned int vendor_id,
				  unsigned int sub_vendor_id,
				  unsigned int device_id,
				  unsigned int sub_device_id)
{
	return os_ops->bus_pcie_init(dev_name,
				     vendor_id,
				     sub_vendor_id,
				     device_id,
				     sub_device_id);
}


void nrf_wifi_osal_bus_pcie_deinit(void *os_pcie_priv)
{
	os_ops->bus_pcie_deinit(os_pcie_priv);
}


void *nrf_wifi_osal_bus_pcie_dev_add(void *os_pcie_priv,
				     void *osal_pcie_dev_ctx)
{
	return os_ops->bus_pcie_dev_add(os_pcie_priv,
					osal_pcie_dev_ctx);

}


void nrf_wifi_osal_bus_pcie_dev_rem(void *os_pcie_dev_ctx)
{
	return os_ops->bus_pcie_dev_rem(os_pcie_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_osal_bus_pcie_dev_init(void *os_pcie_dev_ctx)
{
	return os_ops->bus_pcie_dev_init(os_pcie_dev_ctx);

}


void nrf_wifi_osal_bus_pcie_dev_deinit(void *os_pcie_dev_ctx)
{
	return os_ops->bus_pcie_dev_deinit(os_pcie_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_osal_bus_pcie_dev_intr_reg(void *os_pcie_dev_ctx,
							 void *callbk_data,
							 int (*callbk_fn)(void *callbk_data))
{
	return os_ops->bus_pcie_dev_intr_reg(os_pcie_dev_ctx,
					     callbk_data,
					     callbk_fn);
}


void nrf_wifi_osal_bus_pcie_dev_intr_unreg(void *os_pcie_dev_ctx)
{
	os_ops->bus_pcie_dev_intr_unreg(os_pcie_dev_ctx);
}


void *nrf_wifi_osal_bus_pcie_dev_dma_map(void *os_pcie_dev_ctx,
					 void *virt_addr,
					 size_t size,
					 enum nrf_wifi_osal_dma_dir dir)
{
	return os_ops->bus_pcie_dev_dma_map(os_pcie_dev_ctx,
					    virt_addr,
					    size,
					    dir);
}


void nrf_wifi_osal_bus_pcie_dev_dma_unmap(void *os_pcie_dev_ctx,
					  void *dma_addr,
					  size_t size,
					  enum nrf_wifi_osal_dma_dir dir)
{
	os_ops->bus_pcie_dev_dma_unmap(os_pcie_dev_ctx,
				       dma_addr,
				       size,
				       dir);
}


void nrf_wifi_osal_bus_pcie_dev_host_map_get(void *os_pcie_dev_ctx,
					     struct nrf_wifi_osal_host_map *host_map)
{
	os_ops->bus_pcie_dev_host_map_get(os_pcie_dev_ctx,
					  host_map);
}


void *nrf_wifi_osal_bus_qspi_init(void)
{
	return os_ops->bus_qspi_init();
}


void nrf_wifi_osal_bus_qspi_deinit(void *os_qspi_priv)
{
	os_ops->bus_qspi_deinit(os_qspi_priv);
}


void *nrf_wifi_osal_bus_qspi_dev_add(void *os_qspi_priv,
				     void *osal_qspi_dev_ctx)
{
	return os_ops->bus_qspi_dev_add(os_qspi_priv,
					osal_qspi_dev_ctx);
}


void nrf_wifi_osal_bus_qspi_dev_rem(void *os_qspi_dev_ctx)
{
	os_ops->bus_qspi_dev_rem(os_qspi_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_osal_bus_qspi_dev_init(void *os_qspi_dev_ctx)
{
	return os_ops->bus_qspi_dev_init(os_qspi_dev_ctx);
}


void nrf_wifi_osal_bus_qspi_dev_deinit(void *os_qspi_dev_ctx)
{
	os_ops->bus_qspi_dev_deinit(os_qspi_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_osal_bus_qspi_dev_intr_reg(void *os_qspi_dev_ctx,
							 void *callbk_data,
							 int (*callbk_fn)(void *callbk_data))
{
	return os_ops->bus_qspi_dev_intr_reg(os_qspi_dev_ctx,
					     callbk_data,
					     callbk_fn);
}


void nrf_wifi_osal_bus_qspi_dev_intr_unreg(void *os_qspi_dev_ctx)
{
	os_ops->bus_qspi_dev_intr_unreg(os_qspi_dev_ctx);
}


void nrf_wifi_osal_bus_qspi_dev_host_map_get(void *os_qspi_dev_ctx,
					     struct nrf_wifi_osal_host_map *host_map)
{
	os_ops->bus_qspi_dev_host_map_get(os_qspi_dev_ctx,
					  host_map);
}


unsigned int nrf_wifi_osal_qspi_read_reg32(void *priv,
					   unsigned long addr)
{
	return os_ops->qspi_read_reg32(priv,
				       addr);
}


void nrf_wifi_osal_qspi_write_reg32(void *priv,
				    unsigned long addr,
				    unsigned int val)
{
	os_ops->qspi_write_reg32(priv,
				 addr,
				 val);
}


void nrf_wifi_osal_qspi_cpy_from(void *priv,
				 void *dest,
				 unsigned long addr,
				 size_t count)
{
	os_ops->qspi_cpy_from(priv,
			      dest,
			      addr,
			      count);
}


void nrf_wifi_osal_qspi_cpy_to(void *priv,
			       unsigned long addr,
			       const void *src,
			       size_t count)
{
	os_ops->qspi_cpy_to(priv,
			    addr,
			    src,
			    count);
}


void *nrf_wifi_osal_bus_spi_init(void)
{
	return os_ops->bus_spi_init();
}


void nrf_wifi_osal_bus_spi_deinit(void *os_spi_priv)
{
	os_ops->bus_spi_deinit(os_spi_priv);
}


void *nrf_wifi_osal_bus_spi_dev_add(void *os_spi_priv,
				    void *osal_spi_dev_ctx)
{
	return os_ops->bus_spi_dev_add(os_spi_priv,
				       osal_spi_dev_ctx);
}


void nrf_wifi_osal_bus_spi_dev_rem(void *os_spi_dev_ctx)
{
	os_ops->bus_spi_dev_rem(os_spi_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_osal_bus_spi_dev_init(void *os_spi_dev_ctx)
{
	return os_ops->bus_spi_dev_init(os_spi_dev_ctx);
}


void nrf_wifi_osal_bus_spi_dev_deinit(void *os_spi_dev_ctx)
{
	os_ops->bus_spi_dev_deinit(os_spi_dev_ctx);
}


enum nrf_wifi_status nrf_wifi_osal_bus_spi_dev_intr_reg(void *os_spi_dev_ctx,
							void *callbk_data,
							int (*callbk_fn)(void *callbk_data))
{
	return os_ops->bus_spi_dev_intr_reg(os_spi_dev_ctx,
					    callbk_data,
					    callbk_fn);
}


void nrf_wifi_osal_bus_spi_dev_intr_unreg(void *os_spi_dev_ctx)
{
	os_ops->bus_spi_dev_intr_unreg(os_spi_dev_ctx);
}


void nrf_wifi_osal_bus_spi_dev_host_map_get(void *os_spi_dev_ctx,
					    struct nrf_wifi_osal_host_map *host_map)
{
	os_ops->bus_spi_dev_host_map_get(os_spi_dev_ctx,
					 host_map);
}

unsigned int nrf_wifi_osal_spi_read_reg32(void *os_spi_dev_ctx,
					  unsigned long addr)
{
	return os_ops->spi_read_reg32(os_spi_dev_ctx, addr);
}


void nrf_wifi_osal_spi_write_reg32(void *os_spi_dev_ctx,
				   unsigned long addr,
				   unsigned int val)
{
	os_ops->spi_write_reg32(os_spi_dev_ctx,
				addr,
				val);
}


void nrf_wifi_osal_spi_cpy_from(void *os_spi_dev_ctx,
				void *dest,
				unsigned long addr,
				size_t count)
{
	os_ops->spi_cpy_from(os_spi_dev_ctx,
			     dest,
			     addr,
			     count);
}


void nrf_wifi_osal_spi_cpy_to(void *os_spi_dev_ctx,
			      unsigned long addr,
			      const void *src,
			      size_t count)
{
		os_ops->spi_cpy_to(os_spi_dev_ctx,
				   addr,
				   src,
				   count);
}

#ifdef NRF_WIFI_LOW_POWER
int nrf_wifi_osal_bus_qspi_ps_sleep(void *os_qspi_priv)
{
	return os_ops->bus_qspi_ps_sleep(os_qspi_priv);
}


int nrf_wifi_osal_bus_qspi_ps_wake(void *os_qspi_priv)
{
	return os_ops->bus_qspi_ps_wake(os_qspi_priv);
}


int nrf_wifi_osal_bus_qspi_ps_status(void *os_qspi_priv)
{
	return os_ops->bus_qspi_ps_status(os_qspi_priv);
}
#endif /* NRF_WIFI_LOW_POWER */

unsigned char nrf_wifi_osal_rand8_get(void)
{
	return os_ops->rand8_get();
}

int nrf_wifi_osal_ipc_send_msg(unsigned int msg_type,
				 void *msg,
				 unsigned int msg_len)
{
	return os_ops->ipc_send_msg(msg_type, msg, msg_len);
}

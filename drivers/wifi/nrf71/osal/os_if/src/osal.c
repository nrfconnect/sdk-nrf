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


void *nrf_wifi_osal_spinlock_alloc(void)
{
	return os_ops->spinlock_alloc();
}


void nrf_wifi_osal_spinlock_free(void *lock)
{
	os_ops->spinlock_free(lock);
}


void nrf_wifi_osal_spinlock_init(void *lock)
{
	os_ops->spinlock_init(lock);
}


void nrf_wifi_osal_spinlock_take(void *lock)
{
	os_ops->spinlock_take(lock);
}


void nrf_wifi_osal_spinlock_rel(void *lock)
{
	os_ops->spinlock_rel(lock);
}

void nrf_wifi_osal_spinlock_irq_take(void *lock,
				     unsigned long *flags)
{
	os_ops->spinlock_irq_take(lock,
				  flags);
}


void nrf_wifi_osal_spinlock_irq_rel(void *lock,
				    unsigned long *flags)
{
	os_ops->spinlock_irq_rel(lock,
				 flags);
}


#if WIFI_NRF71_LOG_LEVEL >= NRF_WIFI_LOG_LEVEL_DBG
int nrf_wifi_osal_log_dbg(const char *fmt,
			  ...)
{
	va_list args;
	int ret = -1;

	va_start(args, fmt);

	ret = os_ops->log_dbg(fmt, args);

	va_end(args);

	return ret;
}
#endif /* WIFI_NRF71_LOG_LEVEL_DBG */


#if WIFI_NRF71_LOG_LEVEL >= NRF_WIFI_LOG_LEVEL_INF
int nrf_wifi_osal_log_info(const char *fmt,
			   ...)
{
	va_list args;
	int ret = -1;

	va_start(args, fmt);

	ret = os_ops->log_info(fmt, args);

	va_end(args);

	return ret;
}
#endif /* WIFI_NRF71_LOG_LEVEL_INF */


#if WIFI_NRF71_LOG_LEVEL >= NRF_WIFI_LOG_LEVEL_ERR
int nrf_wifi_osal_log_err(const char *fmt,
			  ...)
{
	va_list args;
	int ret = -1;

	va_start(args, fmt);

	ret = os_ops->log_err(fmt, args);

	va_end(args);

	return ret;
}
#endif /* WIFI_NRF71_LOG_LEVEL_ERR */


void *nrf_wifi_osal_llist_node_alloc(void)
{
	return os_ops->llist_node_alloc();
}


void *nrf_wifi_osal_ctrl_llist_node_alloc(void)
{
	return os_ops->ctrl_llist_node_alloc();
}


void nrf_wifi_osal_llist_node_free(void *node)
{
	os_ops->llist_node_free(node);
}


void nrf_wifi_osal_ctrl_llist_node_free(void *node)
{
	os_ops->ctrl_llist_node_free(node);
}


void *nrf_wifi_osal_llist_node_data_get(void *node)
{
	return os_ops->llist_node_data_get(node);
}


void nrf_wifi_osal_llist_node_data_set(void *node,
				       void *data)
{
	os_ops->llist_node_data_set(node,
				    data);
}


void *nrf_wifi_osal_llist_alloc(void)
{
	return os_ops->llist_alloc();
}

void *nrf_wifi_osal_ctrl_llist_alloc(void)
{
	return os_ops->ctrl_llist_alloc();
}

void nrf_wifi_osal_llist_free(void *llist)
{
	return os_ops->llist_free(llist);
}

void nrf_wifi_osal_ctrl_llist_free(void *llist)
{
	return os_ops->ctrl_llist_free(llist);
}

void nrf_wifi_osal_llist_init(void *llist)
{
	return os_ops->llist_init(llist);
}


void nrf_wifi_osal_llist_add_node_tail(void *llist,
				       void *llist_node)
{
	return os_ops->llist_add_node_tail(llist,
					   llist_node);
}

void nrf_wifi_osal_llist_add_node_head(void *llist,
				       void *llist_node)
{
	return os_ops->llist_add_node_head(llist,
					   llist_node);
}


void *nrf_wifi_osal_llist_get_node_head(void *llist)
{
	return os_ops->llist_get_node_head(llist);
}


void *nrf_wifi_osal_llist_get_node_nxt(void *llist,
				       void *llist_node)
{
	return os_ops->llist_get_node_nxt(llist,
					  llist_node);
}


void nrf_wifi_osal_llist_del_node(void *llist,
				  void *llist_node)
{
	os_ops->llist_del_node(llist,
			       llist_node);
}


unsigned int nrf_wifi_osal_llist_len(void *llist)
{
	return os_ops->llist_len(llist);
}


void nrf_wifi_osal_sleep_ms(unsigned int msecs)
{
	os_ops->sleep_ms(msecs);
}


void nrf_wifi_osal_delay_us(unsigned long usecs)
{
	os_ops->delay_us(usecs);
}


unsigned long nrf_wifi_osal_time_get_curr_us(void)
{
	return os_ops->time_get_curr_us();
}


unsigned int nrf_wifi_osal_time_elapsed_us(unsigned long start_time_us)
{
	return os_ops->time_elapsed_us(start_time_us);
}

unsigned long nrf_wifi_osal_time_get_curr_ms(void)
{
	return os_ops->time_get_curr_ms();
}

unsigned int nrf_wifi_osal_time_elapsed_ms(unsigned long start_time_ms)
{
	return os_ops->time_elapsed_ms(start_time_ms);
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

void nrf_wifi_osal_assert(int test_val,
			  int val,
			  enum nrf_wifi_assert_op_type op,
			  char *msg)
{
	return os_ops->assert(test_val, val, op, msg);
}

unsigned int nrf_wifi_osal_strlen(const void *str)
{
	return os_ops->strlen(str);
}

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

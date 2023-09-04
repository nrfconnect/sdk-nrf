/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Implements OSAL APIs to abstract OS primitives.
 */

#include "osal_api.h"
#include "osal_ops.h"

struct wifi_nrf_osal_priv *wifi_nrf_osal_init(void)
{
	struct wifi_nrf_osal_priv *opriv = NULL;
	const struct wifi_nrf_osal_ops *ops = NULL;

	ops = get_os_ops();

	opriv = ops->mem_zalloc(sizeof(struct wifi_nrf_osal_priv));

	if (!opriv) {
		goto out;
	}

	opriv->ops = ops;

out:
	return opriv;
}


void wifi_nrf_osal_deinit(struct wifi_nrf_osal_priv *opriv)
{
	const struct wifi_nrf_osal_ops *ops = NULL;

	ops = opriv->ops;

	ops->mem_free(opriv);
}


void *wifi_nrf_osal_mem_alloc(struct wifi_nrf_osal_priv *opriv,
			      size_t size)
{
	return opriv->ops->mem_alloc(size);
}


void *wifi_nrf_osal_mem_zalloc(struct wifi_nrf_osal_priv *opriv,
			       size_t size)
{
	return opriv->ops->mem_zalloc(size);
}


void wifi_nrf_osal_mem_free(struct wifi_nrf_osal_priv *opriv,
			    void *buf)
{
	opriv->ops->mem_free(buf);
}


void *wifi_nrf_osal_mem_cpy(struct wifi_nrf_osal_priv *opriv,
			    void *dest,
			    const void *src,
			    size_t count)
{
	return opriv->ops->mem_cpy(dest,
				   src,
				   count);
}


void *wifi_nrf_osal_mem_set(struct wifi_nrf_osal_priv *opriv,
			    void *start,
			    int val,
			    size_t size)
{
	return opriv->ops->mem_set(start,
				   val,
				   size);
}


int wifi_nrf_osal_mem_cmp(struct wifi_nrf_osal_priv *opriv,
			  const void *addr1,
			  const void *addr2,
			  size_t size)
{
	return opriv->ops->mem_cmp(addr1,
				   addr2,
				   size);
}


void *wifi_nrf_osal_iomem_mmap(struct wifi_nrf_osal_priv *opriv,
			       unsigned long addr,
			       unsigned long size)
{
	return opriv->ops->iomem_mmap(addr,
				      size);
}


void wifi_nrf_osal_iomem_unmap(struct wifi_nrf_osal_priv *opriv,
			       volatile void *addr)
{
	opriv->ops->iomem_unmap(addr);
}


unsigned int wifi_nrf_osal_iomem_read_reg32(struct wifi_nrf_osal_priv *opriv,
					    const volatile void *addr)
{
	return opriv->ops->iomem_read_reg32(addr);
}


void wifi_nrf_osal_iomem_write_reg32(struct wifi_nrf_osal_priv *opriv,
				     volatile void *addr,
				     unsigned int val)
{
	opriv->ops->iomem_write_reg32(addr,
				      val);
}


void wifi_nrf_osal_iomem_cpy_from(struct wifi_nrf_osal_priv *opriv,
				  void *dest,
				  const volatile void *src,
				  size_t count)
{
	opriv->ops->iomem_cpy_from(dest,
				   src,
				   count);
}


void wifi_nrf_osal_iomem_cpy_to(struct wifi_nrf_osal_priv *opriv,
				volatile void *dest,
				const void *src,
				size_t count)
{
	opriv->ops->iomem_cpy_to(dest,
				 src,
				 count);
}


void *wifi_nrf_osal_spinlock_alloc(struct wifi_nrf_osal_priv *opriv)
{
	return opriv->ops->spinlock_alloc();
}


void wifi_nrf_osal_spinlock_free(struct wifi_nrf_osal_priv *opriv,
				 void *lock)
{
	opriv->ops->spinlock_free(lock);
}


void wifi_nrf_osal_spinlock_init(struct wifi_nrf_osal_priv *opriv,
				 void *lock)
{
	opriv->ops->spinlock_init(lock);
}


void wifi_nrf_osal_spinlock_take(struct wifi_nrf_osal_priv *opriv,
				 void *lock)
{
	opriv->ops->spinlock_take(lock);
}


void wifi_nrf_osal_spinlock_rel(struct wifi_nrf_osal_priv *opriv,
				void *lock)
{
	opriv->ops->spinlock_rel(lock);
}

void wifi_nrf_osal_spinlock_irq_take(struct wifi_nrf_osal_priv *opriv,
				     void *lock,
				     unsigned long *flags)
{
	opriv->ops->spinlock_irq_take(lock,
				      flags);
}


void wifi_nrf_osal_spinlock_irq_rel(struct wifi_nrf_osal_priv *opriv,
				    void *lock,
				    unsigned long *flags)
{
	opriv->ops->spinlock_irq_rel(lock,
				     flags);
}


#if CONFIG_NRF700X_LOG_LEVEL < 1
int wifi_nrf_osal_log_dbg(struct wifi_nrf_osal_priv *opriv,
			  const char *fmt,
			  ...)
{
	va_list args;
	int ret = -1;

	va_start(args, fmt);

	ret = opriv->ops->log_dbg(fmt, args);

	va_end(args);

	return ret;
}
#endif /* CONFIG_NRF700X_LOG_LEVEL < 1 */


#if CONFIG_NRF700X_LOG_LEVEL <= 3
int wifi_nrf_osal_log_info(struct wifi_nrf_osal_priv *opriv,
			   const char *fmt,
			   ...)
{
	va_list args;
	int ret = -1;

	va_start(args, fmt);

	ret = opriv->ops->log_info(fmt, args);

	va_end(args);

	return ret;
}
#endif /* CONFIG_NRF700X_LOG_LEVEL <=3 */


#if CONFIG_NRF700X_LOG_LEVEL <= 5
int wifi_nrf_osal_log_err(struct wifi_nrf_osal_priv *opriv,
			  const char *fmt,
			  ...)
{
	va_list args;
	int ret = -1;

	va_start(args, fmt);

	ret = opriv->ops->log_err(fmt, args);

	va_end(args);

	return ret;
}
#endif /* CONFIG_NRF700X_LOG_LEVEL <=5 */


void *wifi_nrf_osal_llist_node_alloc(struct wifi_nrf_osal_priv *opriv)
{
	return opriv->ops->llist_node_alloc();
}


void wifi_nrf_osal_llist_node_free(struct wifi_nrf_osal_priv *opriv,
				   void *node)
{
	opriv->ops->llist_node_free(node);
}


void *wifi_nrf_osal_llist_node_data_get(struct wifi_nrf_osal_priv *opriv,
					void *node)
{
	return opriv->ops->llist_node_data_get(node);
}


void wifi_nrf_osal_llist_node_data_set(struct wifi_nrf_osal_priv *opriv,
				       void *node,
				       void *data)
{
	opriv->ops->llist_node_data_set(node,
					data);
}


void *wifi_nrf_osal_llist_alloc(struct wifi_nrf_osal_priv *opriv)
{
	return opriv->ops->llist_alloc();
}


void wifi_nrf_osal_llist_free(struct wifi_nrf_osal_priv *opriv,
			      void *llist)
{
	return opriv->ops->llist_free(llist);
}


void wifi_nrf_osal_llist_init(struct wifi_nrf_osal_priv *opriv,
			      void *llist)
{
	return opriv->ops->llist_init(llist);
}


void wifi_nrf_osal_llist_add_node_tail(struct wifi_nrf_osal_priv *opriv,
				       void *llist,
				       void *llist_node)
{
	return opriv->ops->llist_add_node_tail(llist,
					       llist_node);
}

void wifi_nrf_osal_llist_add_node_head(struct wifi_nrf_osal_priv *opriv,
				       void *llist,
				       void *llist_node)
{
	return opriv->ops->llist_add_node_head(llist,
					       llist_node);
}


void *wifi_nrf_osal_llist_get_node_head(struct wifi_nrf_osal_priv *opriv,
					void *llist)
{
	return opriv->ops->llist_get_node_head(llist);
}


void *wifi_nrf_osal_llist_get_node_nxt(struct wifi_nrf_osal_priv *opriv,
				       void *llist,
				       void *llist_node)
{
	return opriv->ops->llist_get_node_nxt(llist,
					      llist_node);
}


void wifi_nrf_osal_llist_del_node(struct wifi_nrf_osal_priv *opriv,
				  void *llist,
				  void *llist_node)
{
	opriv->ops->llist_del_node(llist,
				   llist_node);
}


unsigned int wifi_nrf_osal_llist_len(struct wifi_nrf_osal_priv *opriv,
				     void *llist)
{
	return opriv->ops->llist_len(llist);
}


void *wifi_nrf_osal_nbuf_alloc(struct wifi_nrf_osal_priv *opriv,
			       unsigned int size)
{
	return opriv->ops->nbuf_alloc(size);
}


void wifi_nrf_osal_nbuf_free(struct wifi_nrf_osal_priv *opriv,
			     void *nbuf)
{
	opriv->ops->nbuf_free(nbuf);
}


void wifi_nrf_osal_nbuf_headroom_res(struct wifi_nrf_osal_priv *opriv,
				     void *nbuf,
				     unsigned int size)
{
	opriv->ops->nbuf_headroom_res(nbuf,
				      size);
}


unsigned int wifi_nrf_osal_nbuf_headroom_get(struct wifi_nrf_osal_priv *opriv,
					     void *nbuf)
{
	return opriv->ops->nbuf_headroom_get(nbuf);
}


unsigned int wifi_nrf_osal_nbuf_data_size(struct wifi_nrf_osal_priv *opriv,
					  void *nbuf)
{
	return opriv->ops->nbuf_data_size(nbuf);
}


void *wifi_nrf_osal_nbuf_data_get(struct wifi_nrf_osal_priv *opriv,
				  void *nbuf)
{
	return opriv->ops->nbuf_data_get(nbuf);
}


void *wifi_nrf_osal_nbuf_data_put(struct wifi_nrf_osal_priv *opriv,
				  void *nbuf,
				  unsigned int size)
{
	return opriv->ops->nbuf_data_put(nbuf,
					 size);
}


void *wifi_nrf_osal_nbuf_data_push(struct wifi_nrf_osal_priv *opriv,
				   void *nbuf,
				   unsigned int size)
{
	return opriv->ops->nbuf_data_push(nbuf,
					  size);
}


void *wifi_nrf_osal_nbuf_data_pull(struct wifi_nrf_osal_priv *opriv,
				   void *nbuf,
				   unsigned int size)
{
	return opriv->ops->nbuf_data_pull(nbuf,
					  size);
}

unsigned char wifi_nrf_osal_nbuf_get_priority(struct wifi_nrf_osal_priv *opriv,
					      void *nbuf)
{
	return opriv->ops->nbuf_get_priority(nbuf);
}


void *wifi_nrf_osal_tasklet_alloc(struct wifi_nrf_osal_priv *opriv, int type)
{
	return opriv->ops->tasklet_alloc(type);
}


void wifi_nrf_osal_tasklet_free(struct wifi_nrf_osal_priv *opriv,
				void *tasklet)
{
	opriv->ops->tasklet_free(tasklet);
}


void wifi_nrf_osal_tasklet_init(struct wifi_nrf_osal_priv *opriv,
				void *tasklet,
				void (*callbk_fn)(unsigned long),
				unsigned long data)
{
	opriv->ops->tasklet_init(tasklet,
				 callbk_fn,
				 data);
}


void wifi_nrf_osal_tasklet_schedule(struct wifi_nrf_osal_priv *opriv,
				    void *tasklet)
{
	opriv->ops->tasklet_schedule(tasklet);
}


void wifi_nrf_osal_tasklet_kill(struct wifi_nrf_osal_priv *opriv,
				void *tasklet)
{
	opriv->ops->tasklet_kill(tasklet);
}


void wifi_nrf_osal_sleep_ms(struct wifi_nrf_osal_priv *opriv,
			    unsigned int msecs)
{
	opriv->ops->sleep_ms(msecs);
}


void wifi_nrf_osal_delay_us(struct wifi_nrf_osal_priv *opriv,
			    unsigned long usecs)
{
	opriv->ops->delay_us(usecs);
}


unsigned long wifi_nrf_osal_time_get_curr_us(struct wifi_nrf_osal_priv *opriv)
{
	return opriv->ops->time_get_curr_us();
}


unsigned int wifi_nrf_osal_time_elapsed_us(struct wifi_nrf_osal_priv *opriv,
					   unsigned long start_time_us)
{
	return opriv->ops->time_elapsed_us(start_time_us);
}


void *wifi_nrf_osal_bus_pcie_init(struct wifi_nrf_osal_priv *opriv,
				  const char *dev_name,
				  unsigned int vendor_id,
				  unsigned int sub_vendor_id,
				  unsigned int device_id,
				  unsigned int sub_device_id)
{
	return opriv->ops->bus_pcie_init(dev_name,
					 vendor_id,
					 sub_vendor_id,
					 device_id,
					 sub_device_id);
}


void wifi_nrf_osal_bus_pcie_deinit(struct wifi_nrf_osal_priv *opriv,
				   void *os_pcie_priv)
{
	opriv->ops->bus_pcie_deinit(os_pcie_priv);
}


void *wifi_nrf_osal_bus_pcie_dev_add(struct wifi_nrf_osal_priv *opriv,
				     void *os_pcie_priv,
				     void *osal_pcie_dev_ctx)
{
	return opriv->ops->bus_pcie_dev_add(os_pcie_priv,
					    osal_pcie_dev_ctx);

}


void wifi_nrf_osal_bus_pcie_dev_rem(struct wifi_nrf_osal_priv *opriv,
				    void *os_pcie_dev_ctx)
{
	return opriv->ops->bus_pcie_dev_rem(os_pcie_dev_ctx);
}


enum wifi_nrf_status wifi_nrf_osal_bus_pcie_dev_init(struct wifi_nrf_osal_priv *opriv,
						     void *os_pcie_dev_ctx)
{
	return opriv->ops->bus_pcie_dev_init(os_pcie_dev_ctx);

}


void wifi_nrf_osal_bus_pcie_dev_deinit(struct wifi_nrf_osal_priv *opriv,
				       void *os_pcie_dev_ctx)
{
	return opriv->ops->bus_pcie_dev_deinit(os_pcie_dev_ctx);
}


enum wifi_nrf_status wifi_nrf_osal_bus_pcie_dev_intr_reg(struct wifi_nrf_osal_priv *opriv,
							 void *os_pcie_dev_ctx,
							 void *callbk_data,
							 int (*callbk_fn)(void *callbk_data))
{
	return opriv->ops->bus_pcie_dev_intr_reg(os_pcie_dev_ctx,
						 callbk_data,
						 callbk_fn);
}


void wifi_nrf_osal_bus_pcie_dev_intr_unreg(struct wifi_nrf_osal_priv *opriv,
					   void *os_pcie_dev_ctx)
{
	opriv->ops->bus_pcie_dev_intr_unreg(os_pcie_dev_ctx);
}


void *wifi_nrf_osal_bus_pcie_dev_dma_map(struct wifi_nrf_osal_priv *opriv,
					 void *os_pcie_dev_ctx,
					 void *virt_addr,
					 size_t size,
					 enum wifi_nrf_osal_dma_dir dir)
{
	return opriv->ops->bus_pcie_dev_dma_map(os_pcie_dev_ctx,
						virt_addr,
						size,
						dir);
}


void wifi_nrf_osal_bus_pcie_dev_dma_unmap(struct wifi_nrf_osal_priv *opriv,
					  void *os_pcie_dev_ctx,
					  void *dma_addr,
					  size_t size,
					  enum wifi_nrf_osal_dma_dir dir)
{
	opriv->ops->bus_pcie_dev_dma_unmap(os_pcie_dev_ctx,
					   dma_addr,
					   size,
					   dir);
}


void wifi_nrf_osal_bus_pcie_dev_host_map_get(struct wifi_nrf_osal_priv *opriv,
					     void *os_pcie_dev_ctx,
					     struct wifi_nrf_osal_host_map *host_map)
{
	opriv->ops->bus_pcie_dev_host_map_get(os_pcie_dev_ctx,
					      host_map);
}


void *wifi_nrf_osal_bus_qspi_init(struct wifi_nrf_osal_priv *opriv)
{
	return opriv->ops->bus_qspi_init();
}


void wifi_nrf_osal_bus_qspi_deinit(struct wifi_nrf_osal_priv *opriv,
				   void *os_qspi_priv)
{
	opriv->ops->bus_qspi_deinit(os_qspi_priv);
}


void *wifi_nrf_osal_bus_qspi_dev_add(struct wifi_nrf_osal_priv *opriv,
				     void *os_qspi_priv,
				     void *osal_qspi_dev_ctx)
{
	return opriv->ops->bus_qspi_dev_add(os_qspi_priv,
					    osal_qspi_dev_ctx);
}


void wifi_nrf_osal_bus_qspi_dev_rem(struct wifi_nrf_osal_priv *opriv,
				    void *os_qspi_dev_ctx)
{
	opriv->ops->bus_qspi_dev_rem(os_qspi_dev_ctx);
}


enum wifi_nrf_status wifi_nrf_osal_bus_qspi_dev_init(struct wifi_nrf_osal_priv *opriv,
						     void *os_qspi_dev_ctx)
{
	return opriv->ops->bus_qspi_dev_init(os_qspi_dev_ctx);
}


void wifi_nrf_osal_bus_qspi_dev_deinit(struct wifi_nrf_osal_priv *opriv,
				       void *os_qspi_dev_ctx)
{
	opriv->ops->bus_qspi_dev_deinit(os_qspi_dev_ctx);
}


enum wifi_nrf_status wifi_nrf_osal_bus_qspi_dev_intr_reg(struct wifi_nrf_osal_priv *opriv,
							 void *os_qspi_dev_ctx,
							 void *callbk_data,
							 int (*callbk_fn)(void *callbk_data))
{
	return opriv->ops->bus_qspi_dev_intr_reg(os_qspi_dev_ctx,
						 callbk_data,
						 callbk_fn);
}


void wifi_nrf_osal_bus_qspi_dev_intr_unreg(struct wifi_nrf_osal_priv *opriv,
					   void *os_qspi_dev_ctx)
{
	opriv->ops->bus_qspi_dev_intr_unreg(os_qspi_dev_ctx);
}


void wifi_nrf_osal_bus_qspi_dev_host_map_get(struct wifi_nrf_osal_priv *opriv,
					     void *os_qspi_dev_ctx,
					     struct wifi_nrf_osal_host_map *host_map)
{
	opriv->ops->bus_qspi_dev_host_map_get(os_qspi_dev_ctx,
					      host_map);
}


unsigned int wifi_nrf_osal_qspi_read_reg32(struct wifi_nrf_osal_priv *opriv,
					   void *priv,
					   unsigned long addr)
{
	return opriv->ops->qspi_read_reg32(priv,
					   addr);
}


void wifi_nrf_osal_qspi_write_reg32(struct wifi_nrf_osal_priv *opriv,
				    void *priv,
				    unsigned long addr,
				    unsigned int val)
{
	opriv->ops->qspi_write_reg32(priv,
				     addr,
				     val);
}


void wifi_nrf_osal_qspi_cpy_from(struct wifi_nrf_osal_priv *opriv,
				 void *priv,
				 void *dest,
				 unsigned long addr,
				 size_t count)
{
	opriv->ops->qspi_cpy_from(priv,
				  dest,
				  addr,
				  count);
}


void wifi_nrf_osal_qspi_cpy_to(struct wifi_nrf_osal_priv *opriv,
			       void *priv,
			       unsigned long addr,
			       const void *src,
			       size_t count)
{
	opriv->ops->qspi_cpy_to(priv,
				addr,
				src,
				count);
}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
void *wifi_nrf_osal_timer_alloc(struct wifi_nrf_osal_priv *opriv)
{
	return opriv->ops->timer_alloc();
}


void wifi_nrf_osal_timer_free(struct wifi_nrf_osal_priv *opriv,
			      void *timer)
{
	opriv->ops->timer_free(timer);
}


void wifi_nrf_osal_timer_init(struct wifi_nrf_osal_priv *opriv,
			      void *timer,
			      void (*callbk_fn)(unsigned long),
			      unsigned long data)
{
	opriv->ops->timer_init(timer,
			       callbk_fn,
			       data);
}


void wifi_nrf_osal_timer_schedule(struct wifi_nrf_osal_priv *opriv,
				  void *timer,
				  unsigned long duration)
{
	opriv->ops->timer_schedule(timer,
				   duration);
}


void wifi_nrf_osal_timer_kill(struct wifi_nrf_osal_priv *opriv,
			      void *timer)
{
	opriv->ops->timer_kill(timer);
}



int wifi_nrf_osal_bus_qspi_ps_sleep(struct wifi_nrf_osal_priv *opriv,
				    void *os_qspi_priv)
{
	return opriv->ops->bus_qspi_ps_sleep(os_qspi_priv);
}


int wifi_nrf_osal_bus_qspi_ps_wake(struct wifi_nrf_osal_priv *opriv,
				   void *os_qspi_priv)
{
	return opriv->ops->bus_qspi_ps_wake(os_qspi_priv);
}


int wifi_nrf_osal_bus_qspi_ps_status(struct wifi_nrf_osal_priv *opriv,
				     void *os_qspi_priv)
{
	return opriv->ops->bus_qspi_ps_status(os_qspi_priv);
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

void wifi_nrf_osal_assert(struct wifi_nrf_osal_priv *opriv,
			  int test_val,
			  int val,
			  enum wifi_nrf_assert_op_type op,
			  char *msg)
{
	return opriv->ops->assert(test_val, val, op, msg);
}

unsigned int wifi_nrf_osal_strlen(struct wifi_nrf_osal_priv *opriv,
				  const void *str)
{
	return opriv->ops->strlen(str);
}

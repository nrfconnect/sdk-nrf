/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing OPs declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_OPS_H__
#define __OSAL_OPS_H__

#include <stdarg.h>
#include "osal_structs.h"


/**
 * struct wifi_nrf_osal_ops - Ops to be provided by a specific OS implementation.
 * @init: Initialize the OS shim. This is expected to return any OS Shim
 * specific context information that might be needed.
 *
 * @mem_alloc: Allocate memory of @size bytes and return a pointer to the start
 *             of the memory allocated.
 * @mem_zalloc: Allocate memory of @size bytes, zero out the memory and return
 *              a pointer to the start of the zeroed out memory.
 * @mem_free: Free up memory which has been allocated using @mem_alloc or
 *            @mem_zalloc.
 * @mem_cpy: Copy @count number of bytes from @src location in memory to @dest
 *           location in memory.
 * @mem_set: Fill a block of memory of @size bytes starting at @start with a
 *           particular value represented by @val.
 *
 * @iomem_mmap: Map IO memory of @size pointed to by @addr into CPU
 *              space.
 * @iomem_unmap: Unmap IO memory from CPU space that was mapped using
 *               @iomem_mmap.
 * @iomem_read_reg32: Read the value from a 32 bit device register using a memory
 *                    mapped address(@addr).
 * @iomem_write_reg32: Write a 32 bit value (@val) to a 32 bit device register
 *                     using a memory mapped address(@addr).
 * @iomem_cpy_from: Copy a block of data of size @count bytes from memory
 *                  mapped device memory(@src) to host memory(@dest).
 * @iomem_cpy_to: Copy a block of data of size @count bytes from host
 *                memory (@src) to memory mapped device memory(@dest).
 *
 * @spinlock_alloc: Allocate a busy lock.
 * @spinlock_free: Free a busy lock (@lock) allocated by @spinlock_alloc
 * @spinlock_init: Initialize a busy lock (@lock) allocated by @spinlock_alloc.
 * @spinlock_take: Acquire a busy lock (@lock) allocated by @spinlock_alloc.
 * @spinlock_rel: Release a busy lock (@lock) acquired by @spinlock_take.
 *
 * @spinlock_irq_take: Save interrupt states (@flags), disable interrupts and
 *		       take a lock (@lock).
 * @spinlock_irq_rel: Restore interrupt states (@flags) and release lock (@lock)
 *		      acquired using @spinlock_irq_take.
 *
 * @log_dbg: Log a debug message.
 * @log_info: Log an informational message.
 * @log_err: Log an error message.
 *
 * @llist_node_alloc: Allocate a linked list node.
 * @llist_node_free: Free a linked list node which was allocated by
 *                   @llist_node_alloc.
 * @llist_node_data_get: Get the pointer to the data which the linked list node
 *                       points to.
 * @llist_node_data_set: Store the pointer to the data in the linked list node.
 *
 * @llist_alloc: Allocate a linked list.
 * @llist_free: Free a linked list allocated by @llist_alloc.
 * @llist_init: Initialize a linked list allocated by @llist_alloc.
 * @llist_add_node_tail: Add a linked list node allocated by @llist_node_alloc
 *                       to the tail of a linked list allocated by @llist_alloc.
 * @llist_add_node_head: Add a linked list node allocated by @llist_node_alloc
 * 					 to the head of a linked list allocated by @llist_alloc.
 * @llist_get_node_head: Return the head node from a linked list while still
 *                       leaving the node as part of the linked list. If the
 *                       linked list is empty return NULL.
 * @llist_get_node_nxt: Return the node next to the node passed in the
 *                      @llist_node parameter. The node returned is not removed
 *                      from the linked list.
 * @llist_del_node: Remove the node passed in the @llist_node parameter from the
 *                  linked list passed in the @llist parameter.
 * @llist_len: Return the length of the linked list.
 *
 * @nbuf_alloc: Allocate a network buffer of size @size.
 * @nbuf_free: Free a network buffer(@nbuf) which was allocated by @nbuf_alloc.
 * @nbuf_headroom_res: Reserve headroom at the beginning of the data area of a
 *                     network buffer(@nbuf).
 * @nbuf_headroom_get: Get the size of the reserved headroom at the beginning
 *                     of the data area of a network buffer(@nbuf).
 * @nbuf_data_size: Get the size of the data area of a network buffer(@nbuf).
 * @nbuf_data_get: Get the pointer to the data area of a network buffer(@nbuf).
 * @nbuf_data_put: Increase the data area of a network buffer(@nbuf) by @size
 *                 bytes at the end of the area and return the pointer to the
 *                 beginning of the data area.
 * @nbuf_data_push: Increase the data area of a network buffer(@nbuf) by @size
 *                  bytes at the start of the area and return the pointer to the
 *                  beginning of the data area.
 * @nbuf_data_pull: Decrease the data area of a network buffer(@nbuf) by @size
 *                  bytes at the start of the area and return the pointer to the
 *                  beginning of the data area.
 *
 * @tasklet_alloc: Allocate a tasklet structure and return a pointer to it.
 * @tasklet_free: Free a tasklet structure that had been allocated using
 *                @tasklet_alloc.
 * @tasklet_init: Initialize a tasklet structure(@tasklet) that had been
 *                allocated using @tasklet_alloc. Need to pass a callback
 *                function (@callback) which should get invoked when the
 *                tasklet is scheduled and the data(@data) to be passed to the
 *                callback function.
 * @tasklet_schedule: Schedule a tasklet that had been allocated using
 *                    @tasklet_alloc and initialized using @tasklet_init.
 * @tasklet_kill: Terminate a tasklet that had been scheduled
 *                @tasklet_schedule.
 *
 *
 * @sleep_ms: Sleep for @msecs milliseconds.
 * @delay_us: Delay for @usecs microseconds.
 *
 * @time_get_curr_us: Get the current time of the day in microseconds.
 * @time_elapsed_us: Return the time elapsed in microseconds since
 *                   some time instant (@start_time_us).
 *
 * @bus_pcie_reg_drv: This Op will be called when a PCIe device driver is to be
 *                    registered to the OS's PCIe core.
 * @bus_pcie_unreg_drv: This Op will be called when the PCIe device driver is
 *                      to be unregistered from the OS's PCIe core.
 * @bus_pcie_dev_add: This Op will be called when a new PCIe device has
 *		      been detected. This will return a pointer to a OS
 *		      specific PCIe device context.
 * @bus_pcie_dev_rem: This Op will be called when a PCIe device has been
 *		      removed.
 * @bus_pcie_dev_intr_reg: Register the interrupt to the OS for a PCIe device.
 *			   This Op also passes the callback data and the
 *			   function to be called with this data when an
 *                         interrupt is received.
 * @bus_pcie_dev_intr_unreg: Unregister the interrupt that was registered using
 *			     @bus_pcie_dev_intr_reg.
 * @bus_pcie_dev_dma_map: Map host memory of size @size pointed to by the virtual address
 *			  @virt_addr to be used by the PCIe device for DMAing contents.
 *			  The contents are available for DMAing to the device if @dir has a
 *			  value of @WIFI_NRF_OSAL_DMA_DIR_TO_DEV. Conversely the device can DMA
 *			  contents to the host memory if @dir has a value of
 *			  @WIFI_NRF_OSAL_DMA_DIR_FROM_DEV. The function returns the DMA address
 *			  of the mapped memory.
 * @bus_pcie_dev_dma_unmap: Unmap the host memory which was mapped for DMA using
 *			    @bus_pcie_dev_dma_map.
 *			    The address that will be passed to this Op to be unmapped will
 *			    be the DMA address returned by @dma_bus_pcie_dev_map.
 * @bus_pcie_dev_host_map_get: Get the host mapped address for a PCIe device.
 *
 * @timer_alloc: Allocates a timer and returns a pointer.
 * @timer_free: Frees/Deallocates a timer that has been allocated
 *		using @timer_alloc.
 * @timer_init: Initializes a timer that has been allocated using @timer_alloc
 *		Need to pass (@callback) callback function with
 *		the data(@data) to be passed to the callback function,
 *		whenever the timer expires.
 * @timer_schedule: Schedules a timer with a @duration
 *		that has been allocated using @timer_alloc and
 *		initialized with @timer_init.
 * @timer_kill: Kills a timer that has been scheduled @timer_schedule.
 *
 * This structure exposes Ops which need to be implemented by the underlying OS
 * in order for the WLAN driver to work. The Ops can be directly mapped to OS
 * primitives where a one-to-one mapping is available. In case a mapping is not
 * available an equivalent function will need to be implemented and that
 * function will then need to be mapped to the corresponding Op.
 *
 */
struct wifi_nrf_osal_ops {
	void *(*mem_alloc)(size_t size);
	void *(*mem_zalloc)(size_t size);
	void (*mem_free)(void *buf);
	void *(*mem_cpy)(void *dest, const void *src, size_t count);
	void *(*mem_set)(void *start, int val, size_t size);

	void *(*iomem_mmap)(unsigned long addr, unsigned long size);
	void (*iomem_unmap)(volatile void *addr);
	unsigned int (*iomem_read_reg32)(const volatile void *addr);
	void (*iomem_write_reg32)(volatile void *addr, unsigned int val);
	void (*iomem_cpy_from)(void *dest, const volatile void *src, size_t count);
	void (*iomem_cpy_to)(volatile void *dest, const void *src, size_t count);

	unsigned int (*qspi_read_reg32)(void *priv, unsigned long addr);
	void (*qspi_write_reg32)(void *priv, unsigned long addr, unsigned int val);
	void (*qspi_cpy_from)(void *priv, void *dest, unsigned long addr, size_t count);
	void (*qspi_cpy_to)(void *priv, unsigned long addr, const void *src, size_t count);

	void *(*spinlock_alloc)(void);
	void (*spinlock_free)(void *lock);
	void (*spinlock_init)(void *lock);
	void (*spinlock_take)(void *lock);
	void (*spinlock_rel)(void *lock);

	void (*spinlock_irq_take)(void *lock, unsigned long *flags);
	void (*spinlock_irq_rel)(void *lock, unsigned long *flags);

	int (*log_dbg)(const char *fmt, va_list args);
	int (*log_info)(const char *fmt, va_list args);
	int (*log_err)(const char *fmt, va_list args);

	void *(*llist_node_alloc)(void);
	void (*llist_node_free)(void *node);
	void *(*llist_node_data_get)(void *node);
	void (*llist_node_data_set)(void *node, void *data);

	void *(*llist_alloc)(void);
	void (*llist_free)(void *llist);
	void (*llist_init)(void *llist);
	void (*llist_add_node_tail)(void *llist, void *llist_node);
	void (*llist_add_node_head)(void *llist, void *llist_node);
	void *(*llist_get_node_head)(void *llist);
	void *(*llist_get_node_nxt)(void *llist, void *llist_node);
	void (*llist_del_node)(void *llist, void *llist_node);
	unsigned int (*llist_len)(void *llist);

	void *(*nbuf_alloc)(unsigned int size);
	void (*nbuf_free)(void *nbuf);
	void (*nbuf_headroom_res)(void *nbuf, unsigned int size);
	unsigned int (*nbuf_headroom_get)(void *nbuf);
	unsigned int (*nbuf_data_size)(void *nbuf);
	void *(*nbuf_data_get)(void *nbuf);
	void *(*nbuf_data_put)(void *nbuf, unsigned int size);
	void *(*nbuf_data_push)(void *nbuf, unsigned int size);
	void *(*nbuf_data_pull)(void *nbuf, unsigned int size);

	void *(*tasklet_alloc)(int type);
	void (*tasklet_free)(void *tasklet);
	void (*tasklet_init)(void *tasklet,
			     void (*callback)(unsigned long),
			     unsigned long data);
	void (*tasklet_schedule)(void *tasklet);
	void (*tasklet_kill)(void *tasklet);


	int (*sleep_ms)(int msecs);
	int (*delay_us)(int usecs);
	unsigned long (*time_get_curr_us)(void);
	unsigned int (*time_elapsed_us)(unsigned long start_time_us);

	void *(*bus_pcie_init)(const char *dev_name,
			       unsigned int vendor_id,
			       unsigned int sub_vendor_id,
			       unsigned int device_id,
			       unsigned int sub_device_id);
	void (*bus_pcie_deinit)(void *os_pcie_priv);
	void *(*bus_pcie_dev_add)(void *pcie_priv,
				  void *osal_pcie_dev_ctx);
	void (*bus_pcie_dev_rem)(void *os_pcie_dev_ctx);

	enum wifi_nrf_status (*bus_pcie_dev_init)(void *os_pcie_dev_ctx);
	void (*bus_pcie_dev_deinit)(void *os_pcie_dev_ctx);

	enum wifi_nrf_status (*bus_pcie_dev_intr_reg)(void *os_pcie_dev_ctx,
						      void *callbk_data,
						      int (*callback_fn)(void *callbk_data));
	void (*bus_pcie_dev_intr_unreg)(void *os_pcie_dev_ctx);
	void *(*bus_pcie_dev_dma_map)(void *os_pcie_dev_ctx,
				      void *virt_addr,
				      size_t size,
				      enum wifi_nrf_osal_dma_dir dir);
	void (*bus_pcie_dev_dma_unmap)(void *os_pcie_dev_ctx,
				       void *dma_addr,
				       size_t size,
				       enum wifi_nrf_osal_dma_dir dir);
	void (*bus_pcie_dev_host_map_get)(void *os_pcie_dev_ctx,
					  struct wifi_nrf_osal_host_map *host_map);

	void *(*bus_qspi_init)(void);
	void (*bus_qspi_deinit)(void *os_qspi_priv);
	void *(*bus_qspi_dev_add)(void *qspi_priv,
				  void *osal_qspi_dev_ctx);
	void (*bus_qspi_dev_rem)(void *os_qspi_dev_ctx);

	enum wifi_nrf_status (*bus_qspi_dev_init)(void *os_qspi_dev_ctx);
	void (*bus_qspi_dev_deinit)(void *os_qspi_dev_ctx);

	enum wifi_nrf_status (*bus_qspi_dev_intr_reg)(void *os_qspi_dev_ctx,
						      void *callbk_data,
						      int (*callback_fn)(void *callbk_data));
	void (*bus_qspi_dev_intr_unreg)(void *os_qspi_dev_ctx);
	void (*bus_qspi_dev_host_map_get)(void *os_qspi_dev_ctx,
					  struct wifi_nrf_osal_host_map *host_map);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	void *(*timer_alloc)(void);
	void (*timer_free)(void *timer);
	void (*timer_init)(void *timer,
			   void (*callback)(unsigned long),
			   unsigned long data);
	void (*timer_schedule)(void *timer, unsigned long duration);
	void (*timer_kill)(void *timer);
	int (*bus_qspi_ps_sleep)(void *os_qspi_priv);
	int (*bus_qspi_ps_wake)(void *os_qspi_priv);
	int (*bus_qspi_ps_status)(void *os_qspi_priv);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
};


/**
 * get_os_ops() - The OSAL layer expects this Op return a initialized instance
 *                of OS specific Ops.
 *
 * This Op is expected to be implemented by a specific OS shim and is expected
 * to return a pointer to a initialized instance of struct wifi_nrf_osal_ops.
 *
 * Returns: Pointer to instance of OS specific Ops.
 */
const struct wifi_nrf_osal_ops *get_os_ops(void);
#endif /* __OSAL_OPS_H__ */

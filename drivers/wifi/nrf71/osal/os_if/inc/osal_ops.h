/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing OPs declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_OPS_H__
#define __OSAL_OPS_H__

#include "osal_structs.h"


/**
 * @brief struct nrf_wifi_osal_ops - Ops to be provided by a specific OS implementation.
 *
 * This structure exposes Ops which need to be implemented by the underlying OS
 * in order for the WLAN driver to work. The Ops can be directly mapped to OS
 * primitives where a one-to-one mapping is available. In case a mapping is not
 * available, an equivalent function will need to be implemented and that
 * function will then need to be mapped to the corresponding Op.
 */
struct nrf_wifi_osal_ops {
	/**
	 * @brief Allocate memory.
	 *
	 * @param size The size of the memory to allocate for control messages.
	 * @return A pointer to the start of the allocated memory.
	 */
	void *(*mem_alloc)(size_t size);

	/**
	 * @brief Allocate zero-initialized memory for control messages.
	 *
	 * @param size The size of the memory to allocate.
	 * @return A pointer to the start of the allocated memory.
	 */
	void *(*mem_zalloc)(size_t size);

	/**
	 * @brief Free memory allocated for control messages.
	 *
	 * @param buf A pointer to the memory to free.
	 */
	void (*mem_free)(void *buf);

	/**
	 * @brief Allocate zero-initialized memory for data.
	 *
	 * @param size The size of the memory to allocate.
	 * @return A pointer to the start of the allocated memory.
	 */
	void *(*data_mem_zalloc)(size_t size);

	/**
	 * @brief Free memory allocated for data.
	 *
	 * @param buf A pointer to the memory to free.
	 */
	void (*data_mem_free)(void *buf);

	/**
	 * @brief Copy memory.
	 *
	 * @param dest A pointer to the destination memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 * @return A pointer to the destination memory.
	 */
	void *(*mem_cpy)(void *dest, const void *src, size_t count);

	/**
	 * @brief Set memory.
	 *
	 * @param start A pointer to the start of the memory block.
	 * @param val The value to set.
	 * @param size The size of the memory block.
	 * @return A pointer to the start of the memory block.
	 */
	void *(*mem_set)(void *start, int val, size_t size);

	/**
	 * @brief Compare memory.
	 *
	 * @param addr1 A pointer to the first memory block.
	 * @param addr2 A pointer to the second memory block.
	 * @param size The size of the memory blocks.
	 * @return 0 if the memory blocks are equal, a negative value if addr1 is less
	 *		than addr2, a positive value if addr1 is greater than addr2.
	 */
	int (*mem_cmp)(const void *addr1, const void *addr2, size_t size);

	/**
	 * @brief Map IO memory into CPU space.
	 *
	 * @param addr The address of the IO memory.
	 * @param size The size of the IO memory.
	 * @return A pointer to the mapped IO memory.
	 */
	void *(*iomem_mmap)(unsigned long addr, unsigned long size);

	/**
	 * @brief Unmap IO memory from CPU space.
	 *
	 * @param addr A pointer to the mapped IO memory.
	 */
	void (*iomem_unmap)(volatile void *addr);

	/**
	 * @brief Read a 32-bit value from a device register using a memory mapped address.
	 *
	 * @param addr A pointer to the memory mapped address.
	 * @return The value read from the device register.
	 */
	unsigned int (*iomem_read_reg32)(const volatile void *addr);

	/**
	 * @brief Write a 32-bit value to a device register using a memory mapped address.
	 *
	 * @param addr A pointer to the memory mapped address.
	 * @param val The value to write to the device register.
	 */
	void (*iomem_write_reg32)(volatile void *addr, unsigned int val);

	/**
	 * @brief Copy data from memory mapped device memory to host memory.
	 *
	 * @param dest A pointer to the destination memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 */
	void (*iomem_cpy_from)(void *dest, const volatile void *src, size_t count);

	/**
	 * @brief Copy data from host memory to memory mapped device memory.
	 *
	 * @param dest A pointer to the destination memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 */
	void (*iomem_cpy_to)(volatile void *dest, const void *src, size_t count);

	/**
	 * @brief Read a 32-bit value from a QSPI device register.
	 *
	 * @param priv A pointer to the QSPI device private data.
	 * @param addr The address of the device register.
	 * @return The value read from the device register.
	 */
	unsigned int (*qspi_read_reg32)(void *priv, unsigned long addr);

	/**
	 * @brief Write a 32-bit value to a QSPI device register.
	 *
	 * @param priv A pointer to the QSPI device private data.
	 * @param addr The address of the device register.
	 * @param val The value to write to the device register.
	 */
	void (*qspi_write_reg32)(void *priv, unsigned long addr, unsigned int val);

	/**
	 * @brief Copy data from QSPI device memory to host memory.
	 *
	 * @param priv A pointer to the QSPI device private data.
	 * @param dest A pointer to the destination memory.
	 * @param addr The address of the device memory.
	 * @param count The number of bytes to copy.
	 */
	void (*qspi_cpy_from)(void *priv, void *dest, unsigned long addr, size_t count);

	/**
	 * @brief Copy data from host memory to QSPI device memory.
	 *
	 * @param priv A pointer to the QSPI device private data.
	 * @param addr The address of the device memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 */
	void (*qspi_cpy_to)(void *priv, unsigned long addr, const void *src, size_t count);

	/**
	 * @brief Read a 32-bit value from a SPI device register.
	 *
	 * @param priv A pointer to the SPI device private data.
	 * @param addr The address of the device register.
	 * @return The value read from the device register.
	 */
	unsigned int (*spi_read_reg32)(void *priv, unsigned long addr);

	/**
	 * @brief Write a 32-bit value to a SPI device register.
	 *
	 * @param priv A pointer to the SPI device private data.
	 * @param addr The address of the device register.
	 * @param val The value to write to the device register.
	 */
	void (*spi_write_reg32)(void *priv, unsigned long addr, unsigned int val);

	/**
	 * @brief Copy data from SPI device memory to host memory.
	 *
	 * @param priv A pointer to the SPI device private data.
	 * @param dest A pointer to the destination memory.
	 * @param addr The address of the device memory.
	 * @param count The number of bytes to copy.
	 */
	void (*spi_cpy_from)(void *priv, void *dest, unsigned long addr, size_t count);

	/**
	 * @brief Copy data from host memory to SPI device memory.
	 *
	 * @param priv A pointer to the SPI device private data.
	 * @param addr The address of the device memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 */
	void (*spi_cpy_to)(void *priv, unsigned long addr, const void *src, size_t count);

	/**
	 * @brief Allocate a spinlock.
	 *
	 * @return A pointer to the allocated spinlock.
	 */
	void *(*spinlock_alloc)(void);

	/**
	 * @brief Free a spinlock.
	 *
	 * @param lock A pointer to the spinlock to free.
	 */
	void (*spinlock_free)(void *lock);

	/**
	 * @brief Initialize a spinlock.
	 *
	 * @param lock A pointer to the spinlock to initialize.
	 */
	void (*spinlock_init)(void *lock);

	/**
	 * @brief Acquire a spinlock.
	 *
	 * @param lock A pointer to the spinlock to acquire.
	 */
	void (*spinlock_take)(void *lock);

	/**
	 * @brief Release a spinlock.
	 *
	 * @param lock A pointer to the spinlock to release.
	 */
	void (*spinlock_rel)(void *lock);

	/**
	 * @brief Save interrupt states, disable interrupts, and acquire a spinlock.
	 *
	 * @param lock A pointer to the spinlock to acquire.
	 * @param flags A pointer to store the saved interrupt states.
	 */
	void (*spinlock_irq_take)(void *lock, unsigned long *flags);

	/**
	 * @brief Restore interrupt states and release a spinlock.
	 *
	 * @param lock A pointer to the spinlock to release.
	 * @param flags A pointer to the saved interrupt states.
	 */
	void (*spinlock_irq_rel)(void *lock, unsigned long *flags);

	/**
	 * @brief Log a debug message.
	 *
	 * @param fmt The format string of the message.
	 * @param args The arguments for the format string.
	 * @return The number of characters written.
	 */
	int (*log_dbg)(const char *fmt, va_list args);

	/**
	 * @brief Log an informational message.
	 *
	 * @param fmt The format string of the message.
	 * @param args The arguments for the format string.
	 * @return The number of characters written.
	 */
	int (*log_info)(const char *fmt, va_list args);

	/**
	 * @brief Log an error message.
	 *
	 * @param fmt The format string of the message.
	 * @param args The arguments for the format string.
	 * @return The number of characters written.
	 */
	int (*log_err)(const char *fmt, va_list args);

	/**
	 * @brief Allocate a linked list node.
	 *
	 * @return A pointer to the allocated linked list node.
	 */
	void *(*llist_node_alloc)(void);

	/**
	 * @brief Allocate a linked list node from control pool.
	 *
	 * @return A pointer to the allocated linked list node.
	 */
	 void *(*ctrl_llist_node_alloc)(void);

	/**
	 * @brief Free a linked list node.
	 *
	 * @param node A pointer to the linked list node to free.
	 */
	void (*llist_node_free)(void *node);

	/**
	 * @brief Free a linked list node from control pool.
	 *
	 * @param node A pointer to the linked list node to free.
	 */
	 void (*ctrl_llist_node_free)(void *node);

	/**
	 * @brief Get the pointer to the data which the linked list node points to.
	 *
	 * @param node A pointer to the linked list node.
	 * @return A pointer to the data.
	 */
	void *(*llist_node_data_get)(void *node);

	/**
	 * @brief Store the pointer to the data in the linked list node.
	 *
	 * @param node A pointer to the linked list node.
	 * @param data A pointer to the data.
	 */
	void (*llist_node_data_set)(void *node, void *data);

	/**
	 * @brief Allocate a linked list.
	 *
	 * @return A pointer to the allocated linked list.
	 */
	void *(*llist_alloc)(void);

	/**
	 * @brief Allocate a linked list for control path.
	 *
	 * @return A pointer to the allocated linked list.
	 */
	void *(*ctrl_llist_alloc)(void);

	/**
	 * @brief Free a linked list.
	 *
	 * @param llist A pointer to the linked list to free.
	 */
	void (*llist_free)(void *llist);

	/**
	 * @brief Free a linked list for control path.
	 *
	 * @param llist A pointer to the linked list to free.
	 */
	void (*ctrl_llist_free)(void *llist);

	/**
	 * @brief Initialize a linked list.
	 *
	 * @param llist A pointer to the linked list to initialize.
	 */
	void (*llist_init)(void *llist);

	/**
	 * @brief Add a linked list node to the tail of a linked list.
	 *
	 * @param llist A pointer to the linked list.
	 * @param llist_node A pointer to the linked list node to add.
	 */
	void (*llist_add_node_tail)(void *llist, void *llist_node);

	/**
	 * @brief Add a linked list node to the head of a linked list.
	 *
	 * @param llist A pointer to the linked list.
	 * @param llist_node A pointer to the linked list node to add.
	 */
	void (*llist_add_node_head)(void *llist, void *llist_node);

	/**
	 * @brief Return the head node from a linked list.
	 *
	 * @param llist A pointer to the linked list.
	 * @return A pointer to the head node, or NULL if the linked list is empty.
	 */
	void *(*llist_get_node_head)(void *llist);

	/**
	 * @brief Return the node next to the given node in the linked list.
	 *
	 * @param llist A pointer to the linked list.
	 * @param llist_node A pointer to the current node.
	 * @return A pointer to the next node.
	 */
	void *(*llist_get_node_nxt)(void *llist, void *llist_node);

	/**
	 * @brief Remove a node from the linked list.
	 *
	 * @param llist A pointer to the linked list.
	 * @param llist_node A pointer to the node to remove.
	 */
	void (*llist_del_node)(void *llist, void *llist_node);

	/**
	 * @brief Return the length of the linked list.
	 *
	 * @param llist A pointer to the linked list.
	 * @return The length of the linked list.
	 */
	unsigned int (*llist_len)(void *llist);

	/**
	 * @brief Allocate a network buffer.
	 *
	 * @param size The size of the network buffer.
	 * @return A pointer to the allocated network buffer.
	 */
	void *(*nbuf_alloc)(unsigned int size);

	/**
	 * @brief Free a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer to free.
	 */
	void (*nbuf_free)(void *nbuf);

	/**
	 * @brief Reserve headroom at the beginning of the data area of a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @param size The size of the headroom to reserve.
	 */
	void (*nbuf_headroom_res)(void *nbuf, unsigned int size);

	/**
	 * @brief Get the size of the reserved headroom at the beginning of the
	 * data area of a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @return The size of the reserved headroom.
	 */
	unsigned int (*nbuf_headroom_get)(void *nbuf);

	/**
	 * @brief Get the size of the data area of a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @return The size of the data area.
	 */
	unsigned int (*nbuf_data_size)(void *nbuf);

	/**
	 * @brief Get the pointer to the data area of a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @return A pointer to the data area.
	 */
	void *(*nbuf_data_get)(void *nbuf);

	/**
	 * @brief Increase the data area of a network buffer at the end of the area.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @param size The size to increase the data area by.
	 * @return A pointer to the beginning of the data area.
	 */
	void *(*nbuf_data_put)(void *nbuf, unsigned int size);

	/**
	 * @brief Increase the data area of a network buffer at the start of the area.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @param size The size to increase the data area by.
	 * @return A pointer to the beginning of the data area.
	 */
	void *(*nbuf_data_push)(void *nbuf, unsigned int size);

	/**
	 * @brief Decrease the data area of a network buffer at the start of the area.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @param size The size to decrease the data area by.
	 * @return A pointer to the beginning of the data area.
	 */
	void *(*nbuf_data_pull)(void *nbuf, unsigned int size);

	/**
	 * @brief Get the priority of a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @return The priority of the network buffer.
	 */
	unsigned char (*nbuf_get_priority)(void *nbuf);

	/**
	 * @brief Get the checksum status of a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @return The checksum status of the network buffer.
	 */
	unsigned char (*nbuf_get_chksum_done)(void *nbuf);

	/**
	 * @brief Set the checksum status of a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @param chksum_done The checksum status to set.
	 */
	void (*nbuf_set_chksum_done)(void *nbuf, unsigned char chksum_done);
#if defined(NRF71_RAW_DATA_TX) || defined(__DOXYGEN__)
	/**
	 * @brief Set the raw Tx header in a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @param raw_hdr_len The length of the raw header to set.
	 */
	void *(*nbuf_set_raw_tx_hdr)(void *nbuf, unsigned short raw_hdr_len);
	/**
	 * @brief Get the raw Tx header from a network buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 * @return A pointer to the raw Tx header.
	 */
	void *(*nbuf_get_raw_tx_hdr)(void *nbuf);

	/**
	 * @brief Check if the network buffer is a raw Tx buffer.
	 *
	 * @param nbuf A pointer to the network buffer.
	 *
	 * @return true if it is a raw Tx buffer, false otherwise.
	 */
	bool (*nbuf_is_raw_tx)(void *nbuf);
#endif /* NRF71_RAW_DATA_TX || __DOXYGEN__ */
	/**
	 * @brief Allocate a tasklet structure.
	 *
	 * @param type The type of the tasklet.
	 * @return A pointer to the allocated tasklet structure.
	 */
	void *(*tasklet_alloc)(int type);

	/**
	 * @brief Free a tasklet structure.
	 *
	 * @param tasklet A pointer to the tasklet structure to free.
	 */
	void (*tasklet_free)(void *tasklet);

	/**
	 * @brief Initialize a tasklet structure.
	 *
	 * @param tasklet A pointer to the tasklet structure to initialize.
	 * @param callback The callback function to be invoked when the tasklet is scheduled.
	 * @param data The data to be passed to the callback function.
	 */
	void (*tasklet_init)(void *tasklet, void (*callback)(unsigned long), unsigned long data);

	/**
	 * @brief Schedule a tasklet.
	 *
	 * @param tasklet A pointer to the tasklet to schedule.
	 */
	void (*tasklet_schedule)(void *tasklet);

	/**
	 * @brief Terminate a tasklet.
	 *
	 * @param tasklet A pointer to the tasklet to terminate.
	 */
	void (*tasklet_kill)(void *tasklet);

	/**
	 * @brief Sleep for a specified number of milliseconds.
	 *
	 * @param msecs The number of milliseconds to sleep.
	 * @return 0 on success, a negative value on failure.
	 */
	int (*sleep_ms)(int msecs);

	/**
	 * @brief Delay for a specified number of microseconds.
	 *
	 * @param usecs The number of microseconds to delay.
	 * @return 0 on success, a negative value on failure.
	 */
	int (*delay_us)(int usecs);

	/**
	 * @brief Get the current time of the day in microseconds.
	 *
	 * @return The current time of the day in microseconds.
	 */
	unsigned long (*time_get_curr_us)(void);

	/**
	 * @brief Return the time elapsed in microseconds since a specified time instant.
	 *
	 * @param start_time The time instant to measure the elapsed time from.
	 * @return The time elapsed in microseconds.
	 */
	unsigned int (*time_elapsed_us)(unsigned long start_time);

	/** @brief Get the current time of the day in milliseconds.
	 *
	 * @return The current time of the day in milliseconds.
	 */
	unsigned long (*time_get_curr_ms)(void);

	/**
	 * @brief Return the time elapsed in milliseconds since a specified time instant.
	 *
	 * @param start_time The time instant to measure the elapsed time from.
	 * @return The time elapsed in milliseconds.
	 */
	unsigned int (*time_elapsed_ms)(unsigned long start_time_us);

	/**
	 * @brief Initialize the PCIe bus.
	 *
	 * @param dev_name The name of the PCIe device.
	 * @param vendor_id The vendor ID of the PCIe device.
	 * @param sub_vendor_id The sub-vendor ID of the PCIe device.
	 * @param device_id The device ID of the PCIe device.
	 * @param sub_device_id The sub-device ID of the PCIe device.
	 * @return A pointer to the initialized PCIe bus.
	 */
	void *(*bus_pcie_init)(const char *dev_name,
						   unsigned int vendor_id,
						   unsigned int sub_vendor_id,
						   unsigned int device_id,
						   unsigned int sub_device_id);

	/**
	 * @brief Deinitialize the PCIe bus.
	 *
	 * @param os_pcie_priv A pointer to the PCIe bus.
	 */
	void (*bus_pcie_deinit)(void *os_pcie_priv);

	/**
	 * @brief Add a PCIe device to the bus.
	 *
	 * @param pcie_priv A pointer to the PCIe bus.
	 * @param osal_pcie_dev_ctx A pointer to the PCIe device context.
	 * @return A pointer to the added PCIe device.
	 */
	void *(*bus_pcie_dev_add)(void *pcie_priv,
							  void *osal_pcie_dev_ctx);

	/**
	 * @brief Remove a PCIe device from the bus.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 */
	void (*bus_pcie_dev_rem)(void *os_pcie_dev_ctx);

	/**
	 * @brief Initialize a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @return The status of the initialization.
	 */
	enum nrf_wifi_status (*bus_pcie_dev_init)(void *os_pcie_dev_ctx);

	/**
	 * @brief Deinitialize a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 */
	void (*bus_pcie_dev_deinit)(void *os_pcie_dev_ctx);

	/**
	 * @brief Register an interrupt handler for a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @param callbk_data The data to be passed to the callback function.
	 * @param callback_fn The callback function to be invoked when an interrupt occurs.
	 * @return The status of the registration.
	 */
	enum nrf_wifi_status (*bus_pcie_dev_intr_reg)(void *os_pcie_dev_ctx,
			void *callbk_data,
			int (*callback_fn)(void *callbk_data));

	/**
	 * @brief Unregister the interrupt handler for a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 */
	void (*bus_pcie_dev_intr_unreg)(void *os_pcie_dev_ctx);

	/**
	 * @brief Map a DMA buffer for a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @param virt_addr The virtual address of the buffer.
	 * @param size The size of the buffer.
	 * @param dir The direction of the DMA transfer.
	 * @return A pointer to the mapped DMA buffer.
	 */
	void *(*bus_pcie_dev_dma_map)(void *os_pcie_dev_ctx,
								  void *virt_addr,
								  size_t size,
								  enum nrf_wifi_osal_dma_dir dir);

	/**
	 * @brief Unmap a DMA buffer for a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @param dma_addr The DMA address of the buffer.
	 * @param size The size of the buffer.
	 * @param dir The direction of the DMA transfer.
	 */
	void (*bus_pcie_dev_dma_unmap)(void *os_pcie_dev_ctx,
								   void *dma_addr,
								   size_t size,
								   enum nrf_wifi_osal_dma_dir dir);

	/**
	 * @brief Get the host mapping of a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @param host_map A pointer to the host mapping structure.
	 */
	void (*bus_pcie_dev_host_map_get)(void *os_pcie_dev_ctx,
			struct nrf_wifi_osal_host_map *host_map);

	/**
	 * @brief Initialize the QSPI bus.
	 *
	 * @return A pointer to the initialized QSPI bus.
	 */
	void *(*bus_qspi_init)(void);

	/**
	 * @brief Deinitialize the QSPI bus.
	 *
	 * @param os_qspi_priv A pointer to the QSPI bus.
	 */
	void (*bus_qspi_deinit)(void *os_qspi_priv);

	/**
	 * @brief Add a QSPI device to the bus.
	 *
	 * @param qspi_priv A pointer to the QSPI bus.
	 * @param osal_qspi_dev_ctx A pointer to the QSPI device context.
	 * @return A pointer to the added QSPI device.
	 */
	void *(*bus_qspi_dev_add)(void *qspi_priv,
							  void *osal_qspi_dev_ctx);

	/**
	 * @brief Remove a QSPI device from the bus.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 */
	void (*bus_qspi_dev_rem)(void *os_qspi_dev_ctx);

	/**
	 * @brief Initialize a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 * @return The status of the initialization.
	 */
	enum nrf_wifi_status (*bus_qspi_dev_init)(void *os_qspi_dev_ctx);

	/**
	 * @brief Deinitialize a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 */
	void (*bus_qspi_dev_deinit)(void *os_qspi_dev_ctx);

	/**
	 * @brief Register an interrupt handler for a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 * @param callbk_data The data to be passed to the callback function.
	 * @param callback_fn The callback function to be invoked when an interrupt occurs.
	 * @return The status of the registration.
	 */
	enum nrf_wifi_status (*bus_qspi_dev_intr_reg)(void *os_qspi_dev_ctx,
			void *callbk_data,
			int (*callback_fn)(void *callbk_data));

	/**
	 * @brief Unregister the interrupt handler for a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 */
	void (*bus_qspi_dev_intr_unreg)(void *os_qspi_dev_ctx);

	/**
	 * @brief Get the host mapping of a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 * @param host_map A pointer to the host mapping structure.
	 */
	void (*bus_qspi_dev_host_map_get)(void *os_qspi_dev_ctx,
			struct nrf_wifi_osal_host_map *host_map);

	/**
	 * @brief Initialize the SPI bus.
	 *
	 * @return A pointer to the initialized SPI bus.
	 */
	void *(*bus_spi_init)(void);

	/**
	 * @brief Deinitialize the SPI bus.
	 *
	 * @param os_spi_priv A pointer to the SPI bus.
	 */
	void (*bus_spi_deinit)(void *os_spi_priv);

	/**
	 * @brief Add a SPI device to the bus.
	 *
	 * @param spi_priv A pointer to the SPI bus.
	 * @param osal_spi_dev_ctx A pointer to the SPI device context.
	 * @return A pointer to the added SPI device.
	 */
	void *(*bus_spi_dev_add)(void *spi_priv,
							 void *osal_spi_dev_ctx);

	/**
	 * @brief Remove a SPI device from the bus.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 */
	void (*bus_spi_dev_rem)(void *os_spi_dev_ctx);

	/**
	 * @brief Initialize a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 * @return The status of the initialization.
	 */
	enum nrf_wifi_status (*bus_spi_dev_init)(void *os_spi_dev_ctx);

	/**
	 * @brief Deinitialize a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 */
	void (*bus_spi_dev_deinit)(void *os_spi_dev_ctx);

	/**
	 * @brief Register an interrupt handler for a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 * @param callbk_data The data to be passed to the callback function.
	 * @param callback_fn The callback function to be invoked when an interrupt occurs.
	 * @return The status of the registration.
	 */
	enum nrf_wifi_status (*bus_spi_dev_intr_reg)(void *os_spi_dev_ctx,
			void *callbk_data,
			int (*callback_fn)(void *callbk_data));

	/**
	 * @brief Unregister the interrupt handler for a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 */
	void (*bus_spi_dev_intr_unreg)(void *os_spi_dev_ctx);

	/**
	 * @brief Get the host mapping of a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 * @param host_map A pointer to the host mapping structure.
	 */
	void (*bus_spi_dev_host_map_get)(void *os_spi_dev_ctx,
			struct nrf_wifi_osal_host_map *host_map);

	#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
	/**
	 * @brief Allocate a timer.
	 *
	 * @return A pointer to the allocated timer.
	 */
	void *(*timer_alloc)(void);

	/**
	 * @brief Free a timer.
	 *
	 * @param timer A pointer to the timer to free.
	 */
	void (*timer_free)(void *timer);

	/**
	 * @brief Initialize a timer.
	 *
	 * @param timer A pointer to the timer to initialize.
	 * @param callback The callback function to be invoked when the timer expires.
	 * @param data The data to be passed to the callback function.
	 */
	void (*timer_init)(void *timer,
					   void (*callback)(unsigned long),
					   unsigned long data);

	/**
	 * @brief Schedule a timer.
	 *
	 * @param timer A pointer to the timer to schedule.
	 * @param duration The duration of the timer in milliseconds.
	 */
	void (*timer_schedule)(void *timer, unsigned long duration);

	/**
	 * @brief Terminate a timer.
	 *
	 * @param timer A pointer to the timer to terminate.
	 */
	void (*timer_kill)(void *timer);

	/**
	 * @brief Put the QSPI bus to sleep.
	 *
	 * @param os_qspi_priv A pointer to the QSPI bus.
	 * @return 0 on success, a negative value on failure.
	 */
	int (*bus_qspi_ps_sleep)(void *os_qspi_priv);

	/**
	 * @brief Wake up the QSPI bus from sleep.
	 *
	 * @param os_qspi_priv A pointer to the QSPI bus.
	 * @return 0 on success, a negative value on failure.
	 */
	int (*bus_qspi_ps_wake)(void *os_qspi_priv);

	/**
	 * @brief Get the power state of the QSPI bus.
	 *
	 * @param os_qspi_priv A pointer to the QSPI bus.
	 * @return The power state of the QSPI bus.
	 */
	int (*bus_qspi_ps_status)(void *os_qspi_priv);
	#endif /* NRF_WIFI_LOW_POWER */

	/**
	 * @brief Assert a condition and display an error message if the condition is false.
	 *
	 * @param test_val The value to test.
	 * @param val The value to compare against.
	 * @param op The comparison operator.
	 * @param assert_msg The error message to display.
	 */
	void (*assert)(int test_val,
				   int val,
				   enum nrf_wifi_assert_op_type op,
				   char *assert_msg);

	/**
	 * @brief Get the length of a string.
	 *
	 * @param str A pointer to the string.
	 * @return The length of the string.
	 */
	unsigned int (*strlen)(const void *str);

	/**
	 * @brief Get a random 8-bit value.
	 *
	 * @return A random 8-bit value.
	 */
	unsigned char (*rand8_get)(void);
	int (*ipc_send_msg)(unsigned int msg_type, void *msg, unsigned int msg_len);
};
#endif /* __OSAL_OPS_H__ */

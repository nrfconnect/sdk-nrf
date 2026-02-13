/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_API_H__
#define __OSAL_API_H__

#include "osal_structs.h"

/* Have to match zephyr/include/zephyr/logging/log_core.h */
#define NRF_WIFI_LOG_LEVEL_ERR 1U
#define NRF_WIFI_LOG_LEVEL_INF 3U
#define NRF_WIFI_LOG_LEVEL_DBG 4U

#ifndef WIFI_NRF71_LOG_LEVEL
#define WIFI_NRF71_LOG_LEVEL NRF_WIFI_LOG_LEVEL_ERR
#endif

#ifndef NRF71_LOG_VERBOSE
#define __func__ "<snipped>"
#endif /* NRF71_LOG_VERBOSE */

/**
 * @brief Initialize the OSAL layer.
 * @param ops: Pointer to the OSAL operations structure.
 *
 * Initializes the OSAL layer and is expected to be called
 * before using the OSAL layer.
 */
void nrf_wifi_osal_init(const struct nrf_wifi_osal_ops *ops);

/**
 * @brief Deinitialize the OSAL layer.
 *
 * Deinitialize the OSAL layer and is expected to be called after done using
 * the OSAL layer.
 */
void nrf_wifi_osal_deinit(void);

/**
 * @brief Allocate memory for control path requests.
 * @param size Size of the memory to be allocated in bytes.
 *
 * Allocates memory of @p size bytes and returns a pointer to the start
 * of the memory allocated.
 *
 * @return Pointer to start of allocated memory on success, NULL on error.
 */
void *nrf_wifi_osal_mem_alloc(size_t size);

/**
 * @brief Allocated zero-initialized memory for control path requests.
 * @param size Size of the memory to be allocated in bytes.
 *
 * Allocates zero-initialized memory of @p size bytes and returns a pointer to the start
 * of the memory allocated.
 *
 * @return Pointer to start of allocated memory on success, NULL on error.
 */
void *nrf_wifi_osal_mem_zalloc(size_t size);

/**
 * @brief Allocated zero-initialized memory for data.
 *
 * @param size Size of the memory to be allocated in bytes.
 *
 * Allocates memory of @p size bytes, zeroes it out and returns a pointer to the
 * start of the memory allocated.
 *
 * @return: Pointer to start of allocated memory or NULL.
 */
void *nrf_wifi_osal_data_mem_zalloc(size_t size);

/**
 * @brief Free previously allocated memory for control path requests.
 * @param buf Pointer to the memory to be freed.
 *
 * Free up memory which has been allocated using  @ref nrf_wifi_osal_mem_alloc or
 * @ref nrf_wifi_osal_mem_zalloc.
 */
void nrf_wifi_osal_mem_free(void *buf);

/**
 * @brief Free previously allocated memory for data.
 *
 * @param buf Pointer to the memory to be freed.
 *
 * Free up memory which has been allocated using @ref nrf_wifi_osal_mem_alloc or
 * @ref nrf_wifi_osal_mem_zalloc.
 *
 */
void nrf_wifi_osal_data_mem_free(void *buf);

/**
 * @brief Copy contents from one memory location to another.
 *
 * @param dest Pointer to the memory location where contents are to be copied.
 * @param src Pointer to the memory location from where contents are to be copied.
 * @param count Number of bytes to be copied.
 *
 * Copies @p count number of bytes from @p src location in memory to @p dest
 * location in memory.
 *
 * @return Pointer to destination memory if successful, NULL otherwise.
 */
void *nrf_wifi_osal_mem_cpy(void *dest,
			    const void *src,
			    size_t count);

/**
 * @brief Fill a block of memory with a particular value.
 * @param start Pointer to the memory location whose contents are to be set.
 * @param val Value to be set.
 * @param size Number of bytes to be set.
 *
 * Fills a block of memory of @p size bytes, starting at @p start with a value
 * specified by @p val.
 *
 * @return Pointer to memory location which was set on success, NULL on error.
 */
void *nrf_wifi_osal_mem_set(void *start,
			    int val,
			    size_t size);


/**
 * @brief Memory map IO memory into CPU space.
 * @param addr Address of the IO memory to be mapped.
 * @param size Size of the IO memory in bytes.
 *
 * Maps IO memory of @p size bytes pointed to by @p addr into CPU space.
 *
 * @return Pointer to the mapped IO memory on success, NULL on error.
 */
void *nrf_wifi_osal_iomem_mmap(unsigned long addr,
			       unsigned long size);

/**
 * @brief Unmap previously mapped IO memory from CPU space.
 * @param addr Pointer to mapped IO memory to be unmapped.
 *
 * Unmaps IO memory from CPU space that was mapped using nrf_wifi_osal_iomem_mmap.
 */
void nrf_wifi_osal_iomem_unmap(volatile void *addr);

/**
 * @brief Read value from a 32 bit IO memory mapped register.
 * @param addr Pointer to the IO memory mapped register address.
 *
 * @return 32 bit value read from register.
 */
unsigned int nrf_wifi_osal_iomem_read_reg32(const volatile void *addr);

/**
 * @brief Write a 32 bit value to a IO memory mapped register.
 * @param addr Pointer to the IO memory mapped register address.
 * @param val Value to be written to the register.
 *
 * Writes a 32 bit value (val) to a 32 bit device register using a memory
 * mapped address (addr).
 */
void nrf_wifi_osal_iomem_write_reg32(volatile void *addr,
				     unsigned int val);

/**
 * @brief Copy data from the memory of a memory mapped IO device to host memory.
 * @param dest Pointer to the host memory where data is to be copied.
 * @param src Pointer to the memory of the memory mapped IO device from where data is to be copied.
 * @param count The size of the data to be copied in bytes.
 *
 * Copies a block of data of size  count bytes from memory mapped device memory(src)
 * to host memory(dest).
 */
void nrf_wifi_osal_iomem_cpy_from(void *dest,
				  const volatile void *src,
				  size_t count);

/**
 * @brief Copy data to the memory of a memory mapped IO device from host memory.
 * @param dest: Pointer to the memory of the memory mapped IO device where data is to be copied.
 * @param src: Pointer to the host memory from where data is to be copied.
 * @param count: The size of the data to be copied in bytes.
 *
 * Copies a block of data of size  count bytes from host memory (src) to memory mapped
 * device memory(dest).
 */
void nrf_wifi_osal_iomem_cpy_to(volatile void *dest,
				const void *src,
				size_t count);


/**
 * @brief Allocate a busy lock.
 *
 * Allocates a busy lock.
 *
 * @return Pointer to the busy lock instance on success, NULL on error.
 */
void *nrf_wifi_osal_spinlock_alloc(void);

/**
 * @brief Free a busy lock.
 * @param lock Pointer to a busy lock instance.
 *
 * Frees a busy lock (lock) allocated by nrf_wifi_osal_spinlock_alloc.
 */
void nrf_wifi_osal_spinlock_free(void *lock);


/**
 * @brief Initialize a busy lock.
 * @param lock Pointer to a busy lock instance.
 *
 * Initialize a busy lock (lock) allocated by  nrf_wifi_osal_spinlock_alloc.
 */
void nrf_wifi_osal_spinlock_init(void *lock);


/**
 * @brief Acquire a busy lock.
 * @param lock: Pointer to a busy lock instance.
 *
 * Acquires a busy lock (lock) allocated by nrf_wifi_osal_spinlock_alloc.
 */
void nrf_wifi_osal_spinlock_take(void *lock);


/**
 * @brief Releases a busy lock.
 * @param lock: Pointer to a busy lock instance.
 *
 * Releases a busy lock (lock) acquired by nrf_wifi_osal_spinlock_take.
 */
void nrf_wifi_osal_spinlock_rel(void *lock);


/**
 * @brief Acquire a busy lock and disable interrupts.
 * @param lock Pointer to a busy lock instance.
 * @param flags Interrupt state flags.
 *
 * Saves interrupt states (@p flags), disable interrupts and takes a
 * busy lock (@p lock).
 */
void nrf_wifi_osal_spinlock_irq_take(void *lock,
				     unsigned long *flags);


/**
 * @brief Release a busy lock and enable interrupts.
 * @param lock Pointer to a busy lock instance.
 * @param flags Interrupt state flags.
 *
 * Restores interrupt states (@p flags) and releases busy lock (@p lock) acquired
 * using nrf_wifi_osal_spinlock_irq_take.
 */
void nrf_wifi_osal_spinlock_irq_rel(void *lock,
				    unsigned long *flags);


#if WIFI_NRF71_LOG_LEVEL >= NRF_WIFI_LOG_LEVEL_DBG
/**
 * @brief Log a debug message.
 * @param fmt: Format string.
 * @param ...: Variable arguments.
 *
 * Logs a debug message.
 *
 * @return Number of characters of the message logged.
 */
int nrf_wifi_osal_log_dbg(const char *fmt, ...);
#else
#define nrf_wifi_osal_log_dbg(fmt, ...)
#endif

#if WIFI_NRF71_LOG_LEVEL >= NRF_WIFI_LOG_LEVEL_INF
/**
 * @brief Logs an informational message.
 * @param fmt Format string.
 * @param ... Variable arguments.
 *
 * Logs an informational message.
 *
 * @return Number of characters of the message logged.
 */
int nrf_wifi_osal_log_info(const char *fmt, ...);
#else
#define nrf_wifi_osal_log_info(fmt, ...)
#endif

#if WIFI_NRF71_LOG_LEVEL >= NRF_WIFI_LOG_LEVEL_ERR
/**
 * @brief Logs an error message.
 * @param fmt Format string.
 * @param ... Variable arguments.
 *
 * Logs an error message.
 *
 * @return Number of characters of the message logged.
 */
int nrf_wifi_osal_log_err(const char *fmt, ...);
#else
#define nrf_wifi_osal_log_err(fmt, ...)
#endif

/**
 * @brief Allocate a linked list node.
 *
 * Allocate a linked list node.
 *
 * @return Pointer to the linked list node allocated.
 *         NULL if there is an error.
 */
void *nrf_wifi_osal_llist_node_alloc(void);

/**
 * @brief Allocate a linked list node from control pool.
 *
 * Allocate a linked list node from control pool.
 *
 * @return Pointer to the linked list node allocated.
 *         NULL if there is an error.
 */
void *nrf_wifi_osal_ctrl_llist_node_alloc(void);

/**
 * @brief Free a linked list node.
 * @param node Pointer to a linked list node.
 *
 * Frees a linked list node(node) which was allocated by nrf_wifi_osal_llist_node_alloc.
 */
void nrf_wifi_osal_llist_node_free(void *node);

/**
 * @brief Free a linked list node in control pool.
 * @param node Pointer to a linked list node.
 *
 * Frees a linked list node(node) which was allocated by nrf_wifi_osal_ctrl_llist_node_alloc.
 */
void nrf_wifi_osal_ctrl_llist_node_free(void *node);

/**
 * @brief Get data stored in a linked list node.
 * @param node Pointer to a linked list node.
 *
 * Get the data from a linked list node.
 *
 * @return Pointer to the data stored in the linked list node.
 *         NULL if there is an error.
 */
void *nrf_wifi_osal_llist_node_data_get(void *node);


/**
 * @brief Set data in a linked list node.
 * @param node Pointer to a linked list node.
 * @param data Pointer to the data to be stored in the linked list node.
 *
 * Stores the pointer to the data(data) in a linked list node(node).
 */
void nrf_wifi_osal_llist_node_data_set(void *node,
				       void *data);


/**
 * @brief Allocate a linked list.
 *
 * Allocate a linked list.
 *
 * @return Pointer to the allocated linked list on success, NULL on error.
 */
void *nrf_wifi_osal_llist_alloc(void);

/**
 * @brief Free a linked list.
 * @param llist Pointer to a linked list.
 *
 * Frees a linked list (llist) allocated by  nrf_wifi_osal_llist_alloc.
 */
void nrf_wifi_osal_llist_free(void *llist);

/**
 * @brief Allocate a linked list.
 *
 * Allocate a linked list.
 *
 * @return Pointer to the allocated linked list on success, NULL on error.
 */
void *nrf_wifi_osal_ctrl_llist_alloc(void);

/**
 * @brief Free a linked list.
 * @param llist Pointer to a linked list.
 *
 * Frees a linked list (llist) allocated by  nrf_wifi_osal_llist_alloc.
 */
void nrf_wifi_osal_ctrl_llist_free(void *llist);

/**
 * @brief Initialize a linked list.
 * @param llist Pointer to a linked list.
 *
 * Initialize a linked list (llist) allocated by  nrf_wifi_osal_llist_alloc.
 */
void nrf_wifi_osal_llist_init(void *llist);


/**
 * @brief Add a node to the tail of a linked list.
 * @param llist Pointer to a linked list.
 * @param llist_node Pointer to a linked list node.
 *
 * Adds a linked list node ( llist_node) allocated by  nrf_wifi_osal_llist_node_alloc
 * to the tail of a linked list (llist) allocated by  nrf_wifi_osal_llist_alloc.
 */
void nrf_wifi_osal_llist_add_node_tail(void *llist,
				       void *llist_node);


/**
 * @brief Add a node to the head of a linked list.
 * @param llist Pointer to a linked list.
 * @param llist_node Pointer to a linked list node.
 *
 * Adds a linked list node ( llist_node) allocated by  nrf_wifi_osal_llist_node_alloc
 * to the head of a linked list (llist) allocated by  nrf_wifi_osal_llist_alloc.
 */
void nrf_wifi_osal_llist_add_node_head(void *llist,
				       void *llist_node);
/**
 * @brief Get the head of a linked list.
 * @param llist Pointer to a linked list.
 *
 * Returns the head node from a linked list (llist) while still leaving the
 * node as part of the linked list. If the linked list is empty, returns NULL.
 *
 * @return Pointer to the head of the linked list on success, NULL on error.
 */
void *nrf_wifi_osal_llist_get_node_head(void *llist);


/**
 * @brief Get the next node in a linked list.
 * @param llist Pointer to a linked list.
 * @param llist_node Pointer to a linked list node.
 *
 * Return the node next to the node passed in the @p llist_node parameter.
 * The node returned is not removed from the linked list.
 *
 * @return Pointer to the next node in the linked list if successful, NULL otherwise.
 */
void *nrf_wifi_osal_llist_get_node_nxt(void *llist,
				       void *llist_node);


/**
 * @brief Delete node from a linked list.
 * @param llist Pointer to a linked list.
 * @param llist_node Pointer to a linked list node.
 *
 * Removes the node passed in the @p llist_node parameter from the linked list
 * passed in the @p llist parameter.
 */
void nrf_wifi_osal_llist_del_node(void *llist,
				  void *llist_node);


/**
 * @brief Get length of a linked list.
 * @param llist Pointer to a linked list.
 *
 * Returns the length of the linked list(@p llist).
 *
 * @return Linked list length in bytes.
 */
unsigned int nrf_wifi_osal_llist_len(void *llist);


/**
 * @brief Allocate a network buffer
 * @param size Size in bytes of the network buffer to be allocated.
 *
 * Allocate a network buffer.
 *
 * @return Pointer to the allocated network buffer if successful, NULL otherwise.
 */
void *nrf_wifi_osal_nbuf_alloc(unsigned int size);


/**
 * @brief Free a network buffer.
 * @param nbuf Pointer to a network buffer.
 *
 * Frees a network buffer(@p nbuf) which was allocated by
 * nrf_wifi_osal_nbuf_alloc().
 */
void nrf_wifi_osal_nbuf_free(void *nbuf);


/**
 * @brief Reserve headroom space in a network buffer.
 * @param nbuf Pointer to a network buffer.
 * @param size Size in bytes of the headroom to be reserved.
 *
 * Reserves headroom of size(@p size) bytes at the beginning of the data area of
 * a network buffer(@p nbuf).
 */
void nrf_wifi_osal_nbuf_headroom_res(void *nbuf,
				     unsigned int size);



/**
 * @brief Get the size of the headroom in a network buffer.
 * @param nbuf Pointer to a network buffer.
 *
 * Gets the size of the reserved headroom at the beginning
 * of the data area of a network buffer(@p nbuf).
 *
 * @return Size of the network buffer data headroom in bytes.
 */
unsigned int nrf_wifi_osal_nbuf_headroom_get(void *nbuf);

/**
 * @brief Get the size of data in a network buffer.
 * @param nbuf Pointer to a network buffer.
 *
 * Gets the size of the data area of a network buffer (@p nbuf).
 *
 * @return Size of the network buffer data in bytes.
 */
unsigned int nrf_wifi_osal_nbuf_data_size(void *nbuf);


/**
 * @brief Get a handle to the data in a network buffer.
 * @param nbuf Pointer to a network buffer.
 *
 * Gets the pointer to the data area of a network buffer(@p nbuf).
 *
 * @return Pointer to the data in the network buffer if successful, otherwise NULL.
 */
void *nrf_wifi_osal_nbuf_data_get(void *nbuf);


/**
 * @brief Extend the tail portion of the data in a network buffer.
 * @param nbuf Pointer to a network buffer.
 * @param size Size in bytes of the extension.
 *
 * Increases the data area of a network buffer(@p nbuf) by @p size bytes at the
 * end of the area and returns the pointer to the beginning of
 * the data area.
 *
 * @return Updated pointer to the data in the network buffer if successful, otherwise NULL.
 */
void *nrf_wifi_osal_nbuf_data_put(void *nbuf,
				  unsigned int size);


/**
 * @brief Extend the head portion of the data in a network buffer.
 * @param nbuf Pointer to a network buffer.
 * @param size Size in bytes, of the extension.
 *
 * Increases the data area of a network buffer(@p nbuf) by @p size bytes at the
 * start of the area and returns the pointer to the beginning of
 * the data area.
 *
 * @return Updated pointer to the data in the network buffer if successful, NULL otherwise.
 */
void *nrf_wifi_osal_nbuf_data_push(void *nbuf,
				   unsigned int size);


/**
 * @brief Reduce the head portion of the data in a network buffer.
 * @param nbuf Pointer to a network buffer.
 * @param size Size in bytes, of the reduction.
 *
 * Decreases the data area of a network buffer(@p nbuf) by @p size bytes at the
 * start of the area and returns the pointer to the beginning
 * of the data area.
 *
 * @return Updated pointer to the data in the network buffer if successful, NULL otherwise.
 */
void *nrf_wifi_osal_nbuf_data_pull(void *nbuf,
				   unsigned int size);


/**
 * @brief Get the priority of a network buffer.
 * @param nbuf Pointer to a network buffer.
 *
 * Gets the priority of a network buffer.
 *
 * @return Priority of the network buffer.
 */
unsigned char nrf_wifi_osal_nbuf_get_priority(void *nbuf);

/**
 * @brief Get the checksum status of a network buffer.
 * @param nbuf Pointer to a network buffer.
 *
 * Get the checksum status of a network buffer.
 *
 * @return Checksum status of the network buffer.
 */
unsigned char nrf_wifi_osal_nbuf_get_chksum_done(void *nbuf);

/**
 * @brief Set the checksum status of a network buffer.
 * @param nbuf Pointer to a network buffer.
 * @param chksum_done Checksum status of the network buffer.
 *
 * Set the checksum status of a network buffer.
 */
void nrf_wifi_osal_nbuf_set_chksum_done(void *nbuf,
					unsigned char chksum_done);

#if defined(CONFIG_NRF71_RAW_DATA_TX) || defined(__DOXYGEN__)
/**
 * @brief Set the raw Tx header in a network buffer.
 * @param nbuf Pointer to a network buffer.
 * @param raw_hdr_len Length of the raw Tx header to be set.
 *
 * Sets the raw Tx header in a network buffer.
 *
 * * @return Pointer to the raw Tx header if successful, NULL otherwise.
 */
void *nrf_wifi_osal_nbuf_set_raw_tx_hdr(void *nbuf,
					unsigned short raw_hdr_len);

/**
 * @brief Get the raw Tx header from a network buffer.
 * @param nbuf Pointer to a network buffer.
 *
 * Gets the raw Tx header from a network buffer.
 *
 * @return Pointer to the raw Tx header if successful, NULL otherwise.
 */
void *nrf_wifi_osal_nbuf_get_raw_tx_hdr(void *nbuf);

/**
 * @brief Check if a network buffer is a raw Tx buffer.
 * @param nbuf Pointer to a network buffer.
 *
 * Checks if a network buffer is a raw Tx buffer.
 *
 * @return true if the network buffer is a raw Tx buffer, false otherwise.
 */
bool nrf_wifi_osal_nbuf_is_raw_tx(void *nbuf);
#endif /* CONFIG_NRF71_RAW_DATA_TX || __DOXYGEN__ */

/**
 * @brief Allocate a tasklet.
 * @param type Type of tasklet.
 *
 * Allocates a tasklet structure and returns a pointer to it.
 *
 * @return Pointer to the tasklet instance allocated if successful, otherwise NULL.
 */
void *nrf_wifi_osal_tasklet_alloc(int type);

/**
 * @brief Free a tasklet.
 * @param tasklet Pointer to a tasklet.
 *
 * Frees a tasklet structure that had been allocated using
 * nrf_wifi_osal_tasklet_alloc().
 */
void nrf_wifi_osal_tasklet_free(void *tasklet);


/**
 * @brief - Initialize a tasklet.
 * @param tasklet Pointer to a tasklet.
 * @param callbk_fn Callback function to be invoked by the tasklet.
 * @param data Data to be passed to the callback function when the tasklet invokes it.
 *
 * Initializes a tasklet.
 */
void nrf_wifi_osal_tasklet_init(void *tasklet,
				void (*callbk_fn)(unsigned long),
				unsigned long data);


/**
 * @brief Schedule a tasklet.
 * @param tasklet Pointer to a tasklet.
 *
 * Schedules a tasklet that had been allocated using
 *  nrf_wifi_osal_tasklet_alloc and initialized using
 *  nrf_wifi_osal_tasklet_init.
 */
void nrf_wifi_osal_tasklet_schedule(void *tasklet);


/**
 * @brief Terminate a tasklet.
 * @param tasklet Pointer to a tasklet.
 *
 * Terminates a tasklet(tasklet) that had been scheduled by
 * nrf_wifi_osal_tasklet_schedule.
 */
void nrf_wifi_osal_tasklet_kill(void *tasklet);


/**
 * @brief Sleep for a specified duration in milliseconds.
 * @param msecs: Sleep duration in milliseconds.
 *
 * Puts the calling thread to sleep for at least msecs milliseconds.
 */
void nrf_wifi_osal_sleep_ms(unsigned int msecs);


/**
 * @brief Delay for a specified duration in microseconds.
 * @param usecs Delay duration in microseconds.
 *
 * Delays execution of calling thread for usecs microseconds. This is
 * busy-waiting and won't allow other threads to execute during
 * the time lapse.
 */
void nrf_wifi_osal_delay_us(unsigned long usecs);


/**
 * @brief Get current system uptime in microseconds.
 *
 * Get the current system uptime in microseconds.
 *
 * @return System uptime in microseconds.
 */
unsigned long nrf_wifi_osal_time_get_curr_us(void);

/**
 * @brief Get elapsed time in microseconds.
 * @param start_time_us The timestamp in microseconds from which elapsed time is to be measured.
 *
 * Get the elapsed system uptime in microseconds.
 *
 * @return Elapsed time in microseconds.
 */
unsigned int nrf_wifi_osal_time_elapsed_us(unsigned long start_time_us);

/**
 * nrf_wifi_osal_time_get_curr_ms() - Get current system uptime in milliseconds.
 *
 * Gets the current system uptime in milliseconds.
 *
 * Return: System uptime in milliseconds.
 */
unsigned long nrf_wifi_osal_time_get_curr_ms(void);

/**
 * nrf_wifi_osal_time_elapsed_ms() - Get elapsed time in milliseconds
 * @param start_time_ms: The timestamp in milliseconds from which elapsed
 *			   time is to be measured.
 *
 * Returns the time elapsed in milliseconds since some
 * time instant (@p start_time_ms).
 *
 * Return: Elapsed time in milliseconds.
 */
unsigned int nrf_wifi_osal_time_elapsed_ms(unsigned long start_time_ms);

/**
 * @brief Initialize a PCIe driver.
 * @param dev_name Name of the PCIe device.
 * @param vendor_id Vendor ID of the PCIe device.
 * @param sub_vendor_id Sub-vendor ID of the PCIE device.
 * @param device_id Device ID of the PCIe device.
 * @param sub_device_id Sub-device ID of the PCIe device.
 *
 * Initializes a PCIe device.
 *
 * @return OS specific PCIe device context.
 */
void *nrf_wifi_osal_bus_pcie_init(const char *dev_name,
				  unsigned int vendor_id,
				  unsigned int sub_vendor_id,
				  unsigned int device_id,
				  unsigned int sub_device_id);


/**
 * @brief Deinitialize a PCIe device driver.
 * @param os_pcie_priv OS specific PCIe context.
 *
 * This API should be called when the PCIe device driver is to be unregistered from
 * the OS's PCIe core.
 */
void nrf_wifi_osal_bus_pcie_deinit(void *os_pcie_priv);


/**
 * brief Add a PCIe device instance.
 * @param os_pcie_priv OS specific PCIe context.
 * @param osal_pcie_dev_ctx: Pointer to the OSAL PCIe device context.
 */
void *nrf_wifi_osal_bus_pcie_dev_add(void *os_pcie_priv,
				     void *osal_pcie_dev_ctx);


/**
 * @brief Remove a PCIe device instance.
 * @param os_pcie_dev_ctx Pointer to the OS specific PCIe device context which was
 *	returned by  nrf_wifi_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a matching PCIe device is removed from the system.
 */
void nrf_wifi_osal_bus_pcie_dev_rem(void *os_pcie_dev_ctx);


/**
 * @brief Initialize a PCIe device instance.
 * @param os_pcie_dev_ctx Pointer to the OS specific PCIe device context which was
 *                        returned by  nrf_wifi_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a PCIe device is to be initialized.
 *
 * @return NRF_WIFI_STATUS_SUCCESS if successful, NRF_WIFI_STATUS_FAIL otherwise.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_pcie_dev_init(void *os_pcie_dev_ctx);


/**
 * @brief Deinitialize a PCIe device instance.
 * @param os_pcie_dev_ctx Pointer to the OS specific PCIe device context which was
 *                        returned by  nrf_wifi_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a PCIe device is to be deinitialized.
 */
void nrf_wifi_osal_bus_pcie_dev_deinit(void *os_pcie_dev_ctx);


/**
 * @brief Register an interrupt handler for a PCIe device.
 * @param os_pcie_dev_ctx OS specific PCIe device context.
 * @param callbk_data Data to be passed to the ISR.
 * @param callbk_fn ISR to be invoked on receiving an interrupt.
 *
 * Registers an interrupt handler to the OS. This API also passes the callback
 * data to be passed to the ISR when an interrupt is received.
 *
 * @return NRF_WIFI_STATUS_SUCCESS if successful, NRF_WIFI_STATUS_FAIL otherwise.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_pcie_dev_intr_reg(void *os_pcie_dev_ctx,
							 void *callbk_data,
							 int (*callbk_fn)(void *callbk_data));


/**
 * @brief Unregister an interrupt handler for a PCIe device.
 * @param os_pcie_dev_ctx OS specific PCIe device context.
 *
 * Unregisters the interrupt handler that was registered using
 */
void nrf_wifi_osal_bus_pcie_dev_intr_unreg(void *os_pcie_dev_ctx);


/**
 * @brief Map host memory for DMA access.
 * @param os_pcie_dev_ctx Pointer to a OS specific PCIe device handle.
 * @param virt_addr Virtual host address to be DMA mapped.
 * @param size Size in bytes of the host memory to be DMA mapped.
 * @param dir DMA direction.
 *
 * Maps host memory of @p size bytes pointed to by the virtual address
 * @p virt_addr to be used by the device(@p dma_dev) for DMAing contents.
 * The contents are available for DMAing to the device if @p dir has a
 * value of NRF_WIFI_OSAL_DMA_DIR_TO_DEV. Conversely the device can DMA
 * contents to the host memory if @p dir has a value of
 * NRF_WIFI_OSAL_DMA_DIR_FROM_DEV. The function returns the DMA address
 * of the mapped memory.
 */
void *nrf_wifi_osal_bus_pcie_dev_dma_map(void *os_pcie_dev_ctx,
					 void *virt_addr,
					 size_t size,
					 enum nrf_wifi_osal_dma_dir dir);


/**
 * @brief Unmap DMA mapped host memory.
 * @param os_pcie_dev_ctx Pointer to a OS specific PCIe device handle.
 * @param dma_addr DMA mapped physical host memory address.
 * @param size Size in bytes of the DMA mapped host memory.
 * @param dir DMA direction.
 *
 * Unmaps the host memory which was mapped for DMA using nrf_wifi_osal_dma_map.
 */
void nrf_wifi_osal_bus_pcie_dev_dma_unmap(void *os_pcie_dev_ctx,
					  void *dma_addr,
					  size_t size,
					  enum nrf_wifi_osal_dma_dir dir);


/**
 * @brief Get host mapped address for a PCIe device.
 * @param os_pcie_dev_ctx OS specific PCIe device context.
 * @param host_map Host map address information.
 *
 * Get host mapped address for a PCIe device.
 */
void nrf_wifi_osal_bus_pcie_dev_host_map_get(void *os_pcie_dev_ctx,
					     struct nrf_wifi_osal_host_map *host_map);




/**
 * @brief Initialize a qspi driver.
 *
 * Registers a qspi device driver to the OS's qspi core.
 *
 * @return OS specific qspi device context.
 */
void *nrf_wifi_osal_bus_qspi_init(void);

/**
 * @brief Deinitialize a qspi device driver.
 * @param os_qspi_priv OS specific qspi context.
 *
 * This API should be called when the qspi device driver is
 * to be unregistered from the OS's qspi core.
 */
void nrf_wifi_osal_bus_qspi_deinit(void *os_qspi_priv);


/**
 * brief Add a qspi device instance.
 * @param os_qspi_priv OS specific qspi context.
 * @param osal_qspi_dev_ctx: Pointer to the OSAL qspi device context.
 *
 * Function to be invoked when a matching qspi device is added to the system.
 * It expects the pointer to an OS specific qspi device context to be returned.
 *
 * @return OS specific qspi device context.
 */
void *nrf_wifi_osal_bus_qspi_dev_add(void *os_qspi_priv,
				     void *osal_qspi_dev_ctx);


/**
 * brief Remove a qspi device instance.
 * @param os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                         returned by nrf_wifi_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a matching qspi device is removed from the system.
 */
void nrf_wifi_osal_bus_qspi_dev_rem(void *os_qspi_dev_ctx);


/**
 * @brief Initialize a qspi device instance.
 * @param os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                         returned by nrf_wifi_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a qspi device is to be initialized.
 *
 * @return
 *     - Pass: NRF_WIFI_STATUS_SUCCESS.
 *     - Fail: NRF_WIFI_STATUS_FAIL.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_qspi_dev_init(void *os_qspi_dev_ctx);


/**
 * brief Deinitialize a qspi device instance.
 * @param os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                         returned by nrf_wifi_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a qspi device is to be deinitialized.
 */
void nrf_wifi_osal_bus_qspi_dev_deinit(void *os_qspi_dev_ctx);


/**
 * brief Register a interrupt handler for a qspi device.
 * @param os_qspi_dev_ctx OS specific qspi device context.
 * @param callbk_data Data to be passed to the ISR.
 * @param callbk_fn ISR to be invoked on receiving an interrupt.
 *
 * Registers an interrupt handler to the OS. This API also passes the callback
 * data to be passed to the ISR when an interrupt is received.
 *
 * @return NRF_WIFI_STATUS_SUCCESS if successful, NRF_WIFI_STATUS_FAIL otherwise.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_qspi_dev_intr_reg(void *os_qspi_dev_ctx,
							 void *callbk_data,
							 int (*callbk_fn)(void *callbk_data));


/**
 * brief Unregister an interrupt handler for a qspi device.
 * @param os_qspi_dev_ctx OS specific qspi device context.
 *
 * Unregisters the interrupt handler that was registered using
 * nrf_wifi_osal_bus_qspi_dev_intr_reg.
 */
void nrf_wifi_osal_bus_qspi_dev_intr_unreg(void *os_qspi_dev_ctx);


/**
 * @brief Get host mapped address for a qspi device.
 * @param os_qspi_dev_ctx OS specific qspi device context.
 * @param host_map Host map address information.
 *
 * Gets the host map address for a qspi device.
 */
void nrf_wifi_osal_bus_qspi_dev_host_map_get(void *os_qspi_dev_ctx,
					     struct nrf_wifi_osal_host_map *host_map);

/**
 * @brief Read value from a 32 bit register on a QSPI slave device.
 * @param priv
 * @param addr Address of the register to read from.
 *
 * @return 32 bit value read from register.
 */
unsigned int nrf_wifi_osal_qspi_read_reg32(void *priv,
					   unsigned long addr);

/**
 * @brief Writes a 32 bit value to a 32 bit device register on a QSPI slave device.
 * @param priv
 * @param addr Address of the register to write to.
 * @param val Value to be written to the register.
 */
void nrf_wifi_osal_qspi_write_reg32(void *priv,
				    unsigned long addr,
				    unsigned int val);

/**
 * @brief Copies data from a QSPI slave device to a destination buffer.
 * @param priv
 * @param dest Destination buffer.
 * @param addr Address of the data to be copied.
 * @param count Number of bytes to be copied.
 */
void nrf_wifi_osal_qspi_cpy_from(void *priv,
				 void *dest,
				 unsigned long addr,
				 size_t count);

/**
 * @brief Copies data from a source buffer to a QSPI slave device.
 * @param priv
 * @param addr Address of the data to be written.
 * @param src Source buffer.
 * @param count Number of bytes to be copied.
 */
void nrf_wifi_osal_qspi_cpy_to(void *priv,
			       unsigned long addr,
			       const void *src,
			       size_t count);

/**
 * @brief Initialize a spi driver.
 *
 * Registers a spi device driver to the OS's spi core.
 *
 * @return OS specific spi device context.
 */
void *nrf_wifi_osal_bus_spi_init(void);

/**
 * @brief Deinitialize a spi device driver.
 * @param os_spi_priv OS specific spi context.
 *
 * This API should be called when the spi device driver is
 * to be unregistered from the OS's spi core.
 */
void nrf_wifi_osal_bus_spi_deinit(void *os_spi_priv);


/**
 * brief Add a spi device instance.
 * @param os_spi_priv OS specific spi context.
 * @param osal_spi_dev_ctx Pointer to the OSAL spi device context.
 *
 * Function to be invoked when a matching spi device is added to the system.
 * It expects the pointer to a OS specific spi device context to be returned.
 *
 * @return OS specific spi device context.
 */
void *nrf_wifi_osal_bus_spi_dev_add(void *os_spi_priv,
				    void *osal_spi_dev_ctx);


/**
 * @brief Remove a spi device instance.
 * @param os_spi_dev_ctx Pointer to the OS specific spi device context which was
 *                       returned by nrf_wifi_osal_bus_spi_dev_add.
 *
 * Function to be invoked when a matching spi device is removed from the system.
 */
void nrf_wifi_osal_bus_spi_dev_rem(void *os_spi_dev_ctx);


/**
 * @brief Initialize a spi device instance.
 * @param os_spi_dev_ctx Pointer to the OS specific spi device context which was
 *                       returned by nrf_wifi_osal_bus_spi_dev_add.
 *
 * Function to be invoked when a spi device is to be initialized.
 *
 * @return
 *      - Pass: nrf_wifi_STATUS_SUCCESS.
 *      - Fail: nrf_wifi_STATUS_FAIL.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_spi_dev_init(void *os_spi_dev_ctx);


/**
 * @brief Deinitialize a spi device instance.
 * @param os_spi_dev_ctx Pointer to the OS specific spi device context which was
 *                       returned by nrf_wifi_osal_bus_spi_dev_add.
 *
 * Function to be invoked when a spi device is to be deinitialized.
 */
void nrf_wifi_osal_bus_spi_dev_deinit(void *os_spi_dev_ctx);


/**
 * @brief Register a interrupt handler for a spi device.
 * @param os_spi_dev_ctx OS specific spi device context.
 * @param callbk_data Data to be passed to the ISR.
 * @param callbk_fn ISR to be invoked on receiving an interrupt.
 *
 * Registers an interrupt handler to the OS. This API also passes the callback
 * data to be passed to the ISR when an interrupt is received.
 *
 * @return
 *     Pass: nrf_wifi_STATUS_SUCCESS.
 *     Fail: nrf_wifi_STATUS_FAIL.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_spi_dev_intr_reg(void *os_spi_dev_ctx,
							void *callbk_data,
							int (*callbk_fn)(void *callbk_data));


/**
 * @brief Unregister an interrupt handler for a spi device.
 * @param os_spi_dev_ctx OS specific spi device context.
 *
 * Unregisters the interrupt handler that was registered using
 *  nrf_wifi_osal_bus_spi_dev_intr_reg.
 */
void nrf_wifi_osal_bus_spi_dev_intr_unreg(void *os_spi_dev_ctx);


/**
 * @brief Get host mapped address for a spi device.
 * @param os_spi_dev_ctx OS specific spi device context.
 * @param host_map Host map address information.
 *
 * Get the host map address for a spi device.
 */
void nrf_wifi_osal_bus_spi_dev_host_map_get(void *os_spi_dev_ctx,
					    struct nrf_wifi_osal_host_map *host_map);

/**
 * @brief Read value from a 32 bit register on a SPI slave device.
 * @param priv
 * @param addr Address of the register to read from.
 *
 * @return 32 bit value read from register.
 */
unsigned int nrf_wifi_osal_spi_read_reg32(void *priv,
					  unsigned long addr);

/**
 * @brief Writes a 32 bit value to a 32 bit device register on a SPI slave device.
 * @param priv
 * @param addr Address of the register to write to.
 * @param val Value to be written to the register.
 */
void nrf_wifi_osal_spi_write_reg32(void *priv,
				   unsigned long addr,
				   unsigned int val);

/**
 * @brief Copies data from a SPI slave device to a destination buffer.
 * @param priv
 * @param dest Destination buffer.
 * @param addr Address of the register to read from.
 * @param count Number of bytes to copy.
 */
void nrf_wifi_osal_spi_cpy_from(void *priv,
				void *dest,
				unsigned long addr,
				size_t count);

/**
 * @brief Copies data from a source buffer to a SPI slave device.
 * @param priv
 * @param addr Address of the register to write to.
 * @param src Source buffer.
 * @param count Number of bytes to copy.
 */
void nrf_wifi_osal_spi_cpy_to(void *priv,
			      unsigned long addr,
			      const void *src,
			      size_t count);


#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
/**
 * @brief Allocate a timer.
 *
 * Allocates a timer instance and returns a pointer to it.
 *
 * @return Pointer to the allocated timer instance.
 */
void *nrf_wifi_osal_timer_alloc(void);

/**
 * @brief Free a timer.
 * @param timer Pointer to a timer instance.
 *
 * Frees/Deallocates a timer that has been allocated using nrf_wifi_osal_timer_alloc
 */
void nrf_wifi_osal_timer_free(void *timer);


/**
 * @brief Initialize a timer.
 * @param timer Pointer to a timer instance.
 * @param callbk_fn Callback function to be invoked when the timer expires.
 * @param data Data to be passed to the callback function.
 *
 * Initializes a timer that has been allocated using nrf_wifi_osal_timer_alloc
 * Need to pass (@p callbk_fn) callback function with the data(@p data) to be
 * passed to the callback function, whenever the timer expires.
 */
void nrf_wifi_osal_timer_init(void *timer,
			      void (*callbk_fn)(unsigned long),
			      unsigned long data);


/**
 * @brief Schedule a timer.
 * @param timer Pointer to a timer instance.
 * @param duration Duration of the timer in seconds.
 *
 * Schedules a timer with a @p duration seconds that has been allocated using
 * nrf_wifi_osal_timer_alloc and initialized with nrf_wifi_osal_timer_init.
 */
void nrf_wifi_osal_timer_schedule(void *timer,
				  unsigned long duration);


/**
 * @brief Kills a timer.
 * @param timer Pointer to a timer instance.
 */
void nrf_wifi_osal_timer_kill(void *timer);

/**
 * @brief Puts the QSPI interface to sleep.
 * @param os_qspi_priv Pointer to the QSPI private data.
 *
 * @return 0 on success, negative error code on failure.
 */
int nrf_wifi_osal_bus_qspi_ps_sleep(void *os_qspi_priv);

/**
 * @brief Wakes up the QSPI interface from sleep.
 * @param os_qspi_priv Pointer to the QSPI private data.
 *
 * @return 0 on success, negative error code on failure.
 */
int nrf_wifi_osal_bus_qspi_ps_wake(void *os_qspi_priv);

/**
 * @brief Get the power status of a QSPI interface.
 * @param os_qspi_priv Pointer to the QSPI private data.
 *
 * @return 0 if the QSPI interface is in sleep mode,
 *         1 if it is awake,
 *         Negative error code on failure.
 */
int nrf_wifi_osal_bus_qspi_ps_status(void *os_qspi_priv);
#endif /* NRF_WIFI_LOW_POWER */

/**
 * @brief nrf_wifi_osal_assert() - Assert a condition with a value.
 * @param test Variable to be tested.
 * @param val Value to be checked for the @p test
 * @param op Type of operation to be done during assertion check.
 * @param msg Assertion message.
 *
 * Compares @p test with @p val. If true, prints assert message.
 */
void nrf_wifi_osal_assert(int test,
			  int val,
			  enum nrf_wifi_assert_op_type op,
			  char *msg);

/**
 * @brief Gives the length of the string @p str.
 * @param str: Pointer to the memory location of the string.
 *
 * Calculates the length of the string pointed to by str.
 *
 * @return The number of bytes of the string str.
 */
unsigned int nrf_wifi_osal_strlen(const void *str);

/**
 * @brief Compare contents from one memory location to another.
 * @param addr1 Pointer to the memory location of first address.
 * @param addr2 Pointer to the memory location of second address.
 * @param count Number of bytes to be compared.
 *
 * Compares count number of bytes from addr1 location in memory to addr2
 * location in memory.
 *
 * @return An integer less than, equal to, or greater than zero.
 */
int nrf_wifi_osal_mem_cmp(const void *addr1,
			  const void *addr2,
			  size_t count);

/**
 * nrf_wifi_osal_rand8_get() - Get a random 8 bit number.
 *
 * Generates an 8 bit random number.
 *
 * Return: an 8 bit random number.
 */
unsigned char nrf_wifi_osal_rand8_get(void);

int nrf_wifi_osal_ipc_send_msg(unsigned int msg_type,
	void *msg,
	unsigned int msg_len);

#endif /* __OSAL_API_H__ */

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_API_H__
#define __OSAL_API_H__

#include <stdarg.h>
#include "osal_structs.h"

/**
 * wifi_nrf_osal_init() - Initialize the OSAL layer.
 *
 * Initializes the OSAL layer and is expected to be called
 * before using the OSAL layer. Returns a pointer to the OSAL context
 * which might need to be passed to further API calls.
 *
 * Return: Pointer to instance of OSAL context.
 */
struct wifi_nrf_osal_priv *wifi_nrf_osal_init(void);


/**
 * wifi_nrf_osal_deinit() - Deinitialize the OSAL layer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 *
 * Deinitializes the OSAL layer and is expected to be called after done using
 * the OSAL layer.
 *
 * Return: None.
 */
void wifi_nrf_osal_deinit(struct wifi_nrf_osal_priv *opriv);


/**
 * wifi_nrf_osal_mem_alloc() - Allocate memory.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @size: Size of the memory to be allocated in bytes.
 *
 * Allocates memory of @size bytes and returns a pointer to the start
 * of the memory allocated.
 *
 * Return:
 *		Pass: Pointer to start of allocated memory.
 *		Error: NULL.
 */
void *wifi_nrf_osal_mem_alloc(struct wifi_nrf_osal_priv *opriv,
			       size_t size);

/**
 * wifi_nrf_osal_mem_zalloc() - Allocated zero-initialized memory.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @size: Size of the memory to be allocated in bytes.
 *
 * Allocates memory of @size bytes, zeroes it out and returns a pointer to the
 * start of the memory allocated.
 *
 * Return:
 *		Pass: Pointer to start of allocated memory.
 *		Error: NULL.
 */
void *wifi_nrf_osal_mem_zalloc(struct wifi_nrf_osal_priv *opriv,
				size_t size);

/**
 * wifi_nrf_osal_mem_free() - Free previously allocated memory.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @buf: Pointer to the memory to be freed.
 *
 * Free up memory which has been allocated using @wifi_nrf_osal_mem_alloc or
 * @wifi_nrf_osal_mem_zalloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_mem_free(struct wifi_nrf_osal_priv *opriv,
			     void *buf);

/**
 * wifi_nrf_osal_mem_cpy() - Copy contents from one memory location to another.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @dest: Pointer to the memory location where contents are to be copied.
 * @src: Pointer to the memory location from where contents are to be copied.
 * @count: Number of bytes to be copied.
 *
 * Copies @count number of bytes from @src location in memory to @dest
 * location in memory.
 *
 * Return:
 *		Pass: Pointer to destination memory.
 *		Error: NULL.
 */
void *wifi_nrf_osal_mem_cpy(struct wifi_nrf_osal_priv *opriv,
			     void *dest,
			     const void *src,
			     size_t count);

/**
 * wifi_nrf_osal_mem_set() - Fill a block of memory with a particular value.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @start: Pointer to the memory location whose contents are to be set.
 * @val: Value to be set.
 * @size: Number of bytes to be set.
 *
 * Fills a block of memory of @size bytes starting at @start with a particular
 * value represented by @val.
 *
 * Return:
 *		Pass: Pointer to memory location which was set.
 *		Error: NULL.
 */
void *wifi_nrf_osal_mem_set(struct wifi_nrf_osal_priv *opriv,
			     void *start,
			     int val,
			     size_t size);


/**
 * wifi_nrf_osal_iomem_mmap() - Memory map IO memory into CPU space.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @addr: Address of the IO memory to be mapped.
 * @size: Size of the IO memory in bytes.
 *
 * Maps IO memory of @size bytes pointed to by @addr into CPU space.
 *
 * Return:
 *		Pass: Pointer to the mapped IO memory.
 *		Error: NULL.
 */
void *wifi_nrf_osal_iomem_mmap(struct wifi_nrf_osal_priv *opriv,
				unsigned long addr,
				unsigned long size);

/**
 * wifi_nrf_osal_iomem_unmap() - Unmap previously mapped IO memory from CPU space.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @addr: Pointer to mapped IO memory to be unmapped.
 *
 * Unmaps IO memory from CPU space that was mapped using @wifi_nrf_osal_iomem_mmap.
 *
 * Return: None.
 */
void wifi_nrf_osal_iomem_unmap(struct wifi_nrf_osal_priv *opriv,
				volatile void *addr);

/**
 * wifi_nrf_osal_iomem_read_reg32() - Read value from a 32 bit IO memory mapped
 *                                   register.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @addr: Pointer to the IO memory mapped register address.
 *
 * Reads value from a 32 bit device register using a memory mapped
 * address(@addr).
 *
 * Return: 32 bit value read from register.
 */
unsigned int wifi_nrf_osal_iomem_read_reg32(struct wifi_nrf_osal_priv *opriv,
					     const volatile void *addr);

/**
 * wifi_nrf_osal_iomem_write_reg32() - Write a 32 bit value to a IO memory mapped register.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @addr: Pointer to the IO memory mapped register address.
 * @val: Value to be written to the register.
 *
 * Writes a 32 bit value (@val) to a 32 bit device register using a memory
 * mapped address(@addr).
 *
 * Return: None.
 */
void wifi_nrf_osal_iomem_write_reg32(struct wifi_nrf_osal_priv *opriv,
				      volatile void *addr,
				      unsigned int val);

/**
 * wifi_nrf_osal_iomem_cpy_from() - Copy data from the memory of a memory
 *                                 mapped IO device to host memory.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @dest: Pointer to the host memory where data is to be copied.
 * @src: Pointer to the memory of the memory mapped IO device from where
 *       data is to be copied.
 * @count: The size of the data to be copied in bytes.
 *
 * Copies a block of data of size @count bytes from memory mapped device
 * memory(@src) to host memory(@dest).
 *
 * Return: None.
 */
void wifi_nrf_osal_iomem_cpy_from(struct wifi_nrf_osal_priv *opriv,
				   void *dest,
				   const volatile void *src,
				   size_t count);

/**
 * wifi_nrf_osal_iomem_cpy_to() - Copy data to the memory of a memory
 *                               mapped IO device from host memory.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @dest: Pointer to the memory of the memory mapped IO device where
 *        data is to be copied.
 * @src: Pointer to the host memory from where data is to be copied.
 * @count: The size of the data to be copied in bytes.
 *
 * Copies a block of data of size @count bytes from host memory (@src) to
 * memory mapped device memory(@dest).
 *
 * Return: None.
 */
void wifi_nrf_osal_iomem_cpy_to(struct wifi_nrf_osal_priv *opriv,
				 volatile void *dest,
				 const void *src,
				 size_t count);


/**
 * wifi_nrf_osal_spinlock_alloc() - Allocate a busy lock.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 *
 * Allocates a busy lock.
 *
 * Return:
 *		Pass: Pointer to the busy lock instance.
 *		Error: NULL.
 */
void *wifi_nrf_osal_spinlock_alloc(struct wifi_nrf_osal_priv *opriv);

/**
 * wifi_nrf_osal_spinlock_free() - Free a busy lock.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @lock: Pointer to a busy lock instance.
 *
 * Frees a busy lock (@lock) allocated by @wifi_nrf_osal_spinlock_alloc
 *
 * Return: None.
 */
void wifi_nrf_osal_spinlock_free(struct wifi_nrf_osal_priv *opriv,
				  void *lock);


/**
 * wifi_nrf_osal_spinlock_init() - Initialize a busy lock.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @lock: Pointer to a busy lock instance.
 *
 * Initializes a busy lock (@lock) allocated by @wifi_nrf_osal_spinlock_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_spinlock_init(struct wifi_nrf_osal_priv *opriv,
				  void *lock);


/**
 * wifi_nrf_osal_spinlock_take() - Acquire a buys lock.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @lock: Pointer to a busy lock instance.
 *
 * Acquires a busy lock (@lock) allocated by @wifi_nrf_osal_spinlock_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_spinlock_take(struct wifi_nrf_osal_priv *opriv,
				  void *lock);


/**
 * wifi_nrf_osal_spinlock_rel() - Releases a busy lock.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @lock: Pointer to a busy lock instance.
 *
 * Releases a busy lock (@lock) acquired by @wifi_nrf_osal_spinlock_take.
 *
 * Return: None.
 */
void wifi_nrf_osal_spinlock_rel(struct wifi_nrf_osal_priv *opriv,
				 void *lock);


/**
 * wifi_nrf_osal_spinlock_irq_take() - Acquire a busy lock and disable
 *                                    interrupts.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @lock: Pointer to a busy lock instance.
 * @flags: Interrupt state flags.
 *
 * Saves interrupt states (@flags), disable interrupts and take a
 * busy lock (@lock).
 *
 * Return: None.
 */
void wifi_nrf_osal_spinlock_irq_take(struct wifi_nrf_osal_priv *opriv,
				      void *lock,
				      unsigned long *flags);


/**
 * wifi_nrf_osal_spinlock_irq_rel() - Release a busy lock and enable interrupts.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @lock: Pointer to a busy lock instance.
 * @flags: Interrupt state flags.
 *
 * Restores interrupt states (@flags) and releases busy lock (@lock) acquired
 * using @wifi_nrf_osal_spinlock_irq_take.
 *
 * Return: None.
 */
void wifi_nrf_osal_spinlock_irq_rel(struct wifi_nrf_osal_priv *opriv,
				     void *lock,
				     unsigned long *flags);


/**
 * wifi_nrf_osal_log_dbg() - Log a debug message.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @fmt: Format string.
 * @...: Variable arguments.
 *
 * Logs a debug message.
 *
 * Return: Number of characters of the message logged.
 */
int wifi_nrf_osal_log_dbg(struct wifi_nrf_osal_priv *opriv,
			   const char *fmt, ...);


/**
 * wifi_nrf_osal_log_info() - Log a informational message.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @fmt: Format string.
 * @...: Variable arguments.
 *
 * Logs an informational message.
 *
 * Return: Number of characters of the message logged.
 */
int wifi_nrf_osal_log_info(struct wifi_nrf_osal_priv *opriv,
			    const char *fmt, ...);


/**
 * wifi_nrf_osal_log_err() - Logs an error message.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @fmt: Format string.
 * @...: Variable arguments.
 *
 * Logs an error message.
 *
 * Return: Number of characters of the message logged.
 */
int wifi_nrf_osal_log_err(struct wifi_nrf_osal_priv *opriv,
			   const char *fmt, ...);


/**
 * wifi_nrf_osal_llist_node_alloc() - Allocate a linked list mode.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 *
 * Allocates a linked list node.
 *
 * Return:
 *		Pass: Pointer to the linked list node allocated.
 *		Error: NULL.
 */
void *wifi_nrf_osal_llist_node_alloc(struct wifi_nrf_osal_priv *opriv);


/**
 * wifi_nrf_osal_llist_node_free() - Free a linked list node.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @node: Pointer to a linked list node.
 *
 * Frees a linked list node(@node) which was allocated by
 * @wifi_nrf_osal_llist_node_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_llist_node_free(struct wifi_nrf_osal_priv *opriv,
				    void *node);

/**
 * wifi_nrf_osal_llist_node_data_get() - Get data stored in a linked list node.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @node: Pointer to a linked list node.
 *
 * Gets the pointer to the data which the linked list node(@node) points to.
 *
 * Return:
 *		Pass: Pointer to the data stored in the linked list node.
 *		Error: NULL.
 */
void *wifi_nrf_osal_llist_node_data_get(struct wifi_nrf_osal_priv *opriv,
					 void *node);


/**
 * wifi_nrf_osal_llist_node_data_set() - Set data in a linked list node.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @node: Pointer to a linked list node.
 * @data: Pointer to the data to be stored in the linked list node.
 *
 * Stores the pointer to the data(@data) in a linked list node(@node).
 *
 *
 * Return: None.
 */
void wifi_nrf_osal_llist_node_data_set(struct wifi_nrf_osal_priv *opriv,
					void *node,
					void *data);


/**
 * wifi_nrf_osal_llist_alloc() - Allocate a linked list.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 *
 * Allocates a linked list.
 *
 * Return:
 *		Pass: Pointer to the allocated linked list.
 *		Error: NULL.
 */
void *wifi_nrf_osal_llist_alloc(struct wifi_nrf_osal_priv *opriv);


/**
 * wifi_nrf_osal_llist_free() - Free a linked list.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @llist: Pointer to a linked list.
 *
 * Frees a linked list(@llist) allocated by @wifi_nrf_osal_llist_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_llist_free(struct wifi_nrf_osal_priv *opriv,
			       void *llist);


/**
 * wifi_nrf_osal_llist_init() - Initialize a linked list.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @llist: Pointer to a linked list.
 *
 * Initializes a linked list(@llist) allocated by @wifi_nrf_osal_llist_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_llist_init(struct wifi_nrf_osal_priv *opriv,
			       void *llist);


/**
 * wifi_nrf_osal_llist_add_node_tail() - Add a node to the tail of a linked list.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @llist: Pointer to a linked list.
 * @llist_node: Pointer to a linked list node.
 *
 * Adds a linked list node(@llist_node) allocated by @wifi_nrf_osal_llist_node_alloc
 * to the tail of a linked list(@llist) allocated by @wifi_nrf_osal_llist_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_llist_add_node_tail(struct wifi_nrf_osal_priv *opriv,
					void *llist,
					void *llist_node);


/**
 * wifi_nrf_osal_llist_get_node_head() - Get the head of a linked list.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @llist: Pointer to a linked list.
 *
 * Returns the head node from a linked list(@llist) while still leaving the
 * node as part of the linked list. If the linked list is empty
 * returns NULL.
 *
 * Return:
 *		Pass: Pointer to the head of the linked list.
 *		Error: NULL.
 */
void *wifi_nrf_osal_llist_get_node_head(struct wifi_nrf_osal_priv *opriv,
					 void *llist);


/**
 * wifi_nrf_osal_llist_get_node_nxt() - Get the next node in a linked list.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @llist: Pointer to a linked list.
 * @llist_node: Pointer to a linked list node.
 *
 * Return the node next to the node passed in the @llist_node parameter.
 * The node returned is not removed from the linked list.
 *
 * Return:
 *		Pass: Pointer to the next node in the linked list.
 *		Error: NULL.
 */
void *wifi_nrf_osal_llist_get_node_nxt(struct wifi_nrf_osal_priv *opriv,
					void *llist,
					void *llist_node);


/**
 * wifi_nrf_osal_llist_del_node() - Delete node from a linked list.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @llist: Pointer to a linked list.
 * @llist_node: Pointer to a linked list node.
 *
 * Removes the node passed in the @llist_node parameter from the linked list
 * passed in the @llist parameter.
 *
 * Return: None.
 */
void wifi_nrf_osal_llist_del_node(struct wifi_nrf_osal_priv *opriv,
				   void *llist,
				   void *llist_node);


/**
 * wifi_nrf_osal_llist_len() - Get length of a linked list.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @llist: Pointer to a linked list.
 *
 * Returns the length of the linked list(@llist).
 *
 * Return: Linked list length in bytes.
 */
unsigned int wifi_nrf_osal_llist_len(struct wifi_nrf_osal_priv *opriv,
				      void *llist);


/**
 * wifi_nrf_osal_nbuf_alloc() - Allocate a network buffer
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @size: Size in bytes of the network buffer to allocated.
 *
 * Allocates a network buffer of size @size.
 *
 * Return:
 *		Pass: Pointer to the allocated network buffer.
 *		Error: NULL.
 */
void *wifi_nrf_osal_nbuf_alloc(struct wifi_nrf_osal_priv *opriv,
				unsigned int size);


/**
 * wifi_nrf_osal_nbuf_free() - Free a network buffer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @nbuf: Pointer to a network buffer.
 *
 * Frees a network buffer(@nbuf) which was allocated by
 * @wifi_nrf_osal_nbuf_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_nbuf_free(struct wifi_nrf_osal_priv *opriv,
			      void *nbuf);


/**
 * wifi_nrf_osal_nbuf_headroom_res() - Reserve headroom space in a network buffer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @nbuf: Pointer to a network buffer.
 * @size: Size in bytes of the headroom to be reserved.
 *
 * Reserves headroom of size(@size) bytes at the beginning of the data area of
 * a network buffer(@nbuf).
 *
 * Return: None.
 */
void wifi_nrf_osal_nbuf_headroom_res(struct wifi_nrf_osal_priv *opriv,
				      void *nbuf,
				      unsigned int size);



/**
 * wifi_nrf_osal_nbuf_headroom_get() - Get the size of the headroom in a network buffer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @nbuf: Pointer to a network buffer.
 *
 * Gets the size of the reserved headroom at the beginning
 * of the data area of a network buffer(@nbuf).
 *
 * Return: Size of the network buffer data headroom in bytes.
 */
unsigned int wifi_nrf_osal_nbuf_headroom_get(struct wifi_nrf_osal_priv *opriv,
					      void *nbuf);

/**
 * wifi_nrf_osal_nbuf_data_size() - Get the size of data in a network buffer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @nbuf: Pointer to a network buffer.
 *
 * Gets the size of the data area of a network buffer(@nbuf).
 *
 * Return: Size of the network buffer data in bytes.
 */
unsigned int wifi_nrf_osal_nbuf_data_size(struct wifi_nrf_osal_priv *opriv,
					   void *nbuf);


/**
 * wifi_nrf_osal_nbuf_data_get() - Get a handle to the data in a network buffer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @nbuf: Pointer to a network buffer.
 *
 * Gets the pointer to the data area of a network buffer(@nbuf).
 *
 * Return:
 *		Pass: Pointer to the data in the network buffer.
 *		Error: NULL.
 */
void *wifi_nrf_osal_nbuf_data_get(struct wifi_nrf_osal_priv *opriv,
				   void *nbuf);


/**
 * wifi_nrf_osal_nbuf_data_put() - Extend the tail portion of the data
 *                                in a network buffer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @nbuf: Pointer to a network buffer.
 * @size: Size in bytes, of the extension.
 *
 * Increases the data area of a network buffer(@nbuf) by @size bytes at the
 * end of the area and returns the pointer to the beginning of
 * the data area.
 *
 * Return:
 *		Pass: Updated pointer to the data in the network buffer.
 *		Error: NULL.
 */
void *wifi_nrf_osal_nbuf_data_put(struct wifi_nrf_osal_priv *opriv,
				   void *nbuf,
				   unsigned int size);


/**
 * wifi_nrf_osal_nbuf_data_push() - Extend the head portion of the data
 *                                 in a network buffer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @nbuf: Pointer to a network buffer.
 * @size: Size in bytes, of the extension.
 *
 * Increases the data area of a network buffer(@nbuf) by @size bytes at the
 * start of the area and returns the pointer to the beginning of the
 * data area.
 *
 * Return:
 *		Pass: Updated pointer to the data in the network buffer.
 *		Error: NULL.
 */
void *wifi_nrf_osal_nbuf_data_push(struct wifi_nrf_osal_priv *opriv,
				    void *nbuf,
				    unsigned int size);


/**
 * wifi_nrf_osal_nbuf_data_pull() - Reduce the head portion of the data
 *                                 in a network buffer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @nbuf: Pointer to a network buffer.
 * @size: Size in bytes, of the reduction.
 *
 * Decreases the data area of a network buffer(@nbuf) by @size bytes at the
 * start of the area and returns the pointer to the beginning
 * of the data area.
 *
 * Return:
 *		Pass: Updated pointer to the data in the network buffer.
 *		Error: NULL.
 */
void *wifi_nrf_osal_nbuf_data_pull(struct wifi_nrf_osal_priv *opriv,
				    void *nbuf,
				    unsigned int size);


/**
 * wifi_nrf_osal_tasklet_alloc() - Allocate a tasklet.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @type: Type of tasklet.
 *
 * Allocates a tasklet structure and returns a pointer to it.
 *
 * Return:
 *		Pass: Pointer to the tasklet instance allocated.
 *		Error: NULL.
 */
void *wifi_nrf_osal_tasklet_alloc(struct wifi_nrf_osal_priv *opriv, int type);


/**
 * wifi_nrf_osal_tasklet_free() - Free a tasklet.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @tasklet: Pointer to a tasklet.
 *
 * Frees a tasklet structure that had been allocated using
 * @wifi_nrf_osal_tasklet_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_tasklet_free(struct wifi_nrf_osal_priv *opriv,
				 void *tasklet);


/**
 * wifi_nrf_osal_tasklet_init() - Initialize a tasklet.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @tasklet:  Pointer to a tasklet.
 * @callbk_fn: Callback function to be invoked by the tasklet.
 * @data: Data to be passed to the callback function when the tasklet
 *        invokes it.
 *
 * Initializes a tasklet structure(@tasklet) that had been allocated using
 * @wifi_nrf_osal_tasklet_alloc. Needs to be passed a callback
 * function (@callbk_fn) which should get invoked when the tasklet is
 * scheduled and the data(@data) which needs to be passed to the
 * callback function.
 *
 * Return: None.
 */
void wifi_nrf_osal_tasklet_init(struct wifi_nrf_osal_priv *opriv,
				 void *tasklet,
				 void (*callbk_fn)(unsigned long),
				 unsigned long data);


/**
 * wifi_nrf_osal_tasklet_schedule() - Schedule a tasklet.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @tasklet:  Pointer to a tasklet.
 *
 * Schedules a tasklet that had been allocated using
 * @wifi_nrf_osal_tasklet_alloc and initialized using
 * @wifi_nrf_osal_tasklet_init.
 *
 * Return: None.
 */
void wifi_nrf_osal_tasklet_schedule(struct wifi_nrf_osal_priv *opriv,
				     void *tasklet);


/**
 * wifi_nrf_osal_tasklet_kill() - Terminate a tasklet.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @tasklet:  Pointer to a tasklet.
 *
 * Terminates a tasklet(@tasklet) that had been scheduled by
 * @wifi_nrf_osal_tasklet_schedule.
 *
 * Return: None.
 */
void wifi_nrf_osal_tasklet_kill(struct wifi_nrf_osal_priv *opriv,
				 void *tasklet);


/**
 * wifi_nrf_osal_sleep_ms() - Sleep for a specified duration in milliseconds.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @msecs: Sleep duration in milliseconds.
 *
 * Puts the calling thread to sleep for atleast @msecs milliseconds.
 *
 * Return: None.
 */
void wifi_nrf_osal_sleep_ms(struct wifi_nrf_osal_priv *opriv,
			     unsigned int msecs);


/**
 * wifi_nrf_osal_delay_us() - Delay for a specified duration in microseconds.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @usecs: Delay duration in microseconds.
 *
 * Delays execution of calling thread for @usecs microseconds. This is
 * busy-waiting and won't allow other threads to execute during
 * the time lapse.
 *
 * Return: None.
 */
void wifi_nrf_osal_delay_us(struct wifi_nrf_osal_priv *opriv,
			     unsigned long usecs);


/**
 * wifi_nrf_osal_time_get_curr_us() - Get current system uptime in microseconds.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 *
 * Gets the current system uptime in microseconds.
 *
 * Return: System uptime in microseconds.
 */
unsigned long wifi_nrf_osal_time_get_curr_us(struct wifi_nrf_osal_priv *opriv);


/**
 * wifi_nrf_osal_time_elapsed_us() - Get elapsed time in microseconds
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @start_time_us: The timestamp in microseconds from which elapsed
 *                 time is to be measured.
 *
 * Returns the time elapsed in microseconds since some
 * time instant (@start_time_us).
 *
 * Return: Elapsed time in microseconds.
 */
unsigned int wifi_nrf_osal_time_elapsed_us(struct wifi_nrf_osal_priv *opriv,
					    unsigned long start_time_us);



/**
 * wifi_nrf_osal_bus_pcie_init() - Initialize a PCIe driver.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @dev_name: Name of the PCIe device.
 * @vendor_id: Vendor ID of the PCIe device.
 * @sub_vendor_id: Sub-vendor ID of the PCIE device.
 * @device_id: Device ID of the PCIe device.
 * @sub_device_id: Sub-device ID of the PCIe device.
 *
 * Registers a PCIe device driver to the OS's PCIe core.
 *
 * Return: OS specific PCIe device context.
 */
void *wifi_nrf_osal_bus_pcie_init(struct wifi_nrf_osal_priv *opriv,
				   const char *dev_name,
				   unsigned int vendor_id,
				   unsigned int sub_vendor_id,
				   unsigned int device_id,
				   unsigned int sub_device_id);


/**
 * wifi_nrf_osal_bus_pcie_deinit() - Deinitialize a PCIe device driver.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_priv: OS specific PCIe context.
 *
 * This API should be called when the PCIe device driver is
 * to be unregistered from the OS's PCIe core.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_pcie_deinit(struct wifi_nrf_osal_priv *opriv,
				    void *os_pcie_priv);


/**
 * wifi_nrf_osal_bus_pcie_dev_add() - Add a PCIe device instance.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @osal_pcie_dev_ctx: Pointer to the OSAL PCIe device context.
 *
 * Function to be invoked when a matching PCIe device is added to the system.
 * It expects the pointer to a OS specific PCIe device context to be returned.
 *
 * Return: OS specific PCIe device context.
 */
void *wifi_nrf_osal_bus_pcie_dev_add(struct wifi_nrf_osal_priv *opriv,
				      void *os_pcie_priv,
				      void *osal_pcie_dev_ctx);


/**
 * wifi_nrf_osal_bus_pcie_dev_rem() - Remove a PCIe device instance.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_dev_ctx: Pointer to the OS specific PCIe device context which was
 *                   returned by @wifi_nrf_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a matching PCIe device is removed from the system.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_pcie_dev_rem(struct wifi_nrf_osal_priv *opriv,
				     void *os_pcie_dev_ctx);


/**
 * wifi_nrf_osal_bus_pcie_dev_init() - Initialize a PCIe device instance.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_dev_ctx: Pointer to the OS specific PCIe device context which was
 *                   returned by @wifi_nrf_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a PCIe device is to be initialized.
 *
 * Return:
 *		Pass: WIFI_NRF_STATUS_SUCCESS.
 *		Fail: WIFI_NRF_STATUS_FAIL.
 */
enum wifi_nrf_status wifi_nrf_osal_bus_pcie_dev_init(struct wifi_nrf_osal_priv *opriv,
						       void *os_pcie_dev_ctx);


/**
 * wifi_nrf_osal_bus_pcie_dev_deinit() - Deinitialize a PCIe device instance.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_dev_ctx: Pointer to the OS specific PCIe device context which was
 *                   returned by @wifi_nrf_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a PCIe device is to be deinitialized.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_pcie_dev_deinit(struct wifi_nrf_osal_priv *opriv,
					void *os_pcie_dev_ctx);


/**
 * wifi_nrf_osal_bus_pcie_dev_intr_reg() - Register a interrupt handler for a PCIe device.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_dev_ctx: OS specific PCIe device context.
 * @callbk_data: Data to be passed to the ISR.
 * @callbk_fn: ISR to be invoked on receiving an interrupt.
 *
 * Registers an interrupt handler to the OS. This API also passes the callback
 * data to be passed to the ISR when an interrupt is received.
 *
 * Return:
 *	    Pass: WIFI_NRF_STATUS_SUCCESS.
 *	    Fail: WIFI_NRF_STATUS_FAIL.
 */
enum wifi_nrf_status wifi_nrf_osal_bus_pcie_dev_intr_reg(struct wifi_nrf_osal_priv *opriv,
							   void *os_pcie_dev_ctx,
							   void *callbk_data,
							   int (*callbk_fn)(void *callbk_data));


/**
 * wifi_nrf_osal_bus_pcie_dev_intr_unreg() - Unregister an interrupt handler for a PCIe device.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_dev_ctx: OS specific PCIe device context.
 *
 * Unregisters the interrupt handler that was registered using
 * @wifi_nrf_osal_bus_pcie_dev_intr_reg.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_pcie_dev_intr_unreg(struct wifi_nrf_osal_priv *opriv,
					    void *os_pcie_dev_ctx);


/**
 * wifi_nrf_osal_bus_pcie_dev_dma_map() - Map host memory for DMA access.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_dev_ctx: Pointer to a OS specific PCIe device handle.
 * @virt_addr: Virtual host address to be DMA mapped.
 * @size: Size in bytes of the host memory to be DMA mapped.
 * @dir: DMA direction.
 *
 * Maps host memory of @size bytes pointed to by the virtual address
 * @virt_addr to be used by the device(@dma_dev) for DMAing contents.
 * The contents are available for DMAing to the device if @dir has a
 * value of @WIFI_NRF_OSAL_DMA_DIR_TO_DEV. Conversely the device can DMA
 * contents to the host memory if @dir has a value of
 * @WIFI_NRF_OSAL_DMA_DIR_FROM_DEV. The function returns the DMA address
 * of the mapped memory.
 *
 * Return:
 *		Pass: Pointer to the DMA mapped physical address.
 *		Error: NULL.
 */
void *wifi_nrf_osal_bus_pcie_dev_dma_map(struct wifi_nrf_osal_priv *opriv,
					  void *os_pcie_dev_ctx,
					  void *virt_addr,
					  size_t size,
					  enum wifi_nrf_osal_dma_dir dir);


/**
 * wifi_nrf_osal_bus_pcie_dev_dma_unmap() - Unmap DMA mapped host memory.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_dev_ctx: Pointer to a OS specific PCIe device handle.
 * @dma_addr: DMA mapped physical host memory address.
 * @size: Size in bytes of the DMA mapped host memory.
 * @dir: DMA direction.
 *
 * Unmaps the host memory which was mapped for DMA using @wifi_nrf_osal_dma_map.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_pcie_dev_dma_unmap(struct wifi_nrf_osal_priv *opriv,
					   void *os_pcie_dev_ctx,
					   void *dma_addr,
					   size_t size,
					   enum wifi_nrf_osal_dma_dir dir);


/**
 * wifi_nrf_osal_bus_pcie_dev_host_map_get() - Get host mapped address for a PCIe device.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_pcie_dev_ctx: OS specific PCIe device context.
 * @host_map: Host map address information.
 *
 * Gets the host map address for a PCIe device.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_pcie_dev_host_map_get(struct wifi_nrf_osal_priv *opriv,
					      void *os_pcie_dev_ctx,
					      struct wifi_nrf_osal_host_map *host_map);




/**
 * wifi_nrf_osal_bus_qspi_init() - Initialize a qspi driver.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 *
 * Registers a qspi device driver to the OS's qspi core.
 *
 * Return: OS specific qspi device context.
 */
void *wifi_nrf_osal_bus_qspi_init(struct wifi_nrf_osal_priv *opriv);


/**
 * wifi_nrf_osal_bus_qspi_deinit() - Deinitialize a qspi device driver.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_qspi_priv: OS specific qspi context.
 *
 * This API should be called when the qspi device driver is
 * to be unregistered from the OS's qspi core.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_qspi_deinit(struct wifi_nrf_osal_priv *opriv,
				    void *os_qspi_priv);


/**
 * wifi_nrf_osal_bus_qspi_dev_add() - Add a qspi device instance.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @osal_qspi_dev_ctx: Pointer to the OSAL qspi device context.
 *
 * Function to be invoked when a matching qspi device is added to the system.
 * It expects the pointer to a OS specific qspi device context to be returned.
 *
 * Return: OS specific qspi device context.
 */
void *wifi_nrf_osal_bus_qspi_dev_add(struct wifi_nrf_osal_priv *opriv,
				      void *os_qspi_priv,
				      void *osal_qspi_dev_ctx);


/**
 * wifi_nrf_osal_bus_qspi_dev_rem() - Remove a qspi device instance.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                   returned by @wifi_nrf_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a matching qspi device is removed from the system.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_qspi_dev_rem(struct wifi_nrf_osal_priv *opriv,
				     void *os_qspi_dev_ctx);


/**
 * wifi_nrf_osal_bus_qspi_dev_init() - Initialize a qspi device instance.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                   returned by @wifi_nrf_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a qspi device is to be initialized.
 *
 * Return:
 *		Pass: WIFI_NRF_STATUS_SUCCESS.
 *		Fail: WIFI_NRF_STATUS_FAIL.
 */
enum wifi_nrf_status wifi_nrf_osal_bus_qspi_dev_init(struct wifi_nrf_osal_priv *opriv,
						       void *os_qspi_dev_ctx);


/**
 * wifi_nrf_osal_bus_qspi_dev_deinit() - Deinitialize a qspi device instance.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                   returned by @wifi_nrf_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a qspi device is to be deinitialized.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_qspi_dev_deinit(struct wifi_nrf_osal_priv *opriv,
					void *os_qspi_dev_ctx);


/**
 * wifi_nrf_osal_bus_qspi_dev_intr_reg() - Register a interrupt handler for a qspi device.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_qspi_dev_ctx: OS specific qspi device context.
 * @callbk_data: Data to be passed to the ISR.
 * @callbk_fn: ISR to be invoked on receiving an interrupt.
 *
 * Registers an interrupt handler to the OS. This API also passes the callback
 * data to be passed to the ISR when an interrupt is received.
 *
 * Return:
 *	    Pass: WIFI_NRF_STATUS_SUCCESS.
 *	    Fail: WIFI_NRF_STATUS_FAIL.
 */
enum wifi_nrf_status wifi_nrf_osal_bus_qspi_dev_intr_reg(struct wifi_nrf_osal_priv *opriv,
							   void *os_qspi_dev_ctx,
							   void *callbk_data,
							   int (*callbk_fn)(void *callbk_data));


/**
 * wifi_nrf_osal_bus_qspi_dev_intr_unreg() - Unregister an interrupt handler for a qspi device.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_qspi_dev_ctx: OS specific qspi device context.
 *
 * Unregisters the interrupt handler that was registered using
 * @wifi_nrf_osal_bus_qspi_dev_intr_reg.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_qspi_dev_intr_unreg(struct wifi_nrf_osal_priv *opriv,
					    void *os_qspi_dev_ctx);


/**
 * wifi_nrf_osal_bus_qspi_dev_host_map_get() - Get host mapped address for a qspi device.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @os_qspi_dev_ctx: OS specific qspi device context.
 * @host_map: Host map address information.
 *
 * Gets the host map address for a qspi device.
 *
 * Return: None.
 */
void wifi_nrf_osal_bus_qspi_dev_host_map_get(struct wifi_nrf_osal_priv *opriv,
					      void *os_qspi_dev_ctx,
					      struct wifi_nrf_osal_host_map *host_map);

/**
 * wifi_nrf_osal_qspi_read_reg32() - Read value from a 32 bit register on a
 *                                  QSPI slave device.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @priv:
 * @addr: Address of the register to read from.
 *
 * Reads value from a 32 bit device register at address(@addr) on
 * a QSPI slave device.
 *
 * Return: 32 bit value read from register.
 */
unsigned int wifi_nrf_osal_qspi_read_reg32(struct wifi_nrf_osal_priv *opriv,
					    void *priv,
					    unsigned long addr);

/**
 * wifi_nrf_osal_qspi_write_reg32() -
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @priv:
 * @addr: Address of the register to write to.
 * @val: Value to be written to the register.
 *
 * Writes a 32 bit value (@val) to a 32 bit device register at address(@addr)
 * on a QSPI slave device.
 *
 * Return: None.
 */
void wifi_nrf_osal_qspi_write_reg32(struct wifi_nrf_osal_priv *opriv,
				     void *priv,
				     unsigned long addr,
				     unsigned int val);

/**
 * wifi_nrf_osal_qspi_cpy_from() -
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @priv:
 * @dest:
 * @src:
 * @count:
 *
 *
 * Return: None.
 */
void wifi_nrf_osal_qspi_cpy_from(struct wifi_nrf_osal_priv *opriv,
				  void *priv,
				  void *dest,
				  unsigned long addr,
				  size_t count);

/**
 * wifi_nrf_osal_qspi_cpy_to() -
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @priv:
 * @dest:
 * @src:
 * @count:
 *
 *
 * Return: None.
 */
void wifi_nrf_osal_qspi_cpy_to(struct wifi_nrf_osal_priv *opriv,
				void *priv,
				unsigned long addr,
				const void *src,
				size_t count);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
/**
 * wifi_nrf_osal_timer_alloc() - Allocate a timer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 *
 * Allocates a timer instance.
 *
 * Return: Pointer to the allocated timer instance.
 */
void *wifi_nrf_osal_timer_alloc(struct wifi_nrf_osal_priv *opriv);


/**
 * wifi_nrf_osal_timer_free() - Free a timer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @timer: Pointer to a timer instance.
 *
 * Frees/Deallocates a timer that has been allocated using
 * @wifi_nrf_osal_timer_alloc.
 *
 * Return: None.
 */
void wifi_nrf_osal_timer_free(struct wifi_nrf_osal_priv *opriv,
			       void *timer);


/**
 * wifi_nrf_osal_timer_init() - Initialize a timer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @timer: Pointer to a timer instance.
 * @callbk_fn: Callback function to be invoked when the timer expires.
 * @data: Data to be passed to the callback function.
 *
 * Initializes a timer that has been allocated using @wifi_nrf_osal_timer_alloc
 * Need to pass (@callbk_fn) callback function with the data(@data) to be
 * passed to the callback function, whenever the timer expires.
 *
 * Return: None.
 */
void wifi_nrf_osal_timer_init(struct wifi_nrf_osal_priv *opriv,
			       void *timer,
			       void (*callbk_fn)(unsigned long),
			       unsigned long data);


/**
 * wifi_nrf_osal_timer_schedule() - Schedule a timer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @timer: Pointer to a timer instance.
 * @duration: Duration of the timer in seconds.
 *
 * Schedules a timer with a @duration seconds that has been allocated using
 * @wifi_nrf_osal_timer_alloc and initialized with @wifi_nrf_osal_timer_init.
 *
 * Return: None.
 */
void wifi_nrf_osal_timer_schedule(struct wifi_nrf_osal_priv *opriv,
				   void *timer,
				   unsigned long duration);


/**
 * wifi_nrf_osal_timer_kill() - Kill a timer.
 * @opriv: Pointer to the OSAL context returned by the @wifi_nrf_osal_init API.
 * @timer: Pointer to a timer instance.
 *
 * Kills a timer that has been scheduled using @wifi_nrf_osal_timer_schedule.
 *
 * Return: None.
 */
void wifi_nrf_osal_timer_kill(struct wifi_nrf_osal_priv *opriv,
			       void *timer);



int wifi_nrf_osal_bus_qspi_ps_sleep(struct wifi_nrf_osal_priv *opriv,
				    void *os_qspi_priv);

int wifi_nrf_osal_bus_qspi_ps_wake(struct wifi_nrf_osal_priv *opriv,
				   void *os_qspi_priv);

int wifi_nrf_osal_bus_qspi_ps_status(struct wifi_nrf_osal_priv *opriv,
				     void *os_qspi_priv);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
#endif /* __OSAL_API_H__ */

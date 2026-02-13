/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing the OPs declarations for the Bus Abstraction Layer
 * (BAL) of the Wi-Fi driver.
 */

#ifndef __BAL_OPS_H__
#define __BAL_OPS_H__

/**
 * @brief Ops to be provided by a particular bus implementation.
 *
 * This structure defines the operations that need to be implemented
 * by a specific bus implementation.
 */
struct nrf_wifi_bal_ops {
	/**
	 * @brief Initialize the bus.
	 *
	 * @param cfg_params Pointer to the configuration parameters.
	 * @param intr_callbk_fn Pointer to the interrupt callback function.
	 * @return Pointer to the initialized instance of the bus.
	 */
	void * (*init)(void *cfg_params,
		       enum nrf_wifi_status (*intr_callbk_fn)(void *hal_ctx));

	/**
	 * @brief Deinitialize the bus.
	 *
	 * @param bus_priv Pointer to the bus private data.
	 */
	void (*deinit)(void *bus_priv);

	/**
	 * @brief Add a device to the bus.
	 *
	 * @param bus_priv Pointer to the bus private data.
	 * @param bal_dev_ctx Pointer to the BAL device context.
	 * @return Pointer to the added device context.
	 */
	void * (*dev_add)(void *bus_priv,
			  void *bal_dev_ctx);

	/**
	 * @brief Remove a device from the bus.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 */
	void (*dev_rem)(void *bus_dev_ctx);

	/**
	 * @brief Initialize a device on the bus.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 * @return Status of the device initialization.
	 */
	enum nrf_wifi_status (*dev_init)(void *bus_dev_ctx);

	/**
	 * @brief Deinitialize a device on the bus.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 */
	void (*dev_deinit)(void *bus_dev_ctx);

	/**
	 * @brief Read a word from the bus.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 * @param addr_offset Address offset.
	 * @return The read word.
	 */
	unsigned int (*read_word)(void *bus_dev_ctx,
				  unsigned long addr_offset);

	/**
	 * @brief Write a word to the bus.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 * @param addr_offset Address offset.
	 * @param val Value to write.
	 */
	void (*write_word)(void *bus_dev_ctx,
			   unsigned long addr_offset,
			   unsigned int val);

	/**
	 * @brief Read a block of data from the bus.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 * @param dest_addr Destination address.
	 * @param src_addr_offset Source address offset.
	 * @param len Length of the block to read.
	 */
	void (*read_block)(void *bus_dev_ctx,
			   void *dest_addr,
			   unsigned long src_addr_offset,
			   size_t len);

	/**
	 * @brief Write a block of data to the bus.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 * @param dest_addr_offset Destination address offset.
	 * @param src_addr Pointer to the source address.
	 * @param len Length of the block to write.
	 */
	void (*write_block)(void *bus_dev_ctx,
				unsigned long dest_addr_offset,
				const void *src_addr,
				size_t len);

	/**
	 * @brief Map a DMA buffer.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 * @param virt_addr Virtual address of the buffer.
	 * @param len Length of the buffer.
	 * @param dma_dir DMA direction.
	 * @return Physical address of the mapped buffer.
	 */
	unsigned long (*dma_map)(void *bus_dev_ctx,
				 unsigned long virt_addr,
				 size_t len,
				 enum nrf_wifi_osal_dma_dir dma_dir);

	/**
	 * @brief Unmap a DMA buffer.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 * @param phy_addr Physical address of the buffer.
	 * @param len Length of the buffer.
	 * @param dma_dir DMA direction.
	 * @return Physical address of the unmapped buffer.
	 */
	unsigned long (*dma_unmap)(void *bus_dev_ctx,
				   unsigned long phy_addr,
				   size_t len,
				   enum nrf_wifi_osal_dma_dir dma_dir);

#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
	/**
	 * @brief Put the device into power-saving sleep mode.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 */
	void (*rpu_ps_sleep)(void *bus_dev_ctx);

	/**
	 * @brief Wake the device from power-saving sleep mode.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 */
	void (*rpu_ps_wake)(void *bus_dev_ctx);

	/**
	 * @brief Get the power-saving status of the device.
	 *
	 * @param bus_dev_ctx Pointer to the bus device context.
	 * @return Power-saving status of the device.
	 */
	int (*rpu_ps_status)(void *bus_dev_ctx);
#endif /* NRF_WIFI_LOW_POWER */
};

/**
 * @brief Get the bus operations.
 *
 * @return Pointer to the bus operations.
 */
struct nrf_wifi_bal_ops *get_bus_ops(void);
#endif /* __BAL_OPS_H__ */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing the API declarations for the Bus Abstraction Layer
 * (BAL) of the Wi-Fi driver.
 */

#ifndef __BAL_API_H__
#define __BAL_API_H__

#include "osal_api.h"
#include "bal_structs.h"

/**
 * @brief  Initialize the BAL layer.
 *
 * @param cfg_params Pointer to the configuration parameters for the BAL layer.
 * @param intr_callbk_fn Pointer to the callback function which the user of this
 *	       layer needs to implement to handle interrupts from the RPU.
 *
 * This API is used to initialize the BAL layer and is expected to be called
 * before using the BAL layer. This API returns a pointer to the BAL context
 * which might need to be passed to further API calls.
 *
 * @return Pointer to instance of BAL layer context.
 */
struct nrf_wifi_bal_priv *nrf_wifi_bal_init(struct nrf_wifi_bal_cfg_params *cfg_params,
					    enum nrf_wifi_status (*intr_callbk_fn)(void *hal_ctx));


/**
 * @brief Deinitialize the BAL layer.
 *
 * This API is used to deinitialize the BAL layer and is expected to be called
 * after done using the BAL layer.
 *
 * @param bpriv Pointer to the BAL layer context returned by the
 *              @ref nrf_wifi_bal_init API.
 */
void nrf_wifi_bal_deinit(struct nrf_wifi_bal_priv *bpriv);

/**
 * @brief Add a device context to the BAL layer.
 *
 * @param bpriv Pointer to the BAL layer context returned by the
 *              @ref nrf_wifi_bal_init API.
 * @param hal_dev_ctx Pointer to the HAL device context.
 *
 * @return Pointer to the added device context.
 */
struct nrf_wifi_bal_dev_ctx *nrf_wifi_bal_dev_add(struct nrf_wifi_bal_priv *bpriv,
	void *hal_dev_ctx);

/**
 * @brief Remove a device context from the BAL layer.
 *
 * @param bal_dev_ctx Pointer to the device context returned by the
 *                    @ref nrf_wifi_bal_dev_add API.
 */
void nrf_wifi_bal_dev_rem(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx);

/**
 * @brief Initialize a device context in the BAL layer.
 *
 * @param bal_dev_ctx Pointer to the device context returned by the
 *                    @ref nrf_wifi_bal_dev_add API.
 *
 * @return Status of the device initialization.
 */
enum nrf_wifi_status nrf_wifi_bal_dev_init(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx);

/**
 * @brief Deinitialize a device context in the BAL layer.
 *
 * @param bal_dev_ctx Pointer to the device context returned by the
 *                    @ref nrf_wifi_bal_dev_add API.
 */
void nrf_wifi_bal_dev_deinit(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx);

/**
 * @brief Read a word from a specific address offset.
 *
 * @param ctx Pointer to the context.
 * @param addr_offset Address offset to read from.
 *
 * @return The read word.
 */
unsigned int nrf_wifi_bal_read_word(void *ctx, unsigned long addr_offset);

/**
 * @brief Write a word to a specific address offset.
 *
 * @param ctx Pointer to the context.
 * @param addr_offset Address offset to write to.
 * @param val Value to write.
 */
void nrf_wifi_bal_write_word(void *ctx,
		unsigned long addr_offset,
		unsigned int val);

/**
 * @brief Read a block of data from a specific address offset.
 *
 * @param ctx Pointer to the context.
 * @param dest_addr Pointer to the destination address.
 * @param src_addr_offset Source address offset to read from.
 * @param len Length of the data to read.
 */
void nrf_wifi_bal_read_block(void *ctx,
		void *dest_addr,
		unsigned long src_addr_offset,
		size_t len);

/**
 * @brief Write a block of data to a specific address offset.
 *
 * @param ctx Pointer to the context.
 * @param dest_addr_offset Destination address offset to write to.
 * @param src_addr Pointer to the source address.
 * @param len Length of the data to write.
 */
void nrf_wifi_bal_write_block(void *ctx,
		unsigned long dest_addr_offset,
		const void *src_addr,
		size_t len);

/**
 * @brief Map a virtual address to a physical address for DMA transfer.
 *
 * @param ctx Pointer to the context.
 * @param virt_addr Virtual address to map.
 * @param len Length of the data to map.
 * @param dma_dir DMA direction.
 *
 * @return The mapped physical address.
 */
unsigned long nrf_wifi_bal_dma_map(void *ctx,
		unsigned long virt_addr,
		size_t len,
		enum nrf_wifi_osal_dma_dir dma_dir);

/**
 * @brief Unmap a physical address for DMA transfer.
 *
 * @param ctx Pointer to the context.
 * @param phy_addr Physical address to unmap.
 * @param len Length of the data to unmap.
 * @param dma_dir DMA direction.
 */
unsigned long nrf_wifi_bal_dma_unmap(void *ctx,
		unsigned long phy_addr,
		size_t len,
		enum nrf_wifi_osal_dma_dir dma_dir);

/**
 * @brief Enable bus access recording.
 *
 * @param ctx Pointer to the context.
 */
void nrf_wifi_bal_bus_access_rec_enab(void *ctx);

/**
 * @brief Disable bus access recording.
 *
 * @param ctx Pointer to the context.
 */
void nrf_wifi_bal_bus_access_rec_disab(void *ctx);

/**
 * @brief Print bus access count.
 *
 * @param ctx Pointer to the context.
 */
void nrf_wifi_bal_bus_access_cnt_print(void *ctx);

#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
/**
 * @brief Put the RPU to sleep.
 *
 * @param ctx Pointer to the context.
 */
void nrf_wifi_bal_rpu_ps_sleep(void *ctx);

/**
 * @brief Wake up the RPU from sleep.
 *
 * @param ctx Pointer to the context.
 */
void nrf_wifi_bal_rpu_ps_wake(void *ctx);

/**
 * @brief Get the power-saving status of the RPU.
 *
 * @param ctx Pointer to the context.
 *
 * @return Power-saving status.
 */
int nrf_wifi_bal_rpu_ps_status(void *ctx);
#endif /* NRF_WIFI_LOW_POWER */
#endif /* __BAL_API_H__ */

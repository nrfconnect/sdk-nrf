/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
 * nrf_wifi_bal_init() - Initialize the BAL layer.
 *
 * @intr_callbk_fn: Pointer to the callback function which the user of this
 *                  layer needs to implement to handle interrupts from the
 *                  RPU.
 *
 * This API is used to initialize the BAL layer and is expected to be called
 * before using the BAL layer. This API returns a pointer to the BAL context
 * which might need to be passed to further API calls.
 *
 * Returns: Pointer to instance of BAL layer context.
 */
struct nrf_wifi_bal_priv *nrf_wifi_bal_init(struct nrf_wifi_osal_priv *opriv,
					    struct nrf_wifi_bal_cfg_params *cfg_params,
					    enum nrf_wifi_status (*intr_callbk_fn)(void *hal_ctx));


/**
 * nrf_wifi_bal_deinit() - Deinitialize the BAL layer.
 * @bpriv: Pointer to the BAL layer context returned by the
 *         @nrf_wifi_bal_init API.
 *
 * This API is used to deinitialize the BAL layer and is expected to be called
 * after done using the BAL layer.
 *
 * Returns: None.
 */
void nrf_wifi_bal_deinit(struct nrf_wifi_bal_priv *bpriv);


struct nrf_wifi_bal_dev_ctx *nrf_wifi_bal_dev_add(struct nrf_wifi_bal_priv *bpriv,
						  void *hal_dev_ctx);

void nrf_wifi_bal_dev_rem(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx);

enum nrf_wifi_status nrf_wifi_bal_dev_init(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx);

void nrf_wifi_bal_dev_deinit(struct nrf_wifi_bal_dev_ctx *bal_dev_ctx);

unsigned int nrf_wifi_bal_read_word(void *ctx,
				    unsigned long addr_offset);

void nrf_wifi_bal_write_word(void *ctx,
			     unsigned long addr_offset,
			     unsigned int val);

void nrf_wifi_bal_read_block(void *ctx,
			     void *dest_addr,
			     unsigned long src_addr_offset,
			     size_t len);

void nrf_wifi_bal_write_block(void *ctx,
			      unsigned long dest_addr_offset,
			      const void *src_addr,
			      size_t len);

unsigned long nrf_wifi_bal_dma_map(void *ctx,
				   unsigned long virt_addr,
				   size_t len,
				   enum nrf_wifi_osal_dma_dir dma_dir);

unsigned long nrf_wifi_bal_dma_unmap(void *ctx,
				     unsigned long phy_addr,
				     size_t len,
				     enum nrf_wifi_osal_dma_dir dma_dir);

void nrf_wifi_bal_bus_access_rec_enab(void *ctx);

void nrf_wifi_bal_bus_access_rec_disab(void *ctx);

void nrf_wifi_bal_bus_access_cnt_print(void *ctx);

#ifdef CONFIG_NRF_WIFI_LOW_POWER
void nrf_wifi_bal_rpu_ps_sleep(void *ctx);

void nrf_wifi_bal_rpu_ps_wake(void *ctx);

int nrf_wifi_bal_rpu_ps_status(void *ctx);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
#endif /* __BAL_API_H__ */

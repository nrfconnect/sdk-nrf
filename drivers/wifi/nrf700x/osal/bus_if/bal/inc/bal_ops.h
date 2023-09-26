/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing the OPs declarations for the Bus Abstraction Layer
 * (BAL) of the Wi-Fi driver.
 */

#ifndef __BAL_OPS_H__
#define __BAL_OPS_H__

#include <stdbool.h>

/**
 * struct nrf_wifi_bal_ops - Ops to be provided by a particular bus
 *                           implementation.
 * @init:
 * @deinit:
 * @dev_init:
 * @dev_deinit:
 * @read_word:
 * @write_word:
 * @read_block:
 * @write_block:
 * @dma_map:
 * @dma_unmap:
 */
struct nrf_wifi_bal_ops {
	void * (*init)(struct nrf_wifi_osal_priv *opriv,
		       void *cfg_params,
		       enum nrf_wifi_status (*intr_callbk_fn)(void *hal_ctx));
	void (*deinit)(void *bus_priv);
	void * (*dev_add)(void *bus_priv,
			  void *bal_dev_ctx);
	void (*dev_rem)(void *bus_dev_ctx);

	enum nrf_wifi_status (*dev_init)(void *bus_dev_ctx);
	void (*dev_deinit)(void *bus_dev_ctx);
	unsigned int (*read_word)(void *bus_dev_ctx,
				  unsigned long addr_offset);
	void (*write_word)(void *bus_dev_ctx,
			   unsigned long addr_offset,
			   unsigned int val);
	void (*read_block)(void *bus_dev_ctx,
			   void *dest_addr,
			   unsigned long src_addr_offset,
			   size_t len);
	void (*write_block)(void *bus_dev_ctx,
			    unsigned long dest_addr_offset,
			    const void *src_addr,
			    size_t len);
	unsigned long (*dma_map)(void *bus_dev_ctx,
				 unsigned long virt_addr,
				 size_t len,
				 enum nrf_wifi_osal_dma_dir dma_dir);
	unsigned long (*dma_unmap)(void *bus_dev_ctx,
				   unsigned long phy_addr,
				   size_t len,
				   enum nrf_wifi_osal_dma_dir dma_dir);
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	void (*rpu_ps_sleep)(void *bus_dev_ctx);
	void (*rpu_ps_wake)(void *bus_dev_ctx);
	int (*rpu_ps_status)(void *bus_dev_ctx);
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
};


/**
 * get_bus_ops() - The BAL layer expects this Op return a initialized instance
 *                of Bus specific Ops.
 *
 * This Op is expected to be implemented by a specific Bus shim and is expected
 * to return a pointer to a initialized instance of struct nrf_wifi_bal_ops.
 *
 * Returns: Pointer to instance of Bus specific Ops.
 */
struct nrf_wifi_bal_ops *get_bus_ops(void);
#endif /* __BAL_OPS_H__ */

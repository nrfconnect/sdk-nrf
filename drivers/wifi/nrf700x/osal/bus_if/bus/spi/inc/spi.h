/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header for the Linux SPI bus layer specific structure declarations of the
 * Wi-Fi driver.
 */

#ifndef __LINUX_SPI_H__
#define __LINUX_SPI_H__

#include "osal_structs.h"
#include "bal_structs.h"

/**
 * struct wifi_nrf_bus_spi_priv - Structure to hold context information for the Linux SPI bus.
 * @opriv: Pointer to the OSAL context.
 * @os_spi_priv:
 * @intr_callbk_fn:
 * @cfg_params:
 *
 * This structure maintains the context information necessary for the operation
 * of the Linux SPI bus. Some of the elements of the structure need to be initialized
 * during the initialization of the Linux SPI bus while others need to be kept
 * updated over the duration of the Linux SPI bus operation.
 */
struct wifi_nrf_bus_spi_priv {
	struct wifi_nrf_osal_priv *opriv;
	void *os_spi_priv;

	enum wifi_nrf_status (*intr_callbk_fn)(void *hal_ctx);

	/* TODO: See if this can be removed by getting the information from PAL */
	struct wifi_nrf_bal_cfg_params cfg_params;
};


struct wifi_nrf_bus_spi_dev_ctx {
	struct wifi_nrf_bus_spi_priv *spi_priv;
	void *bal_dev_ctx;
	void *os_spi_dev_ctx;

	unsigned long host_addr_base;
	unsigned long addr_pktram_base;
};
#endif /* __LINUX_SPI_H__ */

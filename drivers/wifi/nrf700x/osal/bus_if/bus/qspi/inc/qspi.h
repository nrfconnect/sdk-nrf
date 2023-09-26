/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header for the QSPI bus layer specific structure declarations of the
 * Wi-Fi driver.
 */

#ifndef __QSPI_H__
#define __QSPI_H__

/**
 * struct nrf_wifi_bus_qspi_priv - Structure to hold context information for the QSPI bus.
 * @opriv: Pointer to the OSAL context.
 * @os_qspi_priv:
 * @intr_callbk_fn:
 * @cfg_params:
 *
 * This structure maintains the context information necessary for the operation
 * of the QSPI bus. Some of the elements of the structure need to be initialized
 * during the initialization of the QSPI bus while others need to be kept
 * updated over the duration of the QSPI bus operation.
 */
struct nrf_wifi_bus_qspi_priv {
	struct nrf_wifi_osal_priv *opriv;
	void *os_qspi_priv;

	enum nrf_wifi_status (*intr_callbk_fn)(void *hal_ctx);

	/* TODO: See if this can be removed by getting the information from PAL */
	struct nrf_wifi_bal_cfg_params cfg_params;
};


struct nrf_wifi_bus_qspi_dev_ctx {
	struct nrf_wifi_bus_qspi_priv *qspi_priv;
	void *bal_dev_ctx;
	void *os_qspi_dev_ctx;

	unsigned long host_addr_base;
	unsigned long addr_pktram_base;
};
#endif /* __QSPI_H__ */

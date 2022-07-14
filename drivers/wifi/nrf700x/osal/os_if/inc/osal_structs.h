/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing structure declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_STRUCTS_H__
#define __OSAL_STRUCTS_H__

#include <stddef.h>

/**
 * enum wifi_nrf_status - The status of an operation performed by the
 *                        RPU driver.
 * @WIFI_NRF_STATUS_SUCCESS: The operation was successful.
 * @WIFI_NRF_STATUS_FAIL: The operation failed.
 *
 * This enum lists the possible outcomes of an operation performed by the
 * RPU driver.
 */
enum wifi_nrf_status {
	WIFI_NRF_STATUS_SUCCESS,
	WIFI_NRF_STATUS_FAIL = -1,
};


/**
 * enum wifi_nrf_osal_dma_dir - DMA direction for a DMA operation
 * @WIFI_NRF_OSAL_DMA_DIR_TO_DEV: Data needs to be DMAed to the device.
 * @WIFI_NRF_DMA_DIR_FROM_DEV: Data needs to be DMAed from the device.
 * @WIFI_NRF_DMA_DIR_BIDI: Data can be DMAed in either direction i.e to
 *                        or from the device.
 *
 * This enum lists the possible directions for a DMA operation i.e whether the
 * DMA operation is for transferring data to or from a device
 */
enum wifi_nrf_osal_dma_dir {
	WIFI_NRF_OSAL_DMA_DIR_TO_DEV,
	WIFI_NRF_OSAL_DMA_DIR_FROM_DEV,
	WIFI_NRF_OSAL_DMA_DIR_BIDI
};


struct wifi_nrf_osal_host_map {
	unsigned long addr;
	unsigned long size;
};


struct wifi_nrf_osal_priv {
	const struct wifi_nrf_osal_ops *ops;
};
#endif /* __OSAL_STRUCTS_H__ */

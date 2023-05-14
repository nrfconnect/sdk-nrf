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

/**
 * enum wifi_nrf_tasklet_type - The type of a tasklet
 * @WIFI_NRF_TASKLET_TYPE_BH: The tasklet is a bottom half tasklet i.e it is
 *		scheduled from an interrupt context used for all except
 *		TX done tasklets.
 * @WIFI_NRF_TASKLET_TYPE_IRQ: The tasklet is an IRQ tasklet. It is scheduled
 *		from the Bus ISR, used internally by the SHIM layer.
 * @WIFI_NRF_TASKLET_TYPE_TX_DONE: The tasklet is a TX done tasklet. It is
 *		scheduled from the BH tasklet for TX done interrupts.
 * @WIFI_NRF_TASKLET_TYPE_RX: The tasklet is an RX tasklet. It is scheduled
 *		from the BH tasklet for RX interrupts.
 * @WIFI_NRF_TASKLET_TYPE_MAX: The maximum number of tasklet types.
 *
 * This enum lists the possible types of a tasklet.
 * Each tasklet type is associated with its own work queue.
 */
enum wifi_nrf_tasklet_type {
	WIFI_NRF_TASKLET_TYPE_BH,
	WIFI_NRF_TASKLET_TYPE_IRQ,
	WIFI_NRF_TASKLET_TYPE_TX_DONE,
	WIFI_NRF_TASKLET_TYPE_RX,
	WIFI_NRF_TASKLET_TYPE_MAX
};

struct wifi_nrf_osal_host_map {
	unsigned long addr;
	unsigned long size;
};


struct wifi_nrf_osal_priv {
	const struct wifi_nrf_osal_ops *ops;
};
#endif /* __OSAL_STRUCTS_H__ */

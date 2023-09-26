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
 * enum nrf_wifi_status - The status of an operation performed by the
 *                        RPU driver.
 * @NRF_WIFI_STATUS_SUCCESS: The operation was successful.
 * @NRF_WIFI_STATUS_FAIL: The operation failed.
 *
 * This enum lists the possible outcomes of an operation performed by the
 * RPU driver.
 */
enum nrf_wifi_status {
	NRF_WIFI_STATUS_SUCCESS,
	NRF_WIFI_STATUS_FAIL = -1,
};


/**
 * enum nrf_wifi_osal_dma_dir - DMA direction for a DMA operation
 * @NRF_WIFI_OSAL_DMA_DIR_TO_DEV: Data needs to be DMAed to the device.
 * @NRF_WIFI_DMA_DIR_FROM_DEV: Data needs to be DMAed from the device.
 * @NRF_WIFI_DMA_DIR_BIDI: Data can be DMAed in either direction i.e to
 *                        or from the device.
 *
 * This enum lists the possible directions for a DMA operation i.e whether the
 * DMA operation is for transferring data to or from a device
 */
enum nrf_wifi_osal_dma_dir {
	NRF_WIFI_OSAL_DMA_DIR_TO_DEV,
	NRF_WIFI_OSAL_DMA_DIR_FROM_DEV,
	NRF_WIFI_OSAL_DMA_DIR_BIDI
};

/**
 * enum nrf_wifi_tasklet_type - The type of a tasklet
 * @NRF_WIFI_TASKLET_TYPE_BH: The tasklet is a bottom half tasklet i.e it is
 *		scheduled from an interrupt context used for all except
 *		TX done tasklets.
 * @NRF_WIFI_TASKLET_TYPE_IRQ: The tasklet is an IRQ tasklet. It is scheduled
 *		from the Bus ISR, used internally by the SHIM layer.
 * @NRF_WIFI_TASKLET_TYPE_TX_DONE: The tasklet is a TX done tasklet. It is
 *		scheduled from the BH tasklet for TX done interrupts.
 * @NRF_WIFI_TASKLET_TYPE_RX: The tasklet is an RX tasklet. It is scheduled
 *		from the BH tasklet for RX interrupts.
 * @NRF_WIFI_TASKLET_TYPE_MAX: The maximum number of tasklet types.
 *
 * This enum lists the possible types of a tasklet.
 * Each tasklet type is associated with its own work queue.
 */
enum nrf_wifi_tasklet_type {
	NRF_WIFI_TASKLET_TYPE_BH,
	NRF_WIFI_TASKLET_TYPE_IRQ,
	NRF_WIFI_TASKLET_TYPE_TX_DONE,
	NRF_WIFI_TASKLET_TYPE_RX,
	NRF_WIFI_TASKLET_TYPE_MAX
};

struct nrf_wifi_osal_host_map {
	unsigned long addr;
	unsigned long size;
};


struct nrf_wifi_osal_priv {
	const struct nrf_wifi_osal_ops *ops;
};

/**
 * enum nrf_wifi_assert_type - The type of assertion operation has to be done
 * @NRF_WIFI_ASSERT_EQUAL_TO: The assertion check for equality.
 * @NRF_WIFI_ASSERT_NOT_EQUAL_TO: The assertion check for non-equality.
 * @NRF_WIFI_ASSERT_LESS_THAN: The assertion check for lesser value.
 * @NRF_WIFI_ASSERT_LESS_THAN_OR_EQUAL_TO: The assertion check
 *		for equal or lesser.
 * @NRF_WIFI_ASSERT_MORE_THAN: The assertion check for condition
 *		of more than value.
 * @NRF_WIFI_ASSERT_MORE_THAN_OR_EQUAL_TO: The assertion check for condition
 *		equal or more than value.
 *
 * This enum lists the possible type of operation in the assertion
 * check should be taken.
 */
enum nrf_wifi_assert_op_type {
	NRF_WIFI_ASSERT_EQUAL_TO,
	NRF_WIFI_ASSERT_NOT_EQUAL_TO,
	NRF_WIFI_ASSERT_LESS_THAN,
	NRF_WIFI_ASSERT_LESS_THAN_EQUAL_TO,
	NRF_WIFI_ASSERT_GREATER_THAN,
	NRF_WIFI_ASSERT_GREATER_THAN_EQUAL_TO,
};
#endif /* __OSAL_STRUCTS_H__ */

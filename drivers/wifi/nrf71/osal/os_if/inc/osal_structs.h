/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file osal_structs.h
 * @brief Structure declarations for the OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_STRUCTS_H__
#define __OSAL_STRUCTS_H__

#ifdef __ZEPHYR__
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#elif __KERNEL__
/* For Linux, use kernel internal headers instead of C headers*/
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
#undef strlen
#include <linux/stdarg.h>
#else
#include <stdarg.h>
#endif
#else
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#endif

/**
 * @brief The status of an operation performed by the RPU driver.
 */
enum nrf_wifi_status {
	/** The operation was successful. */
	NRF_WIFI_STATUS_SUCCESS,
	/** The operation failed. */
	NRF_WIFI_STATUS_FAIL = -1
};

/**
 * @brief DMA direction for a DMA operation.
 */
enum nrf_wifi_osal_dma_dir {
	/** Data needs to be DMAed to the device. */
	NRF_WIFI_OSAL_DMA_DIR_TO_DEV,
	/** Data needs to be DMAed from the device. */
	NRF_WIFI_OSAL_DMA_DIR_FROM_DEV,
	/** Data can be DMAed in either direction i.e to or from the device. */
	NRF_WIFI_OSAL_DMA_DIR_BIDI
};

/**
 * @brief The type of a tasklet.
 */
enum nrf_wifi_tasklet_type {
	/** The tasklet is a bottom half tasklet i.e it is scheduled from an interrupt context used
	 * for all except TX done tasklets.
	 */
	NRF_WIFI_TASKLET_TYPE_BH,
	/** The tasklet is an IRQ tasklet. It is scheduled from the Bus ISR, used internally by the
	 * SHIM layer.
	 */
	NRF_WIFI_TASKLET_TYPE_IRQ,
	/** The tasklet is a TX done tasklet. It is scheduled from the BH tasklet for TX done
	 * interrupts.
	 */
	NRF_WIFI_TASKLET_TYPE_TX_DONE,
	/** The tasklet is an RX tasklet. It is scheduled from the BH tasklet for RX interrupts. */
	NRF_WIFI_TASKLET_TYPE_RX,
	/** The maximum number of tasklet types. */
	NRF_WIFI_TASKLET_TYPE_MAX
};

/**
 * @brief Structure representing a host map.
 */
struct nrf_wifi_osal_host_map {
	/** The address of the host map. */
	unsigned long addr;
	/** The size of the host map. */
	unsigned long size;
};

/**
 * @brief Structure representing the private data of the OSAL layer.
 */
struct nrf_wifi_osal_priv {
	/** Pointer to the OSAL operations. */
	const struct nrf_wifi_osal_ops *ops;
};

/**
 * @brief The type of assertion operation to be performed.
 */
enum nrf_wifi_assert_op_type {
	/** The assertion check for equality. */
	NRF_WIFI_ASSERT_EQUAL_TO,
	/** The assertion check for non-equality. */
	NRF_WIFI_ASSERT_NOT_EQUAL_TO,
	/** The assertion check for lesser value. */
	NRF_WIFI_ASSERT_LESS_THAN,
	/** The assertion check for equal or lesser. */
	NRF_WIFI_ASSERT_LESS_THAN_EQUAL_TO,
	/** The assertion check for condition of more than value. */
	NRF_WIFI_ASSERT_GREATER_THAN,
	/** The assertion check for condition equal or more than value. */
	NRF_WIFI_ASSERT_GREATER_THAN_EQUAL_TO
};

#endif /* __OSAL_STRUCTS_H__ */

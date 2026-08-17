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
/* For Linux, use kernel internal headers instead of C headers */
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/stdarg.h>
#else
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#endif

#include <common/status.h>

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

#endif /* __OSAL_STRUCTS_H__ */

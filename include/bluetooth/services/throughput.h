/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup nrf_bt_throughput BLE GATT Throughput Service API
 * @{
 * @brief BLE GATT Throughput Service API.
 */

#ifndef __GATT_THROUGHPUT_H
#define __GATT_THROUGHPUT_H

#include <bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief BLE GATT throughput metrics. */
struct metrics {

        /** Number of GATT writes received. */
	u32_t write_count;

        /** Number of bytes received. */
	u32_t write_len;

        /** Transfer speed in bits per second. */
	u32_t write_rate;
};

/** @brief Throughput characteristic UUID. */
#define BT_UUID_THROUGHPUT_CHAR BT_UUID_DECLARE_16(0x1524)

/** @brief Throughput Service UUID. */
#define BT_UUID_THROUGHPUT                                                     \
	BT_UUID_DECLARE_128(0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D,    \
			    0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04)

/**
 * @brief Initialize the BLE GATT Throughput Service.
 */
void throughput_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __GATT_THROUGHPUT_H */

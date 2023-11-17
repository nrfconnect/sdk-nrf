/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_nrf_host_extensions Bluetooth vendor specific APIs
 * @{
 * @brief Bluetooth Host APIs for Nodic-LL vendor-specific commands.
 */

#ifndef BT_HCI_TYPES_HOST_EXTENSIONS_H_
#define BT_HCI_TYPES_HOST_EXTENSIONS_H_

#include <stdbool.h>
#include <stdint.h>

#define BT_HCI_OP_SET_REMOTE_TX_POWER	BT_OP(BT_OGF_VS, 0x010A)

struct bt_hci_set_remote_tx_power_level {
	uint16_t handle;
	uint8_t  phy;
	int8_t	 delta;
} __packed;

#define BT_HCI_OP_SET_POWER_CONTROL_REQUEST_PARAMS	BT_OP(BT_OGF_VS, 0x0110)

struct bt_hci_cp_set_power_control_request_param {
	uint8_t auto_enable;
	uint8_t apr_enable;
	uint16_t beta;
	int8_t lower_limit;
	int8_t upper_limit;
	int8_t lower_target_rssi;
	int8_t upper_target_rssi;
	uint16_t wait_period_ms;
	uint8_t apr_margin;
} __packed;

/**
 * @}
 */

#endif /* BT_HCI_TYPES_HOST_EXTENSIONS_H_ */

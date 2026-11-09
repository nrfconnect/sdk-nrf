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

#if defined(CONFIG_BT_NRF_LE_POWER_CONTROL_EXT)
#define BT_HCI_OP_SET_CONN_POWER_CONTROL_REQUEST_PARAMS	BT_OP(BT_OGF_VS, 0x0125)

struct bt_hci_cp_set_conn_power_control_request_param {
	uint16_t handle;
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

#define BT_HCI_OP_SET_CONN_TX_POWER_BOUNDS	BT_OP(BT_OGF_VS, 0x0126)

struct bt_hci_cp_set_conn_tx_power_bounds {
	uint16_t handle;
	int8_t min_tx_power;
	int8_t max_tx_power;
} __packed;

#define BT_HCI_OP_POWER_CONTROL_RSP_REPORT_ENABLE	BT_OP(BT_OGF_VS, 0x0127)

struct bt_hci_cp_power_control_rsp_report_enable {
	uint8_t enable;
} __packed;
#endif /* CONFIG_BT_NRF_LE_POWER_CONTROL_EXT */

/**
 * @}
 */

#endif /* BT_HCI_TYPES_HOST_EXTENSIONS_H_ */

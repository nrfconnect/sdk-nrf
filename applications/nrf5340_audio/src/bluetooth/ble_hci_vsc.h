/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_HCI_VSC_H_
#define _BLE_HCI_VSC_H_

#include <zephyr.h>

#define HCI_OPCODE_VS_SET_OP_FLAGS BT_OP(BT_OGF_VS, 0x3F3)
#define HCI_OPCODE_VS_SET_BD_ADDR BT_OP(BT_OGF_VS, 0x3F0)
#define HCI_OPCODE_VS_SET_ADV_TX_PWR BT_OP(BT_OGF_VS, 0x3F5)
#define HCI_OPCODE_VS_SET_CONN_TX_PWR BT_OP(BT_OGF_VS, 0x3F6)
#define HCI_OPCODE_VS_SET_LED_PIN_MAP BT_OP(BT_OGF_VS, 0x3A2)
#define HCI_OPCODE_VS_SET_RADIO_FE_CFG BT_OP(BT_OGF_VS, 0x3A3)

/* This bit setting enables the flag from controller from controller
 * if an ISO packet is lost.
 */
#define BLE_HCI_VSC_OP_ISO_LOST_NOTIFY (1 << 17)
#define BLE_HCI_VSC_OP_DIS_POWER_MONITOR (1 << 15)

struct ble_hci_vs_rp_status {
	int8_t status;
} __packed;

struct ble_hci_vs_cp_set_op_flag {
	uint32_t flag_bit;
	uint8_t setting;
} __packed;

struct ble_hci_vs_cp_set_bd_addr {
	uint8_t bd_addr[6];
} __packed;

struct ble_hci_vs_cp_set_adv_tx_pwr {
	int8_t tx_power;
} __packed;

struct ble_hci_vs_cp_set_conn_tx_pwr {
	uint16_t handle;
	int8_t tx_power;
} __packed;

struct ble_hci_vs_cp_set_led_pin_map {
	uint8_t id;
	uint8_t mode;
	uint16_t pin;
} __packed;

struct ble_hci_vs_cp_set_radio_fe_cfg {
	int8_t max_tx_power;
	uint8_t ant_id;
} __packed;

enum ble_hci_vs_tx_power {
	BLE_HCI_VSC_TX_PWR_Pos3dBm = 3,
	BLE_HCI_VSC_TX_PWR_0dBm = 0,
	BLE_HCI_VSC_TX_PWR_Neg1dBm = -1,
	BLE_HCI_VSC_TX_PWR_Neg2dBm = -2,
	BLE_HCI_VSC_TX_PWR_Neg3dBm = -3,
	BLE_HCI_VSC_TX_PWR_Neg4dBm = -4,
	BLE_HCI_VSC_TX_PWR_Neg5dBm = -5,
	BLE_HCI_VSC_TX_PWR_Neg6dBm = -6,
	BLE_HCI_VSC_TX_PWR_Neg7dBm = -7,
	BLE_HCI_VSC_TX_PWR_Neg8dBm = -8,
	BLE_HCI_VSC_TX_PWR_Neg12dBm = -12,
	BLE_HCI_VSC_TX_PWR_Neg16dBm = -16,
	BLE_HCI_VSC_TX_PWR_Neg20dBm = -20,
	BLE_HCI_VSC_TX_PWR_Neg40dBm = -40,
};

enum ble_hci_vs_led_function_id {
	PAL_LED_ID_CPU_ACTIVE = 0x10,
	PAL_LED_ID_ERROR = 0x11,
	PAL_LED_ID_BLE_TX = 0x12,
	PAL_LED_ID_BLE_RX = 0x13,
};

enum ble_hci_vs_led_function_mode {
	PAL_LED_MODE_ACTIVE_LOW = 0x00,
	PAL_LED_MODE_ACTIVE_HIGH = 0x01,
	PAL_LED_MODE_DISABLE_TOGGLE = 0xFF,
};

/**
 * @brief Set Bluetooth MAC device address
 * @param bd_addr	Bluetooth MAC device address
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_set_bd_addr(uint8_t *bd_addr);

/**
 * @brief Set controller operation mode flag
 * @param flag_bit	The target bit in operation mode flag
 * @param setting	The setting of the bit
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_set_op_flag(uint32_t flag_bit, uint8_t setting);

/**
 * @brief Set advertising TX power
 * @param tx_power TX power setting for the advertising.
 *                 Please check ble_hci_vs_tx_power for possible settings
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_set_adv_tx_pwr(enum ble_hci_vs_tx_power tx_power);

/**
 * @brief Set TX power for specific connection
 * @param conn_handle Specific connection handle for TX power setting
 * @param tx_power TX power setting for the specific connection handle
 *                 Please check ble_hci_vs_tx_power for possible settings
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_set_conn_tx_pwr(uint16_t conn_handle, enum ble_hci_vs_tx_power tx_power);

/**
 * @brief Map LED pin to a specific controller function
 *
 * @details Only support for gpio0 (pin 0-31)
 *
 * @param id Describes the LED function
 *           Please check ble_hci_vs_led_function_id for possible IDs
 * @param mode Describes how the pin is toggled
 *           Please check ble_hci_vs_led_function_mode for possible modes
 * @param pin Pin designator of the GPIO
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_map_led_pin(enum ble_hci_vs_led_function_id id,
			    enum ble_hci_vs_led_function_mode mode, uint16_t pin);

#endif /* _BLE_HCI_VSC_H_ */

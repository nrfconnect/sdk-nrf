/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_HCI_VSC_H_
#define _BLE_HCI_VSC_H_

#include <zephyr/kernel.h>

#define HCI_OPCODE_VS_SET_PRI_EXT_ADV_MAX_TX_PWR BT_OP(BT_OGF_VS, 0x000)
#define HCI_OPCODE_VS_SET_LED_PIN_MAP BT_OP(BT_OGF_VS, 0x001)
#define HCI_OPCODE_VS_CONFIG_FEM_PIN BT_OP(BT_OGF_VS, 0x002)
#define HCI_OPCODE_VS_SET_RADIO_FE_CFG BT_OP(BT_OGF_VS, 0x3A3)
#define HCI_OPCODE_VS_SET_BD_ADDR BT_OP(BT_OGF_VS, 0x3F0)
#define HCI_OPCODE_VS_SET_OP_FLAGS BT_OP(BT_OGF_VS, 0x3F3)
#define HCI_OPCODE_VS_SET_ADV_TX_PWR BT_OP(BT_OGF_VS, 0x3F5)
#define HCI_OPCODE_VS_SET_CONN_TX_PWR BT_OP(BT_OGF_VS, 0x3F6)

/* This bit setting enables the flag from controller from controller
 * if an ISO packet is lost.
 */
#define BLE_HCI_VSC_OP_ISO_LOST_NOTIFY (1 << 17)
#define BLE_HCI_VSC_OP_DIS_POWER_MONITOR (1 << 15)

struct ble_hci_vs_cp_nrf21540_pins {
	uint16_t mode;
	uint16_t txen;
	uint16_t rxen;
	uint16_t antsel;
	uint16_t pdn;
	uint16_t csn;
} __packed;

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

enum ble_hci_vs_max_tx_power {
	BLE_HCI_VSC_MAX_TX_PWR_0dBm = 0,
	BLE_HCI_VSC_MAX_TX_PWR_3dBm = 3,
	BLE_HCI_VSC_MAX_TX_PWR_10dBm = 10,
	BLE_HCI_VSC_MAX_TX_PWR_20dBm = 20
};

enum ble_hci_vs_tx_power {
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
	BLE_HCI_VSC_PRI_EXT_ADV_MAX_TX_PWR_DISABLE = 127,
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
 * @brief Set the pins to be used by the nRF21540 (Front End Module - FEM)
 *
 * @param nrf21540_pins	Pointer to a struct containing the pins
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_nrf21540_pins_set(struct ble_hci_vs_cp_nrf21540_pins *nrf21540_pins);

/**
 * @brief Enable VREGRADIO.VREQH in NET core for getting extra TX power
 *
 * @param max_tx_power	Max TX power to set
 *
 * @note        If the nRF21540 is not used, setting max_tx_power to 10 or 20 will
 *              result in +3dBm
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_radio_high_pwr_mode_set(enum ble_hci_vs_max_tx_power max_tx_power);

/**
 * @brief Set Bluetooth MAC device address
 * @param bd_addr	Bluetooth MAC device address
 *
 * @note	This can be used to set a public address for the device
 *		Note that bt_setup_public_id_addr(void) should be called
 *		after this function to properly set the address.
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_bd_addr_set(uint8_t *bd_addr);

/**
 * @brief Set controller operation mode flag
 * @param flag_bit	The target bit in operation mode flag
 * @param setting	The setting of the bit
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_op_flag_set(uint32_t flag_bit, uint8_t setting);

/**
 * @brief Set advertising TX power
 * @param tx_power TX power setting for the advertising.
 *                 Please check ble_hci_vs_tx_power for possible settings
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_adv_tx_pwr_set(enum ble_hci_vs_tx_power tx_power);

/**
 * @brief Set TX power for specific connection
 * @param conn_handle Specific connection handle for TX power setting
 * @param tx_power TX power setting for the specific connection handle
 *                 Please check ble_hci_vs_tx_power for possible settings
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_conn_tx_pwr_set(uint16_t conn_handle, enum ble_hci_vs_tx_power tx_power);

/**
 * @brief Set the maximum transmit power on primary advertising channels
 * @param tx_power TX power setting for the primary advertising channels
 *                 in advertising extension, which are BLE channel 37, 38 and 39
 *                 Please check ble_hci_vs_tx_power for possible settings
 *                 Set to BLE_HCI_VSC_PRI_EXT_ADV_MAX_TX_PWR_DISABLE (-127) for
 *                 disabling this feature
 *
 * @return 0 for success, error otherwise.
 */
int ble_hci_vsc_pri_ext_adv_max_tx_pwr_set(enum ble_hci_vs_tx_power tx_power);

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
int ble_hci_vsc_led_pin_map(enum ble_hci_vs_led_function_id id,
			    enum ble_hci_vs_led_function_mode mode, uint16_t pin);

#endif /* _BLE_HCI_VSC_H_ */

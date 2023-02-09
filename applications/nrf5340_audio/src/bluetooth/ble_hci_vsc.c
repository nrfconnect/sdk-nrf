/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include "ble_hci_vsc.h"
#include <zephyr/bluetooth/hci.h>

#include "led.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ble, CONFIG_BLE_LOG_LEVEL);

static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

static void tx_gain_led_set(uint8_t tx_gain)
{
#if (CONFIG_NRF_21540_ACTIVE)
	switch (tx_gain) {
	case 0: {
		LOG_INF("Setting green: 0");
		(void)led_on(LED_APP_RGB, LED_COLOR_GREEN);
		break;
	}
	case 10: {
		LOG_INF("Setting magenta: 10");
		(void)led_on(LED_APP_RGB, LED_COLOR_MAGENTA);
		break;
	}
	case 20: {
		LOG_INF("Setting white: 20");
		(void)led_on(LED_APP_RGB, LED_COLOR_WHITE);
		break;
	}
	default:
		LOG_INF("Setting blue: default");
		(void)led_on(LED_APP_RGB, LED_COLOR_BLUE);
		break;
	}
#endif
}

int ble_hci_vsc_nrf21540_pins_set(struct ble_hci_vs_cp_nrf21540_pins *nrf21540_pins)
{
	int ret;
	struct net_buf *buf = NULL;

	buf = bt_hci_cmd_create(HCI_OPCODE_VS_CONFIG_FEM_PIN, sizeof(*nrf21540_pins));
	if (!buf) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}

	net_buf_add_mem(buf, nrf21540_pins, sizeof(*nrf21540_pins));

	ret = bt_hci_cmd_send(HCI_OPCODE_VS_CONFIG_FEM_PIN, buf);

	return ret;
}

int ble_hci_vsc_radio_high_pwr_mode_set(enum ble_hci_vs_max_tx_power max_tx_power)
{
	int ret;
	struct ble_hci_vs_cp_set_radio_fe_cfg *cp;
	struct ble_hci_vs_rp_status *rp;
	struct net_buf *buf, *rsp = NULL;

	buf = bt_hci_cmd_create(HCI_OPCODE_VS_SET_RADIO_FE_CFG, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->max_tx_power = max_tx_power;
	cp->ant_id = 0;

	ret = bt_hci_cmd_send_sync(HCI_OPCODE_VS_SET_RADIO_FE_CFG, buf, &rsp);
	if (ret) {
		LOG_ERR("Error for HCI VS command HCI_OPCODE_VS_SET_RADIO_FE_CFG");
		return ret;
	}

	rp = (void *)rsp->data;
	ret = rp->status;
	net_buf_unref(rsp);

	return ret;
}

int ble_hci_vsc_bd_addr_set(uint8_t *bd_addr)
{
	int ret;
	struct ble_hci_vs_cp_set_bd_addr *cp;
	struct net_buf *buf = NULL;

	buf = bt_hci_cmd_create(HCI_OPCODE_VS_SET_BD_ADDR, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}
	cp = net_buf_add(buf, sizeof(*cp));
	memcpy(cp, bd_addr, sizeof(*cp));

	ret = bt_hci_cmd_send(HCI_OPCODE_VS_SET_BD_ADDR, buf);
	return ret;
}

int ble_hci_vsc_op_flag_set(uint32_t flag_bit, uint8_t setting)
{
	int ret;
	struct ble_hci_vs_cp_set_op_flag *cp;
	struct ble_hci_vs_rp_status *rp;
	struct net_buf *buf, *rsp = NULL;

	buf = bt_hci_cmd_create(HCI_OPCODE_VS_SET_OP_FLAGS, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}
	cp = net_buf_add(buf, sizeof(*cp));
	cp->flag_bit = flag_bit;
	cp->setting = setting;

	ret = bt_hci_cmd_send_sync(HCI_OPCODE_VS_SET_OP_FLAGS, buf, &rsp);
	if (ret) {
		LOG_ERR("Error for HCI VS command HCI_OPCODE_VS_SET_OP_FLAGS");
		return ret;
	}

	rp = (void *)rsp->data;
	ret = rp->status;
	net_buf_unref(rsp);

	return ret;
}

int ble_hci_vsc_adv_tx_pwr_set(enum ble_hci_vs_tx_power tx_power)
{
	int ret;
	struct ble_hci_vs_cp_set_adv_tx_pwr *cp;
	struct ble_hci_vs_rp_status *rp;
	struct net_buf *buf, *rsp = NULL;

	buf = bt_hci_cmd_create(HCI_OPCODE_VS_SET_ADV_TX_PWR, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}
	cp = net_buf_add(buf, sizeof(*cp));
	cp->tx_power = tx_power;

	ret = bt_hci_cmd_send_sync(HCI_OPCODE_VS_SET_ADV_TX_PWR, buf, &rsp);
	if (ret) {
		LOG_ERR("Error for HCI VS command HCI_OPCODE_VS_SET_ADV_TX_PWR");
		return ret;
	}

	rp = (void *)rsp->data;
	ret = rp->status;
	net_buf_unref(rsp);

	if (ret) {
		LOG_WRN("Failed to set adv tx power to %d", tx_power);
		return -EINVAL;
	}

	tx_gain_led_set(tx_power);

	return 0;
}

int ble_hci_vsc_conn_tx_pwr_set(uint16_t conn_handle, enum ble_hci_vs_tx_power tx_power)
{
	int ret;
	struct ble_hci_vs_cp_set_conn_tx_pwr *cp;
	struct ble_hci_vs_rp_status *rp;
	struct net_buf *buf, *rsp = NULL;

	buf = bt_hci_cmd_create(HCI_OPCODE_VS_SET_CONN_TX_PWR, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}
	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = conn_handle;
	cp->tx_power = tx_power;

	ret = bt_hci_cmd_send_sync(HCI_OPCODE_VS_SET_CONN_TX_PWR, buf, &rsp);
	if (ret) {
		LOG_ERR("Error for HCI VS command HCI_OPCODE_VS_SET_CONN_TX_PWR");
		return ret;
	}

	rp = (void *)rsp->data;
	ret = rp->status;
	net_buf_unref(rsp);

	return ret;
}

int ble_hci_vsc_pri_ext_adv_max_tx_pwr_set(enum ble_hci_vs_tx_power tx_power)
{
	int ret;
	struct ble_hci_vs_cp_set_adv_tx_pwr *cp;
	struct ble_hci_vs_rp_status *rp;
	struct net_buf *buf, *rsp = NULL;

	buf = bt_hci_cmd_create(HCI_OPCODE_VS_SET_PRI_EXT_ADV_MAX_TX_PWR, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}
	cp = net_buf_add(buf, sizeof(*cp));
	cp->tx_power = tx_power;

	ret = bt_hci_cmd_send_sync(HCI_OPCODE_VS_SET_PRI_EXT_ADV_MAX_TX_PWR, buf, &rsp);
	if (ret) {
		LOG_ERR("Error for HCI VS command HCI_OPCODE_VS_SET_PRI_EXT_ADV_MAX_TX_PWR");
		return ret;
	}

	rp = (void *)rsp->data;
	ret = rp->status;
	net_buf_unref(rsp);
	return ret;
}

int ble_hci_tx_gain_toggle(void)
{
	int ret;
	static uint8_t toggle = 1;

	if (!IS_ENABLED(CONFIG_NRF_21540_ACTIVE)) {
		LOG_WRN("nRF21540 is not active, will not toggle TX gain");
		return -ENODEV;
	}

	/* Toggle pin 0.31 */
	gpio_pin_set(gpio_dev, 31, toggle);

	if (toggle) {
		tx_gain_led_set(10);
	} else {
		tx_gain_led_set(20);
	}

	toggle ^= 1;

	return 0;
}

int ble_hci_vsc_led_pin_map(enum ble_hci_vs_led_function_id id,
			    enum ble_hci_vs_led_function_mode mode, uint16_t pin)
{
	int ret;
	struct ble_hci_vs_cp_set_led_pin_map *cp;
	struct ble_hci_vs_rp_status *rp;
	struct net_buf *buf, *rsp = NULL;

	buf = bt_hci_cmd_create(HCI_OPCODE_VS_SET_LED_PIN_MAP, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->id = id;

	if (mode == GPIO_ACTIVE_LOW) {
		cp->mode = PAL_LED_MODE_ACTIVE_LOW;
	} else {
		cp->mode = PAL_LED_MODE_ACTIVE_HIGH;
	}

	cp->pin = pin;

	ret = bt_hci_cmd_send_sync(HCI_OPCODE_VS_SET_LED_PIN_MAP, buf, &rsp);
	if (ret) {
		LOG_ERR("Error for HCI VS command HCI_OPCODE_VS_SET_LED_PIN_MAP");
		return ret;
	}

	rp = (void *)rsp->data;
	ret = rp->status;
	net_buf_unref(rsp);

	return ret;
}

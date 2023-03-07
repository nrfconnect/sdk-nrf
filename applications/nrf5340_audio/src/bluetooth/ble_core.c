/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "ble_core.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>

#include "macros_common.h"
#include "ble_hci_vsc.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble, CONFIG_BLE_LOG_LEVEL);

#define NET_CORE_RESPONSE_TIMEOUT_MS 500

/* Note that HCI_CMD_TIMEOUT is currently set to 10 seconds in Zephyr */
#define NET_CORE_WATCHDOG_TIME_MS 1000

static ble_core_ready_t m_ready_callback;
static struct bt_le_oob _oob = { .addr = 0 };

static void net_core_timeout_handler(struct k_timer *timer_id);
static void net_core_watchdog_handler(struct k_timer *timer_id);

static struct k_work net_core_ctrl_version_get_work;

K_TIMER_DEFINE(net_core_timeout_alarm_timer, net_core_timeout_handler, NULL);
K_TIMER_DEFINE(net_core_watchdog_timer, net_core_watchdog_handler, NULL);

/* If NET core out of response for a time defined in NET_CORE_RESPONSE_TIMEOUT
 * show error message for indicating user.
 */
static void net_core_timeout_handler(struct k_timer *timer_id)
{
	ERR_CHK_MSG(-EIO, "No response from NET core, check if NET core is programmed");
}

#if !(CONFIG_BT_PRIVACY)
/* Use unique FICR device ID to set random static address */
static void ble_core_setup_random_static_addr(void)
{
	int ret;
	static bt_addr_le_t addr;

	if ((NRF_FICR->INFO.DEVICEID[0] != UINT32_MAX) ||
	    ((NRF_FICR->INFO.DEVICEID[1] & UINT16_MAX) != UINT16_MAX)) {
		/* Put the device ID from FICR into address */
		sys_put_le32(NRF_FICR->INFO.DEVICEID[0], &addr.a.val[0]);
		sys_put_le16(NRF_FICR->INFO.DEVICEID[1], &addr.a.val[4]);

		/* The FICR value is a just a random number, with no knowledge
		 * of the Bluetooth Specification requirements for random
		 * static addresses.
		 */
		BT_ADDR_SET_STATIC(&addr.a);

		addr.type = BT_ADDR_LE_RANDOM;

		ret = bt_id_create(&addr, NULL);
		if (ret) {
			LOG_WRN("Failed to create ID");
		}
	} else {
		LOG_WRN("Unable to read from FICR");
		/* If no address can be created based on FICR,
		 * then a random address is created
		 */
	}
}
#endif /* !(CONFIG_BT_PRIVACY) */

static void mac_print(void)
{
	char dev[BT_ADDR_LE_STR_LEN];
	(void)bt_le_oob_get_local(BT_ID_DEFAULT, &_oob);
	(void)bt_addr_le_to_str(&_oob.addr, dev, BT_ADDR_LE_STR_LEN);
	LOG_INF("MAC: %s", dev);
}

static int ble_core_le_lost_notify_enable(void)
{
	return ble_hci_vsc_op_flag_set(BLE_HCI_VSC_OP_ISO_LOST_NOTIFY, 1);
}

/* Callback called by the Bluetooth stack in Zephyr when Bluetooth is ready */
static void on_bt_ready(int err)
{
	int ret;
	uint16_t ctrl_version = 0;

	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		ERR_CHK(err);
	}

	LOG_DBG("Bluetooth initialized");
	mac_print();

	ret = net_core_ctrl_version_get(&ctrl_version);
	ERR_CHK_MSG(ret, "Failed to get controller version");

	LOG_INF("Controller version: %d", ctrl_version);

	ble_core_le_lost_notify_enable();

#if (CONFIG_NRF_21540_ACTIVE)

	/* Indexes for the pins gotten from nrf21540_ek_fwd.overlay */
	uint8_t tx_pin = NRF_DT_GPIOS_TO_PSEL_BY_IDX(DT_PATH(nrf_gpio_forwarder, nrf21540_gpio_if),
						     gpios, 0);
	uint8_t rx_pin = NRF_DT_GPIOS_TO_PSEL_BY_IDX(DT_PATH(nrf_gpio_forwarder, nrf21540_gpio_if),
						     gpios, 1);
	uint8_t pdn_pin = NRF_DT_GPIOS_TO_PSEL_BY_IDX(DT_PATH(nrf_gpio_forwarder, nrf21540_gpio_if),
						      gpios, 2);
	uint8_t ant_pin = NRF_DT_GPIOS_TO_PSEL_BY_IDX(DT_PATH(nrf_gpio_forwarder, nrf21540_gpio_if),
						      gpios, 3);
	uint8_t mode_pin = NRF_DT_GPIOS_TO_PSEL_BY_IDX(
		DT_PATH(nrf_gpio_forwarder, nrf21540_gpio_if), gpios, 4);

	struct ble_hci_vs_cp_nrf21540_pins nrf21540_pins = {
		.mode = mode_pin,
		.txen = tx_pin,
		.rxen = rx_pin,
		.antsel = ant_pin,
		.pdn = pdn_pin,
		/* Set CS pin to ffff since we are not using the SPI */
		.csn = 0xffff
	};

	ret = ble_hci_vsc_nrf21540_pins_set(&nrf21540_pins);
	ERR_CHK(ret);

	ret = ble_hci_vsc_radio_high_pwr_mode_set(
		MAX(CONFIG_NRF_21540_MAIN_DBM, CONFIG_NRF_21540_PRI_ADV_DBM));
	ERR_CHK(ret);

	ret = ble_hci_vsc_adv_tx_pwr_set(CONFIG_NRF_21540_MAIN_DBM);
	ERR_CHK(ret);

	ret = ble_hci_vsc_pri_ext_adv_max_tx_pwr_set(CONFIG_NRF_21540_PRI_ADV_DBM);
	ERR_CHK(ret);
#endif /* (CONFIG_NRF_21540_ACTIVE) */

	m_ready_callback();
}

static int controller_leds_mapping(void)
{
	int ret;

	ret = ble_hci_vsc_led_pin_map(PAL_LED_ID_CPU_ACTIVE,
				      DT_GPIO_FLAGS_BY_IDX(DT_NODELABEL(rgb2_green), gpios, 0),
				      DT_GPIO_PIN_BY_IDX(DT_NODELABEL(rgb2_green), gpios, 0));
	if (ret) {
		return ret;
	}

	ret = ble_hci_vsc_led_pin_map(PAL_LED_ID_ERROR,
				      DT_GPIO_FLAGS_BY_IDX(DT_NODELABEL(rgb2_red), gpios, 0),
				      DT_GPIO_PIN_BY_IDX(DT_NODELABEL(rgb2_red), gpios, 0));
	if (ret) {
		return ret;
	}

	return 0;
}

static void net_core_watchdog_handler(struct k_timer *timer_id)
{
	k_work_submit(&net_core_ctrl_version_get_work);
}

static void work_net_core_ctrl_version_get(struct k_work *work)
{
	int ret;
	uint16_t ctrl_version = 0;

	ret = net_core_ctrl_version_get(&ctrl_version);

	ERR_CHK_MSG(ret, "Failed to get controller version");

	if (!ctrl_version) {
		ERR_CHK_MSG(-EIO, "Failed to contact net core");
	}
}

int net_core_ctrl_version_get(uint16_t *ctrl_version)
{
	int ret;
	struct net_buf *rsp;

	ret = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &rsp);
	if (ret) {
		return ret;
	}

	struct bt_hci_rp_read_local_version_info *rp = (void *)rsp->data;

	*ctrl_version = sys_le16_to_cpu(rp->hci_revision);

	net_buf_unref(rsp);

	return 0;
}

int ble_core_le_pwr_ctrl_disable(void)
{
	return ble_hci_vsc_op_flag_set(BLE_HCI_VSC_OP_DIS_POWER_MONITOR, 1);
}

int ble_core_init(ble_core_ready_t ready_callback)
{
	int ret;

	if (ready_callback == NULL) {
		return -EINVAL;
	}
	m_ready_callback = ready_callback;

	/* Setup a timer for monitoring if NET core is working or not */
	k_timer_start(&net_core_timeout_alarm_timer, K_MSEC(NET_CORE_RESPONSE_TIMEOUT_MS),
		      K_NO_WAIT);

#if !(CONFIG_BT_PRIVACY)
	ble_core_setup_random_static_addr();
#endif /* !(CONFIG_BT_PRIVACY) */

	/* Enable Bluetooth, with callback function that
	 * will be called when Bluetooth is ready
	 */
	ret = bt_enable(on_bt_ready);
	k_timer_stop(&net_core_timeout_alarm_timer);

	if (ret) {
		LOG_ERR("Bluetooth init failed (ret %d)", ret);
		return ret;
	}

	ret = controller_leds_mapping();
	if (ret) {
		LOG_ERR("Error mapping LED pins to the Bluetooth controller (ret %d)", ret);
		return ret;
	}

	k_work_init(&net_core_ctrl_version_get_work, work_net_core_ctrl_version_get);
	k_timer_start(&net_core_watchdog_timer, K_NO_WAIT, K_MSEC(NET_CORE_WATCHDOG_TIME_MS));

	return 0;
}

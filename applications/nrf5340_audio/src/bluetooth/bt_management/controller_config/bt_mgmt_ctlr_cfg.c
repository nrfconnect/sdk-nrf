/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt_ctlr_cfg_internal.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/task_wdt/task_wdt.h>

#include "macros_common.h"
#if (CONFIG_BT_LL_ACS_NRF53)
#include "ble_hci_vsc.h"
#endif /* (CONFIG_BT_LL_ACS_NRF53) */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mgmt_ctlr_cfg, CONFIG_BT_MGMT_CTLR_CFG_LOG_LEVEL);

#define COMPANY_ID_NORDIC	0x0059
#define COMPANY_ID_PACKETCRAFT	0x07E8

#define WDT_TIMEOUT_MS	      1200
#define CTLR_POLL_INTERVAL_MS (WDT_TIMEOUT_MS - 200)

static struct k_work work_ctlr_poll;
static void ctlr_poll_timer_handler(struct k_timer *timer_id);
static int wdt_ch_id;

K_TIMER_DEFINE(ctlr_poll_timer, ctlr_poll_timer_handler, NULL);

static int bt_ll_acs_nrf53_cfg(void)
{
#if (CONFIG_BT_LL_ACS_NRF53)
	int ret;
	/* Enable notification of lost ISO packets */
	ret = ble_hci_vsc_op_flag_set(BLE_HCI_VSC_OP_ISO_LOST_NOTIFY, 1);
	if (ret) {
		return ret;
	}

#if (CONFIG_NRF_21540_ACTIVE)
	/* Indexes for the pins gotten from nrf21540ek_fwd.overlay */
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
		.csn = 0xffff};

	ret = ble_hci_vsc_nrf21540_pins_set(&nrf21540_pins);
	if (ret) {
		return ret;
	}

	ret = ble_hci_vsc_radio_high_pwr_mode_set(
		MAX(CONFIG_NRF_21540_MAIN_DBM, CONFIG_NRF_21540_PRI_ADV_DBM));
	if (ret) {
		return ret;
	}

	ret = ble_hci_vsc_adv_tx_pwr_set(CONFIG_NRF_21540_MAIN_DBM);
	if (ret) {
		return ret;
	}

	LOG_DBG("TX power set to %d", CONFIG_NRF_21540_MAIN_DBM);

	ret = ble_hci_vsc_pri_adv_chan_max_tx_pwr_set(CONFIG_NRF_21540_PRI_ADV_DBM);
	if (ret) {
		return ret;
	}

	LOG_DBG("Primary advertising TX power set to %d", CONFIG_NRF_21540_PRI_ADV_DBM);
#else
	ret = ble_hci_vsc_adv_tx_pwr_set(CONFIG_BLE_ADV_TX_POWER_DBM);
	if (ret) {
		return ret;
	}

	LOG_DBG("TX power set to %d", CONFIG_BLE_ADV_TX_POWER_DBM);

	/* Disabled by default, only used if another TX power for primary adv channels is needed */
	ret = ble_hci_vsc_pri_adv_chan_max_tx_pwr_set(BLE_HCI_VSC_PRI_EXT_ADV_MAX_TX_PWR_DISABLE);
	if (ret) {
		return ret;
	}

#endif /*CONFIG_NRF_21540_ACTIVE*/

	/* Map controller LEDs*/

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
#else
	return -ENODEV;
#endif /* CONFIG_BT_LL_ACS_NRF53*/
}

static void work_ctlr_poll_handler(struct k_work *work)
{
	int ret;
	uint16_t manufacturer = 0;

	ret = bt_mgmt_ctlr_cfg_manufacturer_get(false, &manufacturer);
	ERR_CHK_MSG(ret, "Failed to contact net core");

	ret = task_wdt_feed(wdt_ch_id);
	ERR_CHK_MSG(ret, "Failed to feed watchdog");
}

static void ctlr_poll_timer_handler(struct k_timer *timer_id)
{
	k_work_submit(&work_ctlr_poll);
}

static void wdt_timeout_cb(int channel_id, void *user_data)
{
	ERR_CHK_MSG(-ETIMEDOUT, "Controller not responsive");
}

static const char *bt_spec_ver_str(uint8_t version_number)
{
	const char * const bt_ver_str[] = {
		"1.0b", "1.1", "1.2", "2.0", "2.1", "3.0", "4.0", "4.1", "4.2",
		"5.0", "5.1", "5.2", "5.3", "5.4"
	};

	if (version_number < ARRAY_SIZE(bt_ver_str)) {
		return bt_ver_str[version_number];
	}

	return "unknown";
}

int bt_mgmt_ctlr_cfg_manufacturer_get(bool print_version, uint16_t *manufacturer)
{
	int ret;
	struct net_buf *rsp;

	ret = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &rsp);
	if (ret) {
		return ret;
	}

	struct bt_hci_rp_read_local_version_info *rp = (void *)rsp->data;

	if (print_version) {
		if (rp->manufacturer == COMPANY_ID_PACKETCRAFT) {
			/* NOTE: The string below is used by the Nordic CI system */
			LOG_INF("Controller: LL_ACS_NRF53. Version: %d", rp->hci_revision);
		} else if (rp->manufacturer == COMPANY_ID_NORDIC) {
			/* NOTE: The string below is used by the Nordic CI system */
			LOG_INF("Controller: SoftDevice: Version %s (0x%02x), Revision %d",
				bt_spec_ver_str(rp->hci_version),
				rp->hci_version,
				rp->hci_revision);
		} else {
			LOG_ERR("Unsupported controller");
			return -EPERM;
		}
	}

	*manufacturer = sys_le16_to_cpu(rp->manufacturer);

	net_buf_unref(rsp);

	return 0;
}

int bt_mgmt_ctlr_cfg_init(bool watchdog_enable)
{
	int ret;
	uint16_t manufacturer = 0;

	ret = bt_mgmt_ctlr_cfg_manufacturer_get(true, &manufacturer);
	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_LL_ACS_NRF53) && (manufacturer == COMPANY_ID_PACKETCRAFT)) {
		ret = bt_ll_acs_nrf53_cfg();
		if (ret) {
			return ret;
		}
	}

	if (watchdog_enable) {
		ret = task_wdt_init(NULL);
		if (ret != 0) {
			LOG_ERR("task wdt init failure: %d\n", ret);
			return ret;
		}

		wdt_ch_id = task_wdt_add(WDT_TIMEOUT_MS, wdt_timeout_cb, NULL);
		if (wdt_ch_id < 0) {
			return wdt_ch_id;
		}

		k_work_init(&work_ctlr_poll, work_ctlr_poll_handler);
		k_timer_start(&ctlr_poll_timer, K_MSEC(CTLR_POLL_INTERVAL_MS),
			      K_MSEC(CTLR_POLL_INTERVAL_MS));
	}

	return 0;
}

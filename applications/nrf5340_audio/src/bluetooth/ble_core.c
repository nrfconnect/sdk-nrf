/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_core.h"

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
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

struct conn_tx_pwr_set_info {
	const struct shell *shell;
	enum ble_hci_vs_tx_power tx_power;
};

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

	LOG_DBG("Advertisement TX power set to %d", CONFIG_NRF_21540_MAIN_DBM);

	ret = ble_hci_vsc_pri_adv_chan_max_tx_pwr_set(CONFIG_NRF_21540_PRI_ADV_DBM);
	ERR_CHK(ret);

	LOG_DBG("Primary advertising TX power set to %d", CONFIG_NRF_21540_PRI_ADV_DBM);
#else
	ret = ble_hci_vsc_adv_tx_pwr_set(CONFIG_BLE_ADV_TX_POWER_DBM);
	ERR_CHK(ret);

	LOG_DBG("Advertisement TX power set to %d", CONFIG_BLE_ADV_TX_POWER_DBM);

	/* Disabled by default, only used if another TX power for primary adv channels is needed */
	ret = ble_hci_vsc_pri_adv_chan_max_tx_pwr_set(BLE_HCI_VSC_PRI_EXT_ADV_MAX_TX_PWR_DISABLE);
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

static void shell_help_print(const struct shell *shell)
{
	shell_print(shell, "Possible TX power values:");
	shell_print(shell, "\t0: 0 dBm");
	shell_print(shell, "\t-1: -1 dBm");
	shell_print(shell, "\t-2: -2 dBm");
	shell_print(shell, "\t-3: -3 dBm");
	shell_print(shell, "\t-4: -4 dBm");
	shell_print(shell, "\t-5: -5 dBm");
	shell_print(shell, "\t-6: -6 dBm");
	shell_print(shell, "\t-7: -7 dBm");
	shell_print(shell, "\t-8: -8 dBm");
	shell_print(shell, "\t-12: -12 dBm");
	shell_print(shell, "\t-16: -16 dBm");
	shell_print(shell, "\t-20: -20 dBm");
	shell_print(shell, "\t-40: -40 dBm");
}

static int tx_power_input_check_and_enum_return(const char *input,
						enum ble_hci_vs_tx_power *tx_power)
{
	if (strcmp(input, "0") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_0dBm;
		return 0;
	} else if (strcmp(input, "-1") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg1dBm;
		return 0;
	} else if (strcmp(input, "-2") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg2dBm;
		return 0;
	} else if (strcmp(input, "-3") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg3dBm;
		return 0;
	} else if (strcmp(input, "-4") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg4dBm;
		return 0;
	} else if (strcmp(input, "-5") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg5dBm;
		return 0;
	} else if (strcmp(input, "-6") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg6dBm;
		return 0;
	} else if (strcmp(input, "-7") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg7dBm;
		return 0;
	} else if (strcmp(input, "-8") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg8dBm;
		return 0;
	} else if (strcmp(input, "-12") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg12dBm;
		return 0;
	} else if (strcmp(input, "-16") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg16dBm;
		return 0;
	} else if (strcmp(input, "-20") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg20dBm;
		return 0;
	} else if (strcmp(input, "-40") == 0) {
		*tx_power = BLE_HCI_VSC_TX_PWR_Neg40dBm;
		return 0;
	} else {
		/* TX power value invalid */
		return -EINVAL;
	}
}

static void conn_tx_pwr_set(struct bt_conn *conn, void *data)
{
	int ret;
	struct bt_conn_info conn_info;
	uint16_t conn_handle;

	struct conn_tx_pwr_set_info info = *(struct conn_tx_pwr_set_info *)data;

	ret = bt_conn_get_info(conn, &conn_info);
	if (ret) {
		shell_error(info.shell, "Error getting the connection info for conn %p", conn);
		return;
	}

	if (conn_info.state != BT_CONN_STATE_CONNECTED) {
		return;
	}

	ret = bt_hci_get_conn_handle(conn, &conn_handle);
	if (ret) {
		shell_error(info.shell, "Error getting the connection handle for conn %p", conn);
		return;
	}

	ret = ble_hci_vsc_conn_tx_pwr_set(conn_handle, info.tx_power);
	if (ret) {
		shell_error(info.shell, "Not able to set TX power for conn handle %d", conn_handle);
		return;
	}

	shell_print(info.shell, "%d dBm TX power set for connection handle %d", info.tx_power,
		    conn_handle);
}

static int cmd_conn_tx_pwr_set(const struct shell *shell, size_t argc, const char **argv)
{
	int ret;
	enum ble_hci_vs_tx_power tx_power;
	struct conn_tx_pwr_set_info info;

	if (IS_ENABLED(CONFIG_NRF_21540_ACTIVE)) {
		shell_print(shell,
			    "Cannot use TX power shell commands when using a front end module");
		return 0;
	}

	if (argc != 2) {
		shell_error(shell, "One argument (TX power) must be provided");
		return -EINVAL;
	}

	if (strcmp(argv[1], "help") == 0) {
		shell_help_print(shell);
		return 1;
	}

	ret = tx_power_input_check_and_enum_return(argv[1], &tx_power);
	if (ret) {
		shell_error(shell, "Invalid TX power input");
		shell_help_print(shell);
		return -EINVAL;
	}

	info.shell = shell;
	info.tx_power = tx_power;

	bt_conn_foreach(BT_CONN_TYPE_LE, conn_tx_pwr_set, &info);

	shell_print(
		shell,
		"Finished setting %d dBm TX power to all active connections, there might be none",
		tx_power);

	return 0;
}

static int cmd_adv_tx_pwr_set(const struct shell *shell, size_t argc, const char **argv)
{
	int ret;
	enum ble_hci_vs_tx_power tx_power;

	if (IS_ENABLED(CONFIG_NRF_21540_ACTIVE)) {
		shell_print(shell,
			    "Cannot use TX power shell commands when using a front end module");
		return 0;
	}

	if (argc != 2) {
		shell_error(shell, "One argument (TX power) must be provided");
		return -EINVAL;
	}

	if (strcmp(argv[1], "help") == 0) {
		shell_help_print(shell);
		return 1;
	}

	ret = tx_power_input_check_and_enum_return(argv[1], &tx_power);
	if (ret) {
		shell_error(shell, "Invalid TX power input");
		shell_help_print(shell);
		return -EINVAL;
	}

	ret = ble_hci_vsc_adv_tx_pwr_set(tx_power);
	if (ret) {
		shell_error(shell, "Not able to set advertisement TX power");
	} else {
		shell_print(shell, "%d dBm TX power set for advertisement", tx_power);
	}

	return 0;
}

static int cmd_pri_adv_chan_max_tx_pwr_set(const struct shell *shell, size_t argc,
					   const char **argv)
{
	int ret;
	enum ble_hci_vs_tx_power tx_power;

	if (IS_ENABLED(CONFIG_NRF_21540_ACTIVE)) {
		shell_print(shell,
			    "Cannot use TX power shell commands when using a front end module");
		return 0;
	}

	if (argc != 2) {
		shell_error(shell, "One argument (TX power) must be provided");
		return -EINVAL;
	}

	if (strcmp(argv[1], "help") == 0) {
		shell_help_print(shell);
		return 1;
	}

	ret = tx_power_input_check_and_enum_return(argv[1], &tx_power);
	if (ret) {
		shell_error(shell, "Invalid TX power input");
		shell_help_print(shell);
		return -EINVAL;
	}

	ret = ble_hci_vsc_pri_adv_chan_max_tx_pwr_set(tx_power);
	if (ret) {
		shell_error(shell,
			    "Not able to set maximum TX power for primary advertisement channels");
	} else {
		shell_print(shell,
			    "Maximum of %d dBm TX power set for primary advertisement channels",
			    tx_power);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	ble_core_cmd,
	SHELL_COND_CMD(CONFIG_SHELL, conn_tx_pwr_set, NULL, "Set connection TX power",
		       cmd_conn_tx_pwr_set),
	SHELL_COND_CMD(CONFIG_SHELL, adv_tx_pwr_set, NULL, "Set advertisement TX power",
		       cmd_adv_tx_pwr_set),
	SHELL_COND_CMD(CONFIG_SHELL, pri_adv_chan_max_tx_pwr_set, NULL,
		       "Set maximum TX power for primary advertisement channels",
		       cmd_pri_adv_chan_max_tx_pwr_set),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ble_core, &ble_core_cmd, "BLE core commands", NULL);

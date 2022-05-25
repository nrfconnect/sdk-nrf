/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_acl_common.h"
#include "ctrl_events.h"
#include "ble_trans.h"
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include "ble_trans.h"
#include "ble_audio_services.h"
#include "ble_acl_gateway.h"
#include "ble_hci_vsc.h"

#if (CONFIG_AUDIO_DEV == GATEWAY)
#include "ble_acl_gateway.h"
#elif (CONFIG_AUDIO_DEV == HEADSET)
#include "ble_acl_headset.h"
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ble, CONFIG_LOG_BLE_LEVEL);

#define BASE_10 10

/* Connection to the peer device - the other nRF5340 Audio device
 * This is the device we are streaming audio to/from.
 */
static struct bt_conn *_p_conn_peer;

#if (CONFIG_AUDIO_DEV == HEADSET)
K_WORK_DEFINE(adv_work, work_adv_start);
#elif (CONFIG_AUDIO_DEV == GATEWAY)
K_WORK_DEFINE(scan_work, work_scan_start);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */

/**@brief BLE connected handler.
 */
static void on_connected_cb(struct bt_conn *conn, uint8_t err)
{
	int ret;
	char addr[BT_ADDR_LE_STR_LEN];
	uint16_t peer_conn_handle;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	ARG_UNUSED(ret);
	if (err) {
		LOG_ERR("ACL connection to %s failed, error %d", addr, err);
	} else {
		/* ACL connection established */
		LOG_DBG("ACL connection to %s established", addr);
		/* Setting TX power for connection if set to anything but 0 */
		if (CONFIG_BLE_CONN_TX_POWER_DBM) {
			ret = bt_hci_get_conn_handle(conn, &peer_conn_handle);
			if (ret) {
				LOG_ERR("Unable to get handle, ret = %d", ret);
			} else {
				ret = ble_hci_vsc_set_conn_tx_pwr(peer_conn_handle,
								  CONFIG_BLE_CONN_TX_POWER_DBM);
				ERR_CHK(ret);
			}
		}
#if (CONFIG_AUDIO_DEV == HEADSET)
		ble_acl_headset_on_connected(conn);
#elif (CONFIG_AUDIO_DEV == GATEWAY)
		ble_acl_gateway_on_connected(conn);
#if (CONFIG_BT_SMP)
		ret = bt_conn_set_security(conn, BT_SECURITY_L2);
		ERR_CHK_MSG(ret, "Failed to set security");
#else
		if (!ble_acl_gateway_all_links_connected()) {
			k_work_submit(&scan_work);
		} else {
			LOG_INF("All ACL links are connected");
			bt_le_scan_stop();
		}
		ret = ble_trans_iso_cis_connect(conn);
		ERR_CHK_MSG(ret, "Failed to connect to ISO CIS channel");
#endif /* (CONFIG_BT_SMP) */
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
	}
}

/**@brief BLE disconnected handler.
 */
static void on_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_DBG("ACL disconnected with %s reason: %d", addr, reason);
#if (CONFIG_AUDIO_DEV == HEADSET)
	if (_p_conn_peer == conn) {
		bt_conn_unref(_p_conn_peer);
		_p_conn_peer = NULL;
	} else {
		LOG_WRN("Unknown peer disconnected");
	}
#elif (CONFIG_AUDIO_DEV == GATEWAY)
	struct bt_conn *conn_active;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		int ret;

		ret = ble_acl_gateway_conn_peer_get(i, &conn_active);
		ERR_CHK_MSG(ret, "Connection peer get error");
		if (conn_active == conn) {
			bt_conn_unref(conn_active);
			conn_active = NULL;
			ret = ble_acl_gateway_conn_peer_set(i, &conn_active);
			ERR_CHK_MSG(ret, "Connection peer set error");
			LOG_DBG("Headset %d disconnected", i);
			break;
		}
	}
	k_work_submit(&scan_work);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
}

/**@brief BLE connection parameter request handler.
 */
static bool conn_param_req_cb(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	/* Connection between two nRF5340 Audio DKs are fixed */
	if ((param->interval_min != CONFIG_BLE_ACL_CONN_INTERVAL) ||
	    (param->interval_max != CONFIG_BLE_ACL_CONN_INTERVAL)) {
		return false;
	}

	return true;
}

/**@brief BLE connection parameters updated handler.
 */
static void conn_param_updated_cb(struct bt_conn *conn, uint16_t interval, uint16_t latency,
				  uint16_t timeout)
{
	LOG_DBG("conn: %x04, int: %d", (int)conn, interval);
	LOG_DBG("lat: %d, to: %d", latency, timeout);

	if (interval <= CONFIG_BLE_ACL_CONN_INTERVAL) {
		/* Link could be ready, since there's no service discover now */
		LOG_DBG("Connection parameters updated");
	}
}

#if (CONFIG_BT_SMP)
static void security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	int ret;

	if (err) {
		LOG_ERR("Security failed: level %u err %d", level, err);
		ret = bt_conn_disconnect(conn, err);
		if (ret) {
			LOG_ERR("Failed to disconnect %d", ret);
		}
	} else {
		LOG_DBG("Security changed: level %u", level);
	}
#if (CONFIG_AUDIO_DEV == GATEWAY)

	ret = ble_acl_gateway_mtu_exchange(conn);
	if (ret) {
		LOG_WRN("MTU exchange procedure failed = %d", ret);
	}
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */
}
#endif /* (CONFIG_BT_SMP) */

/**@brief BLE connection callback handlers.
 */
static struct bt_conn_cb conn_callbacks = {
	.connected = on_connected_cb,
	.disconnected = on_disconnected_cb,
	.le_param_req = conn_param_req_cb,
	.le_param_updated = conn_param_updated_cb,
#if (CONFIG_BT_SMP)
	.security_changed = security_changed_cb,
#endif /* (CONFIG_BT_SMP) */
};

void ble_acl_common_conn_peer_set(struct bt_conn *conn)
{
	_p_conn_peer = conn;
}

void ble_acl_common_conn_peer_get(struct bt_conn **p_conn)
{
	*p_conn = _p_conn_peer;
}

void ble_acl_common_start(void)
{
#if (CONFIG_AUDIO_DEV == HEADSET)
	k_work_submit(&adv_work);
#elif (CONFIG_AUDIO_DEV == GATEWAY)
	k_work_submit(&scan_work);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
}

int ble_acl_common_init(void)
{
	int ret;

	ARG_UNUSED(ret);
	bt_conn_cb_register(&conn_callbacks);

/* TODO: Initialize BLE services here, SMP is required*/
#if (CONFIG_BT_VCS)
	ret = ble_vcs_server_init();
	if (ret) {
		return ret;
	}
#endif /* (CONFIG_BT_VCS) */
#if (CONFIG_BT_VCS_CLIENT)
	ret = ble_vcs_client_init();
	if (ret) {
		return ret;
	}
#endif /* (CONFIG_BT_VCS_CLIENT) */

	return 0;
}

static enum ble_hci_vs_tx_power selection_to_enum_val(int selection)
{
	switch (selection) {
	case 0:
		return BLE_HCI_VSC_TX_PWR_0dBm;
	case 1:
		return BLE_HCI_VSC_TX_PWR_Neg1dBm;
	case 2:
		return BLE_HCI_VSC_TX_PWR_Neg2dBm;
	case 3:
		return BLE_HCI_VSC_TX_PWR_Neg3dBm;
	case 4:
		return BLE_HCI_VSC_TX_PWR_Neg4dBm;
	case 5:
		return BLE_HCI_VSC_TX_PWR_Neg5dBm;
	case 6:
		return BLE_HCI_VSC_TX_PWR_Neg6dBm;
	case 7:
		return BLE_HCI_VSC_TX_PWR_Neg7dBm;
	case 8:
		return BLE_HCI_VSC_TX_PWR_Neg8dBm;
	case 9:
		return BLE_HCI_VSC_TX_PWR_Neg12dBm;
	case 10:
		return BLE_HCI_VSC_TX_PWR_Neg16dBm;
	case 11:
		return BLE_HCI_VSC_TX_PWR_Neg20dBm;
	case 12:
		return BLE_HCI_VSC_TX_PWR_Neg40dBm;
	default:
		return BLE_HCI_VSC_TX_PWR_INVALID;
	}
}

static void shell_print_selection(const struct shell *shell)
{
	shell_print(shell, "Possible settings:");
	shell_print(shell, "\t0: 0dBm");
	shell_print(shell, "\t1: -1dBm");
	shell_print(shell, "\t2: -2dBm");
	shell_print(shell, "\t3: -3dBm");
	shell_print(shell, "\t4: -4dBm");
	shell_print(shell, "\t5: -5dBm");
	shell_print(shell, "\t6: -6dBm");
	shell_print(shell, "\t7: -7dBm");
	shell_print(shell, "\t8: -8dBm");
	shell_print(shell, "\t9: -12dBm");
	shell_print(shell, "\t10: -16dBm");
	shell_print(shell, "\t11: -20dBm");
	shell_print(shell, "\t12: -40dBm");
}

static int cmd_ble_tx_pwr_conn(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	struct bt_conn *conn;
	uint8_t num_conn = 0;
	uint16_t peer_conn_handle;

	if (argc != 2) {
		shell_error(shell, "Wrong number of arguments provided");
		return -EINVAL;
	}

	if (strcmp(argv[1], "help") == 0) {
		shell_print_selection(shell);
		return 0;

	} else if (!isdigit((int)argv[1][0])) {
		shell_error(shell, "Supplied argument is not numeric");
		shell_print_selection(shell);
		return -EINVAL;
	}

	enum ble_hci_vs_tx_power tx_power;

	tx_power = selection_to_enum_val(strtoul(argv[1], NULL, BASE_10));

	if (tx_power == BLE_HCI_VSC_TX_PWR_INVALID) {
		shell_error(shell, "Selection not valid");
		shell_print_selection(shell);
		return -EINVAL;
	}

#if (CONFIG_AUDIO_DEV == GATEWAY)
	for (uint8_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		ret = ble_acl_gateway_conn_peer_get(i, &conn);
		ERR_CHK_MSG(ret, "Connection peer get error");
		if (conn != NULL) {
			ret = bt_hci_get_conn_handle(conn, &peer_conn_handle);
			if (ret) {
				LOG_ERR("Unable to get handle, ret = %d", ret);
			} else {
				ret = ble_hci_vsc_set_conn_tx_pwr(peer_conn_handle, tx_power);
				ERR_CHK(ret);
				num_conn++;
			}
		}
	}
#elif (CONFIG_AUDIO_DEV == HEADSET)
	ble_acl_headset_conn_peer_get(&conn);
	if (conn != NULL) {
		ret = bt_hci_get_conn_handle(conn, &peer_conn_handle);
		if (ret) {
			LOG_ERR("Unable to get handle, ret = %d", ret);
		} else {
			ret = ble_hci_vsc_set_conn_tx_pwr(peer_conn_handle, tx_power);
			ERR_CHK(ret);
			num_conn++;
		}
	}
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */

	shell_print(shell, "TX power for set to %d dBm for %d connection(s)", tx_power, num_conn);

	return 0;
}

static int cmd_ble_tx_pwr_adv(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	if (argc != 2) {
		shell_error(shell, "Wrong number of arguments provided");
		return -EINVAL;
	}

	if (strcmp(argv[1], "help") == 0) {
		shell_print_selection(shell);
		return 0;

	} else if (!isdigit((int)argv[1][0])) {
		shell_error(shell, "Supplied argument is not numeric");
		shell_print_selection(shell);
		return -EINVAL;
	}

	enum ble_hci_vs_tx_power tx_power;

	tx_power = selection_to_enum_val(strtoul(argv[1], NULL, BASE_10));

	if (tx_power == BLE_HCI_VSC_TX_PWR_INVALID) {
		shell_error(shell, "Selection not valid");
		shell_print_selection(shell);
		return -EINVAL;
	}

	ret = ble_hci_vsc_set_adv_tx_pwr(tx_power);
	if (ret) {
		shell_error(shell, "Failed to set adv TX power");
		return ret;
	}

	shell_print(shell, "TX power for advertising set to %d dBm", tx_power);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ble_cmd,
			       SHELL_COND_CMD(CONFIG_SHELL, tx_pwr_conn, NULL,
					      "Set connection TX power", cmd_ble_tx_pwr_conn),
			       SHELL_COND_CMD(CONFIG_SHELL, tx_pwr_adv, NULL,
					      "Set advertising TX power", cmd_ble_tx_pwr_adv),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(bluetooth, &ble_cmd, "Settings related to Bluetooth", NULL);

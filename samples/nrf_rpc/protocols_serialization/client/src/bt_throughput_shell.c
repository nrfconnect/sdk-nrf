/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/throughput.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/__assert.h>

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_THROUGHPUT_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static int advertise(void);

static void connected(struct bt_conn *conn, uint8_t hci_err)
{
	int rc;

	if (hci_err) {
		return;
	}

	rc = bt_conn_set_security(conn, BT_SECURITY_L2);
	__ASSERT(rc == 0, "Failed to set BLE security level: %d", rc);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	advertise();
}

static struct bt_conn_cb conn_cb = {
	.connected = connected,
	.disconnected = disconnected,
};

static int advertise(void)
{
	int rc;

	(void)bt_conn_cb_register(&conn_cb);
	rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (rc) {
		(void)bt_conn_cb_unregister(&conn_cb);
	}

	return rc;
}

static int cmd_advertise(const struct shell *sh, size_t argc, char *argv[])
{
	bool start;
	int rc;

	if (!strcmp(argv[1], "on")) {
		start = true;
	} else if (!strcmp(argv[1], "off")) {
		start = false;
	} else {
		shell_error(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	if (start) {
		rc = advertise();
	} else {
		rc = bt_le_adv_stop();
		(void)bt_conn_cb_unregister(&conn_cb);
	}

	if (rc < 0) {
		shell_error(sh, "Failed to %s advertising: %d", start ? "start" : "stop", rc);
		return -ENOEXEC;
	}

	shell_print(sh, "Advertising %s", start ? "started" : "stopped");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bt_throughput_cmds,
			       SHELL_CMD_ARG(advertise, NULL, "on|off", cmd_advertise, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(bt_throughput, &bt_throughput_cmds, "BLE throughput commands", NULL, 2, 0);

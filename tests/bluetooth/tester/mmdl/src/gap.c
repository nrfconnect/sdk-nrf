/* gap.c - Bluetooth GAP Tester */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>
#include <string.h>

#include <zephyr/toolchain.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_gap
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "bttester.h"

#define CONTROLLER_INDEX 0
#define CONTROLLER_NAME "btp_tester"

#define BT_LE_AD_DISCOV_MASK (BT_LE_AD_LIMITED | BT_LE_AD_GENERAL)
#define ADV_BUF_LEN (sizeof(struct gap_device_found_ev) + 2 * 31)

static atomic_t current_settings;
struct bt_conn_auth_cb cb;

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
static struct bt_le_oob oob_sc_local = { 0 };
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */


static void controller_info(uint8_t *data, uint16_t len)
{
	struct gap_read_controller_info_rp rp;
	uint32_t supported_settings;

	(void)memset(&rp, 0, sizeof(rp));

	struct bt_le_oob oob_local = { 0 };

	bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
	memcpy(rp.address, &oob_local.addr.a, sizeof(bt_addr_t));

	/*
	 * Re-use the oob data read here in get_oob_sc_local_data()
	 */
#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
	oob_sc_local = oob_local;
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */

	/*
	 * If privacy is used, the device uses random type address, otherwise
	 * static random or public type address is used.
	 */
#if !defined(CONFIG_BT_PRIVACY)
	if (oob_local.addr.type == BT_ADDR_LE_RANDOM) {
		atomic_set_bit(&current_settings, GAP_SETTINGS_STATIC_ADDRESS);
	}
#endif /* CONFIG_BT_PRIVACY */

	supported_settings = BIT(GAP_SETTINGS_POWERED);
	supported_settings |= BIT(GAP_SETTINGS_CONNECTABLE);
	supported_settings |= BIT(GAP_SETTINGS_BONDABLE);
	supported_settings |= BIT(GAP_SETTINGS_LE);
	supported_settings |= BIT(GAP_SETTINGS_ADVERTISING);

	rp.supported_settings = sys_cpu_to_le32(supported_settings);
	rp.current_settings = sys_cpu_to_le32(current_settings);

	memcpy(rp.name, CONTROLLER_NAME, sizeof(CONTROLLER_NAME));

	tester_send(BTP_SERVICE_ID_GAP, GAP_READ_CONTROLLER_INFO,
		    CONTROLLER_INDEX, (uint8_t *) &rp, sizeof(rp));
}

void tester_handle_gap(uint8_t opcode, uint8_t index, uint8_t *data,
		       uint16_t len)
{
	LOG_DBG("opcode: 0x%02x", opcode);
	switch (opcode) {
	case GAP_READ_CONTROLLER_INFO:
		controller_info(data, len);
		return;
	default:
		LOG_WRN("Unknown opcode: 0x%x", opcode);
		tester_rsp(BTP_SERVICE_ID_GAP, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

static void tester_init_gap_cb(int err)
{
	if (err) {
		tester_rsp(BTP_SERVICE_ID_CORE, CORE_REGISTER_SERVICE,
			   BTP_INDEX_NONE, BTP_STATUS_FAILED);
		LOG_WRN("Error: %d", err);
		return;
	}

	tester_rsp(BTP_SERVICE_ID_CORE, CORE_REGISTER_SERVICE, BTP_INDEX_NONE,
		   BTP_STATUS_SUCCESS);
}

uint8_t tester_init_gap(void)
{
	int err;

	err = bt_enable(tester_init_gap_cb);
	if (err < 0) {
		LOG_ERR("Unable to enable Bluetooth: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_gap(void)
{
	return BTP_STATUS_SUCCESS;
}

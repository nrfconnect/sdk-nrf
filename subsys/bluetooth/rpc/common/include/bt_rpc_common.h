/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef BT_RPC_COMMON_H_
#define BT_RPC_COMMON_H_

#include "bluetooth/bluetooth.h"
#include "bluetooth/conn.h"
#include "bluetooth/gatt.h"

#include "nrf_rpc_cbor.h"

#define _BT_RPC_TYPE_COMARE_CONCAT2(a, b, c) a ## _ ## b ## _ ## c
#define _BT_RPC_TYPE_COMARE_CONCAT(a, b, c) _BT_RPC_TYPE_COMARE_CONCAT2(a, b, c)
#define _BT_RPC_TYPE_COMARE_UNIQUE() _BT_RPC_TYPE_COMARE_CONCAT(_bt_rpc_rsv_unique, __COUNTER__, __LINE__)
#define BT_RPC_TYPE_COMARE(type1, type2) typedef char _BT_RPC_TYPE_COMARE_UNIQUE()[sizeof( *(type1**)0 = *(type2**)0 )];
#define BT_RPC_FIELD_TYPE_COMARE(str, field, type) typedef char _BT_RPC_TYPE_COMARE_UNIQUE()[sizeof( *(type**)0 = &((str*)0)->field )];


/* All commands and events IDs used in bluetooth API serialization.
 */

enum bt_rpc_cmd_from_cli_to_host
{
	/* bluetooth.h API */
  	BT_RPC_GET_CHECK_TABLE_RPC_CMD,
  	BT_ENABLE_RPC_CMD,
  	BT_LE_ADV_START_RPC_CMD,
  	BT_LE_ADV_STOP_RPC_CMD,
  	BT_LE_SCAN_START_RPC_CMD,
  	BT_SET_NAME_RPC_CMD,
  	BT_GET_NAME_OUT_RPC_CMD,
  	BT_SET_ID_ADDR_RPC_CMD,
  	BT_ID_GET_RPC_CMD,
  	BT_ID_CREATE_RPC_CMD,
  	BT_ID_RESET_RPC_CMD,
  	BT_ID_DELETE_RPC_CMD,
  	BT_LE_ADV_UPDATE_DATA_RPC_CMD,
  	BT_LE_EXT_ADV_CREATE_RPC_CMD,
  	BT_LE_EXT_ADV_DELETE_RPC_CMD,
  	BT_LE_EXT_ADV_START_RPC_CMD,
  	BT_LE_EXT_ADV_STOP_RPC_CMD,
  	BT_LE_EXT_ADV_SET_DATA_RPC_CMD,
  	BT_LE_EXT_ADV_UPDATE_PARAM_RPC_CMD,
  	BT_LE_EXT_ADV_GET_INDEX_RPC_CMD,
  	BT_LE_EXT_ADV_GET_INFO_RPC_CMD,
  	BT_LE_SCAN_STOP_RPC_CMD,
  	BT_LE_SCAN_CB_REGISTER_ON_REMOTE_RPC_CMD,
  	BT_LE_WHITELIST_ADD_RPC_CMD,
  	BT_LE_WHITELIST_REM_RPC_CMD,
  	BT_LE_WHITELIST_CLEAR_RPC_CMD,
  	BT_LE_SET_CHAN_MAP_RPC_CMD,
  	BT_LE_OOB_GET_LOCAL_RPC_CMD,
  	BT_LE_EXT_ADV_OOB_GET_LOCAL_RPC_CMD,
  	BT_UNPAIR_RPC_CMD,
  	BT_FOREACH_BOND_RPC_CMD,
	/* conn.h API */
	BT_CONN_REMOTE_UPDATE_REF_RPC_CMD,
	BT_CONN_GET_INFO_RPC_CMD,
	BT_CONN_GET_REMOTE_INFO_RPC_CMD,
	BT_CONN_LE_PARAM_UPDATE_RPC_CMD,
	BT_CONN_LE_DATA_LEN_UPDATE_RPC_CMD,
	BT_CONN_LE_PHY_UPDATE_RPC_CMD,
	BT_CONN_DISCONNECT_RPC_CMD,
	BT_CONN_LE_CREATE_RPC_CMD,
	BT_CONN_LE_CREATE_AUTO_RPC_CMD,
	BT_CONN_CREATE_AUTO_STOP_RPC_CMD,
	BT_LE_SET_AUTO_CONN_RPC_CMD,
	BT_CONN_SET_SECURITY_RPC_CMD,
	BT_CONN_GET_SECURITY_RPC_CMD,
	BT_CONN_ENC_KEY_SIZE_RPC_CMD,
	BT_CONN_CB_REGISTER_ON_REMOTE_RPC_CMD,
	BT_SET_BONDABLE_RPC_CMD,
	BT_SET_OOB_DATA_FLAG_RPC_CMD,
	BT_LE_OOB_SET_LEGACY_TK_RPC_CMD,
	BT_LE_OOB_SET_SC_DATA_RPC_CMD,
	BT_LE_OOB_GET_SC_DATA_RPC_CMD,
	BT_PASSKEY_SET_RPC_CMD,
	BT_CONN_AUTH_CB_REGISTER_ON_REMOTE_RPC_CMD,
	BT_CONN_AUTH_PASSKEY_ENTRY_RPC_CMD,
	BT_CONN_AUTH_CANCEL_RPC_CMD,
	BT_CONN_AUTH_PASSKEY_CONFIRM_RPC_CMD,
	BT_CONN_AUTH_PAIRING_CONFIRM_RPC_CMD,
	BT_CONN_FOREACH_RPC_CMD,
	BT_CONN_LOOKUP_ADDR_LE_RPC_CMD,
	BT_CONN_GET_DST_OUT_RPC_CMD,
	/* gatt.h API */
	BT_GATT_NOTIFY_CB_RPC_CMD,
	BT_RPC_GATT_START_SERVICE_RPC_CMD,
	BT_RPC_GATT_SEND_SIMPLE_ATTR_RPC_CMD,
	BT_RPC_GATT_SEND_DESC_ATTR_RPC_CMD,
};

enum bt_rpc_cmd_from_host_to_cli
{
	/* bluetooth.h API */
	BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD,
	BT_LE_EXT_ADV_CB_SENT_CALLBACK_RPC_CMD,
	BT_LE_EXT_ADV_CB_SCANNED_CALLBACK_RPC_CMD,
	BT_LE_EXT_ADV_CB_CONNECTED_CALLBACK_RPC_CMD,
	BT_LE_SCAN_CB_RECV_RPC_CMD,
	BT_LE_SCAN_CB_TIMEOUT_RPC_CMD,
	BT_FOREACH_BOND_CB_CALLBACK_RPC_CMD,
	/* conn.h API */
	BT_CONN_CB_CONNECTED_CALL_RPC_CMD,
	BT_CONN_CB_DISCONNECTED_CALL_RPC_CMD,
	BT_CONN_CB_LE_PARAM_REQ_CALL_RPC_CMD,
	BT_CONN_CB_LE_PARAM_UPDATED_CALL_RPC_CMD,
	BT_CONN_CB_LE_PHY_UPDATED_CALL_RPC_CMD,
	BT_CONN_CB_LE_DATA_LEN_UPDATED_CALL_RPC_CMD,
	BT_CONN_CB_IDENTITY_RESOLVED_CALL_RPC_CMD,
	BT_CONN_CB_SECURITY_CHANGED_CALL_RPC_CMD,
	BT_CONN_CB_REMOTE_INFO_AVAILABLE_CALL_RPC_CMD,
	BT_RPC_AUTH_CB_PAIRING_ACCEPT_RPC_CMD,
	BT_RPC_AUTH_CB_PASSKEY_DISPLAY_RPC_CMD,
	BT_RPC_AUTH_CB_PASSKEY_ENTRY_RPC_CMD,
	BT_RPC_AUTH_CB_PASSKEY_CONFIRM_RPC_CMD,
	BT_RPC_AUTH_CB_OOB_DATA_REQUEST_RPC_CMD,
	BT_RPC_AUTH_CB_CANCEL_RPC_CMD,
	BT_RPC_AUTH_CB_PAIRING_CONFIRM_RPC_CMD,
	BT_RPC_AUTH_CB_PINCODE_ENTRY_RPC_CMD,
	BT_RPC_AUTH_CB_PAIRING_COMPLETE_RPC_CMD,
	BT_RPC_AUTH_CB_PAIRING_FAILED_RPC_CMD,
	BT_CONN_FOREACH_CB_CALLBACK_RPC_CMD,
};

enum {
	BT_READY_CB_T_CALLBACK_RPC_EVT,
};

enum {
	FLAG_PAIRING_ACCEPT_PRESENT = 0x001,
	FLAG_PASSKEY_DISPLAY_PRESENT = 0x002,
	FLAG_PASSKEY_ENTRY_PRESENT = 0x004,
	FLAG_PASSKEY_CONFIRM_PRESENT = 0x008,
	FLAG_OOB_DATA_REQUEST_PRESENT = 0x010,
	FLAG_CANCEL_PRESENT = 0x020,
	FLAG_PAIRING_CONFIRM_PRESENT = 0x040,
	FLAG_PAIRING_COMPLETE_PRESENT = 0x80,
	FLAG_PAIRING_FAILED_PRESENT = 0x100,
};

typedef void (*bt_conn_foreach_cb)(struct bt_conn *conn, void *data);
typedef void (*bt_foreach_bond_cb)(const struct bt_bond_info *info,
				   void *user_data);
typedef void (*bt_le_ext_adv_cb_sent)(struct bt_le_ext_adv *adv,
		     struct bt_le_ext_adv_sent_info *info);
typedef void (*bt_le_ext_adv_cb_connected)(struct bt_le_ext_adv *adv,
			  struct bt_le_ext_adv_connected_info *info);
typedef void (*bt_le_ext_adv_cb_scanned)(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_scanned_info *info);


NRF_RPC_GROUP_DECLARE(bt_rpc_grp);

#if defined(CONFIG_BT_RPC_HOST)
void bt_rpc_get_check_table(uint8_t *data, size_t size);
#else
bool bt_rpc_validate_check_table(uint8_t* data, size_t size);
size_t bt_rpc_calc_check_table_size();
#endif

#define BT_RPC_POOL_DEFINE(name, size) \
	BUILD_ASSERT(size > 0, "BT_RPC_POOL empty"); \
	BUILD_ASSERT(size <= 32, "BT_RPC_POOL too big"); \
	static atomic_t name = ~(((uint32_t)1 << (8 * sizeof(uint32_t) - (size))) - (uint32_t)1)

int bt_rpc_pool_reserve(atomic_t *pool_mask);
void bt_rpc_pool_release(atomic_t *pool_mask, int number);

void encode_bt_conn(CborEncoder *encoder, const struct bt_conn *conn);
struct bt_conn *decode_bt_conn(CborValue *value);

#endif /* BT_RPC_COMMON_H_ */

#ifndef BT_RPC_COMMON_H_
#define BT_RPC_COMMON_H_

#include "nrf_rpc_cbor.h"

/* All commands and events IDs used in bluetooth API serialization.
 */

enum {
	BT_ENABLE_RPC_CMD,
	BT_LE_ADV_START_RPC_CMD,
	BT_LE_ADV_STOP_RPC_CMD,
	BT_LE_SCAN_START_RPC_CMD,
	BT_LE_SCAN_CB_T_CALLBACK_RPC_CMD,

	BT_CONN_REMOTE_UPDATE_REF_RPC_CMD,
	BT_CONN_GET_INFO_RPC_CMD,
	BT_CONN_GET_REMOTE_INFO_RPC_CMD,
	BT_CONN_LE_PARAM_UPDATE_RPC_CMD,
	BT_CONN_LE_DATA_LEN_UPDATE_RPC_CMD,
	BT_CONN_LE_PHY_UPDATE_RPC_CMD,
	BT_CONN_DISCONNECT_RPC_CMD,
	BT_CONN_LE_CREATE_RPC_CMD,
	BT_CONN_LE_CREATE_AUTO_RPC_CMD,

	BT_RPC_GET_CHECK_TABLE_RPC_CMD,
};

enum {
	BT_READY_CB_T_CALLBACK_RPC_EVT,
};


NRF_RPC_GROUP_DECLARE(bt_rpc_grp);

#if defined(CONFIG_BT_RPC_HOST)
void bt_rpc_get_check_table(uint8_t *data, size_t size);
#else
bool bt_rpc_validate_check_table(uint8_t* data, size_t size);
size_t bt_rpc_calc_check_table_size();
#endif

#endif /* BT_RPC_COMMON_H_ */

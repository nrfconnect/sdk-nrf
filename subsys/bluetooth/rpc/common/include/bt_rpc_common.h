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
};

enum {
	BT_READY_CB_T_CALLBACK_RPC_EVT,
};


NRF_RPC_GROUP_DECLARE(bt_rpc_grp);

#endif /* BT_RPC_COMMON_H_ */

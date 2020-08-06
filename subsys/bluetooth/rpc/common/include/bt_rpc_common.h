#ifndef BT_RPC_COMMON_H_
#define BT_RPC_COMMON_H_

#include "nrf_rpc_cbor.h"

/* All commands and events IDs used in bluetooth API serialization.
 */

enum {
	BT_ENABLE_RPC_CMD,
};

enum {
	BT_READY_CB_T_ENCODER_RPC_EVT,
};


NRF_RPC_GROUP_DECLARE(bt_rpc_grp);

#endif /* BT_RPC_COMMON_H_ */

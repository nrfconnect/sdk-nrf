/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_TYPES_H_
#define OT_RPC_TYPES_H_

#include <stdint.h>
#include <openthread/thread.h>

/** @brief Offsets used to encode otNetifAddress bit-fields.
 */
enum ot_rpc_netif_address_offsets {
	OT_RPC_NETIF_ADDRESS_PREFERRED_OFFSET = 0,
	OT_RPC_NETIF_ADDRESS_VALID_OFFSET = 1,
	OT_RPC_NETIF_ADDRESS_SCOPE_VALID_OFFSET = 2,
	OT_RPC_NETIF_ADDRESS_SCOPE_OFFSET = 3,
	OT_RPC_NETIF_ADDRESS_SCOPE_MASK = 7 << OT_RPC_NETIF_ADDRESS_SCOPE_OFFSET,
	OT_RPC_NETIF_ADDRESS_RLOC_OFFSET = 7,
	OT_RPC_NETIF_ADDRESS_MESH_LOCAL_OFFSET = 8,
};

/** @brief Offsets used to encode otLinkModeConfig bit-fields.
 */
enum ot_rpc_link_mode_offsets {
	OT_RPC_LINK_MODE_RX_ON_WHEN_IDLE_OFFSET = 0,
	OT_RPC_LINK_MODE_DEVICE_TYPE_OFFSET = 1,
	OT_RPC_LINK_MODE_NETWORK_DATA_OFFSET = 2,
};

/** @brief Key type of internal otMesage registry.
 */
typedef uint32_t ot_msg_key;

/** @brief Key type of internal otUdpSocket registry.
 */
typedef uint32_t ot_socket_key;

/** @brief Key type for internal CoAP request registry.
 */
typedef uint32_t ot_rpc_coap_request_key;

#endif /* OT_RPC_TYPES_H_ */

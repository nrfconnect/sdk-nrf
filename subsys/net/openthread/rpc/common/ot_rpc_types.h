/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

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

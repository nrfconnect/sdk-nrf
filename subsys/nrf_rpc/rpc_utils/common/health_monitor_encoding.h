/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HEALTH_MONITOR_ENCODING_H_
#define HEALTH_MONITOR_ENCODING_H_

#ifdef CONFIG_NRF_RPC_UTILS_SERVER
#define XCODE_UINT(FIELD, CTX) nrf_rpc_encode_uint(CTX, FIELD)
#else
#define XCODE_UINT(FIELD, CTX) FIELD = nrf_rpc_decode_uint(CTX)
#endif

#define XCODE_COUNTERS(OBJ, CTX)				\
	XCODE_UINT(OBJ.child_added, CTX);			\
	XCODE_UINT(OBJ.child_removed, CTX);			\
	XCODE_UINT(OBJ.partition_id_changes, CTX);		\
	XCODE_UINT(OBJ.key_sequence_changes, CTX);		\
	XCODE_UINT(OBJ.network_data_changes, CTX);		\
	XCODE_UINT(OBJ.active_dataset_changes, CTX);		\
	XCODE_UINT(OBJ.pending_dataset_changes, CTX);		\
								\
	XCODE_UINT(OBJ.role_disabled, CTX);			\
	XCODE_UINT(OBJ.role_detached, CTX);			\
	XCODE_UINT(OBJ.role_child, CTX);			\
	XCODE_UINT(OBJ.role_router, CTX);			\
	XCODE_UINT(OBJ.role_leader, CTX);			\
	XCODE_UINT(OBJ.attach_attempts, CTX);			\
	XCODE_UINT(OBJ.better_partition_attach_attempts, CTX);	\
								\
	XCODE_UINT(OBJ.ip_tx_success, CTX);			\
	XCODE_UINT(OBJ.ip_rx_success, CTX);			\
	XCODE_UINT(OBJ.ip_tx_failure, CTX);			\
	XCODE_UINT(OBJ.ip_rx_failure, CTX);			\
								\
	XCODE_UINT(OBJ.mac_tx_unicast, CTX);			\
	XCODE_UINT(OBJ.mac_tx_broadcast, CTX);			\
	XCODE_UINT(OBJ.mac_tx_retry, CTX);			\
	XCODE_UINT(OBJ.mac_rx_unicast, CTX);			\
	XCODE_UINT(OBJ.mac_rx_broadcast, CTX);			\
								\
	XCODE_UINT(OBJ.child_supervision_failure, CTX);		\
	XCODE_UINT(OBJ.child_max, CTX);				\
	XCODE_UINT(OBJ.router_max, CTX);			\
	XCODE_UINT(OBJ.router_added, CTX);			\
	XCODE_UINT(OBJ.router_removed, CTX);			\
	XCODE_UINT(OBJ.srp_server_changes, CTX)

#endif /* HEALTH_MONITOR_ENCODING_H_ */

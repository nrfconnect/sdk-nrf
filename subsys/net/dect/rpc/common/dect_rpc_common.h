/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_RPC_COMMON_H_
#define DECT_RPC_COMMON_H_

#include <stdbool.h>
#include <stddef.h>

#include <nrf_rpc_cbor.h>
#include <net/dect/dect_net_l2.h>

/** Shell-over-DECT NR+ RPC: match L2 DECT shell capacity .
 *  L2 "sett" subcommand uses 1..19 args; others use up to 8 (such as scan, tx, rx).
 */
#define DECT_RPC_SHELL_MAX_ARGC      20   /* subcmd + 19 args (sett) */
#define DECT_RPC_SHELL_ARG_LEN       96   /* max length per DECT NR+ argv string */
#define DECT_RPC_SHELL_OUTPUT_LEN    1024 /* max captured output from DECT NR+ server */

/** Max child associations in DECT_RPC_CMD_IF_STATUS (client and server must match). */
#define DECT_RPC_MAX_STATUS_CHILDREN 16

/** Max tunneled IPv6 payload (IF_SEND / IF_RECEIVE); both roles must use the same Kconfig. */
#define DECT_RPC_MAX_IP_PKT CONFIG_DECT_NR_RPC_MAX_IP_PKT

NRF_RPC_GROUP_DECLARE(dect_rpc_group);

void dect_rpc_decode_skip_extra_ipv6_addrs(struct nrf_rpc_cbor_ctx *ctx, uint32_t claimed,
					   uint32_t applied);
void dect_rpc_decode_skip_extra_ipv6_prefixes(struct nrf_rpc_cbor_ctx *ctx, uint32_t claimed,
					      uint32_t applied);
void dect_rpc_decode_skip_status_association(struct nrf_rpc_cbor_ctx *ctx);

void dect_rpc_decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			  void *handler_data);
void dect_rpc_decode_if_send_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data);
void dect_rpc_report_cmd_decoding_error(uint8_t cmd_evt_id);

/** Single signed 32-bit CBOR int (CONNECT/DISCONNECT/IF_SEND command responses). */
void dect_rpc_int32_rsp_encode(struct nrf_rpc_cbor_ctx *ctx, int32_t status);
bool dect_rpc_int32_rsp_decode(struct nrf_rpc_cbor_ctx *ctx, int32_t *status);

/** IF_STATUS request: optional monotonic sequence (client only; server may skip). */
void dect_rpc_status_req_encode(struct nrf_rpc_cbor_ctx *ctx, uint32_t seq);

/** Zeroed status for server error / no-interface responses (br_net_iface left NULL). */
void dect_rpc_status_init_empty(struct dect_status_info *status);

/** CBOR size hint for dect_rpc_status_rsp_encode(). */
size_t dect_rpc_status_rsp_encode_size(const struct dect_status_info *status);

/** Encode DECT_RPC_CMD_IF_STATUS response from struct dect_status_info. */
void dect_rpc_status_rsp_encode(struct nrf_rpc_cbor_ctx *ctx,
				const struct dect_status_info *status);

/** Decode DECT_RPC_CMD_IF_STATUS response; sets br_net_iface to NULL. Returns false on error. */
bool dect_rpc_status_rsp_decode(struct nrf_rpc_cbor_ctx *ctx, struct dect_status_info *status);

#endif /* DECT_RPC_COMMON_H_ */

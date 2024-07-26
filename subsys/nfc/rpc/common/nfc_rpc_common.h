/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_RPC_COMMON_H_
#define NFC_RPC_COMMON_H_

#include <nrf_rpc_cbor.h>

struct nfc_rpc_param_getter {
	void *data;
	size_t *length;
};

NRF_RPC_GROUP_DECLARE(nfc_group);

void nfc_rpc_decode_error(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			  void *handler_data);
void nfc_rpc_decode_parameter(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data);
/* The function reports about command decoding error (not responses). */
void nfc_rpc_report_decoding_error(uint8_t cmd_evt_id);

#endif /* NFC_RPC_COMMON_H_ */

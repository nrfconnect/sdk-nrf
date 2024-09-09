/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <nrf_rpc_cbor.h>

/** @brief Encode an unsigned integer value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 */
void ser_encode_uint(struct nrf_rpc_cbor_ctx *ctx, uint32_t value);

/** @brief Encode an integer value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 */
void ser_encode_int(struct nrf_rpc_cbor_ctx *ctx, int32_t value);

/** @brief Encode a buffer.
 *
 * @param[in,out] ctx CBOR encoding context.
 * @param[in] data Buffer to encode.
 * @param[in] size Buffer size.
 *
 * @param[in,out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_buffer(struct nrf_rpc_cbor_ctx *ctx, const void *data, size_t size);

/** @brief Decode an unsigned integer value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval Decoded unsigned integer value.
 */
uint32_t ser_decode_uint(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a integer value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval Decoded integer value.
 */
int32_t ser_decode_int(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a buffer.
 *
 * @param[in,out] ctx CBOR decoding context.
 * @param[out] buffer Buffer for a decoded buffer data.
 * @param[in]  size Buffer size.
 *
 * @retval Pointer to a decoded buffer.
 */
void *ser_decode_buffer(struct nrf_rpc_cbor_ctx *ctx, void *buffer, size_t buffer_size);

/** @brief Indicates that decoding is done. Use this function when you finish decoding of the
 *         received serialized packet.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval True if decoding completed successfully.
 *         Otherwise, false will be returned.
 */
bool ser_decoding_done_and_check(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a command response as an integer value.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to the location where the value is decoded.
 */
void ser_rsp_decode_i32(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			void *handler_data);

/** @brief Send response to a command as an integer value.
 *
 * @param[in] group nRF RPC group.
 * @param[in] response Integer value to send.
 */
void ser_rsp_send_int(const struct nrf_rpc_group *group, int32_t response);

#endif /* SERIALIZE_H_ */

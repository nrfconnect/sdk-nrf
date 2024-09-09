/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup nrf_rpc_serialize NRF RPC serialize API
 * @{
 * @brief API for the NRF RPC serialization.
 */

#ifndef NRF_RPC_SERIALIZE_H_
#define NRF_RPC_SERIALIZE_H_

#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <nrf_rpc_cbor.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get a scratchpad item size aligned to 4-byte boundary.
 *
 * @param[in] size scratchpad item size
 *
 * @retval The scratchpad item size rounded up to the next multiple of 4.
 */
#define NRF_RPC_SCRATCHPAD_ALIGN(size) WB_UP(size)

/** @brief Alloc the scratchpad. Scratchpad is used to store a data when decoding serialized data.
 *
 *  @param[in] _scratchpad Scratchpad name.
 *  @param[in] _ctx CBOR decoding context. One unsigned integer will be decoded
 *                    from this value that contains scratchpad buffer size.
 */
#define NRF_RPC_SCRATCHPAD_DECLARE(_scratchpad, _ctx)                                              \
	(_scratchpad)->ctx = _ctx;                                                                 \
	uint32_t _scratchpad_size = nrf_rpc_decode_uint(_ctx);                                     \
	uint32_t _scratchpad_data[NRF_RPC_SCRATCHPAD_ALIGN(_scratchpad_size) / sizeof(uint32_t)];  \
	net_buf_simple_init_with_data(&(_scratchpad)->buf, _scratchpad_data, _scratchpad_size);    \
	net_buf_simple_reset(&(_scratchpad)->buf)

/** @brief Scratchpad structure. */
struct nrf_rpc_scratchpad {
	/* CBOR decoding context */
	struct nrf_rpc_cbor_ctx *ctx;

	/** Data buffer. */
	struct net_buf_simple buf;
};

/** @brief Get the scratchpad item of a given size.
 *         The scratchpad item size will be round up to multiple of 4.
 *
 * @param[in] scratchpad Scratchpad.
 * @param[in] size Scratchpad item size.
 *
 * @retval Pointer to the scratchpad item data.
 */
static inline void *nrf_rpc_scratchpad_add(struct nrf_rpc_scratchpad *scratchpad, size_t size)
{
	return net_buf_simple_add(&scratchpad->buf, NRF_RPC_SCRATCHPAD_ALIGN(size));
}

/** @brief Encode a null value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 */
void nrf_rpc_encode_null(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Encode an undefined value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 */
void nrf_rpc_encode_undefined(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Encode a boolean value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 * @param[in]     value Encoded value.
 */
void nrf_rpc_encode_bool(struct nrf_rpc_cbor_ctx *ctx, bool value);

/** @brief Encode an unsigned integer value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 * @param[in]     value Encoded value.
 */
void nrf_rpc_encode_uint(struct nrf_rpc_cbor_ctx *ctx, uint32_t value);

/** @brief Encode an integer value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 * @param[in]     value Encoded value.
 */
void nrf_rpc_encode_int(struct nrf_rpc_cbor_ctx *ctx, int32_t value);

/** @brief Encode an 64bits unsigned integer value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 * @param[in]     value Encoded value.
 */
void nrf_rpc_encode_uint64(struct nrf_rpc_cbor_ctx *ctx, uint64_t value);

/** @brief Encode an 64bits integer value.
 *
 * @param[in,out] ctx Structure used to encode CBOR stream.
 * @param[in]     value Encoded value.
 */
void nrf_rpc_encode_int64(struct nrf_rpc_cbor_ctx *ctx, int64_t value);

/** @brief Encode a string value.
 *
 * @param[in,out] ctx CBOR encoding context.
 * @param[in] value String to encode.
 * @param[in] len String length.
 */
void nrf_rpc_encode_str(struct nrf_rpc_cbor_ctx *ctx, const char *value, int len);

/** @brief Encode a buffer.
 *
 * @param[in,out] ctx CBOR encoding context.
 * @param[in] data Buffer to encode.
 * @param[in] size Buffer size.
 */
void nrf_rpc_encode_buffer(struct nrf_rpc_cbor_ctx *ctx, const void *data, size_t size);

/** @brief Encode a callback.
 *
 * This function will use callback proxy module to convert a callback pointer
 * to an integer value (slot number).
 *
 * @param[in,out] ctx CBOR encoding context.
 * @param[in] callback Callback to encode.
 */
void nrf_rpc_encode_callback(struct nrf_rpc_cbor_ctx *ctx, void *callback);

/** @brief Encode a callback slot number.
 *
 * @param[in,out] ctx CBOR encoding context.
 * @param[in] slot Callback slot number to encode.
 */
static inline void nrf_rpc_encode_callback_call(struct nrf_rpc_cbor_ctx *ctx, uint32_t slot)
{
	nrf_rpc_encode_uint(ctx, slot);
}

/** @brief Put encode into an invalid state. All further encoding on this encoder will be ignored.
 *         Invalid state can be checked with the is_encoder_invalid() function
 *
 * @param[in,out] ctx CBOR encoding context.
 */
void nrf_rpc_encoder_invalid(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Skip one value to decode.
 *
 * @param[in] ctx CBOR decoding context.
 */
void nrf_rpc_decode_skip(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Check if value is a null. This function will not consume the value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval True, if value is a null.
 *         Otherwise, false will be returned.
 */
bool nrf_rpc_decode_is_null(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Check if value is an undefined. This function will not consume the value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval True, if value is an undefined.
 *         Otherwise, false will be returned.
 */
bool nrf_rpc_decode_is_undefined(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a boolean value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval Decoded boolean value.
 */
bool nrf_rpc_decode_bool(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode an unsigned integer value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval Decoded unsigned integer value.
 */
uint32_t nrf_rpc_decode_uint(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a integer value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval Decoded integer value.
 */
int32_t nrf_rpc_decode_int(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a 64bits unsigned integer value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval Decoded value.
 */
uint64_t nrf_rpc_decode_uint64(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a 64bits integer value.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval Decoded value.
 */
int64_t nrf_rpc_decode_int64(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a string value.
 *
 * @param[in,out] ctx CBOR decoding context.
 * @param[out] buffer Buffer for decoded string.
 * @param[in]  buffer_size Buffer size.
 *
 * @retval Pointer to a decoded string.
 */
char *nrf_rpc_decode_str(struct nrf_rpc_cbor_ctx *ctx, char *buffer, size_t buffer_size);

/** Decode a string value into a scratchpad.
 *
 * @param[in] scratchpad Pointer to the scratchpad.
 * @param[out] len length of decoded string in bytes.
 *
 * @retval Pointer to a decoded string.
 */
char *nrf_rpc_decode_str_into_scratchpad(struct nrf_rpc_scratchpad *scratchpad, size_t *len);

/** @brief Decode a buffer.
 *
 * @param[in,out] ctx CBOR decoding context.
 * @param[out] buffer Buffer for a decoded buffer data.
 * @param[in]  buffer_size Buffer size.
 *
 * @retval Pointer to a decoded buffer.
 */
void *nrf_rpc_decode_buffer(struct nrf_rpc_cbor_ctx *ctx, void *buffer, size_t buffer_size);

/** @brief Decode buffer pointer and length. Moves CBOR buffer pointer past buffer on success.
 *
 * @param[in,out] ctx CBOR decoding context.
 * @param[out]  size Buffer size.
 *
 * @retval Pointer to a buffer within CBOR stream or NULL on error.
 */
const void *nrf_rpc_decode_buffer_ptr_and_size(struct nrf_rpc_cbor_ctx *ctx, size_t *size);

/** @brief Decode buffer into a scratchpad.
 *
 * @param[in] scratchpad Pointer to the scratchpad.
 * @param[out] len length of decoded buffer in bytes.
 *
 * @retval Pointer to a decoded buffer data.
 */
void *nrf_rpc_decode_buffer_into_scratchpad(struct nrf_rpc_scratchpad *scratchpad, size_t *len);

/** @brief Decode a callback.
 *
 * This function will use callback proxy module to associate decoded integer
 * (slot number) with provided handler and returned function pointer.
 *
 * @param[in] ctx CBOR decoding context.
 * @param[in] handler Function which will be called when callback returned by
 *                    this function is called. The handler must be defined by
 *                    @ref NRF_RPC_CBKPROXY_HANDLER.
 *
 * @retval Decoded callback.
 */
void *nrf_rpc_decode_callbackd(struct nrf_rpc_cbor_ctx *ctx, void *handler);

/** @brief Decode callback slot.
 *
 * This function will use callback proxy module to get callback associated with
 * the decoded slot number.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval Callback assigned to the slot encoded in the value parameter.
 */
void *nrf_rpc_decode_callback_call(struct nrf_rpc_cbor_ctx *ctx);

/** @brief Put decoder into an invalid state and set error code that caused it.
 *         All further decoding on this decoder will be ignored.
 *         Invalid state can be checked with the nrf_rpc_decode_valid() function.
 *
 * @param[in] ctx CBOR decoding context.
 * @param[in] err Cbor error code to set.
 */
void nrf_rpc_decoder_invalid(struct nrf_rpc_cbor_ctx *ctx, int err);

/** @brief Returns if decoder is in valid state.
 *
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval True if decoder is in valid state which means that no error occurred
 *         so far. Otherwise, false will be returned.
 */
bool nrf_rpc_decode_valid(const struct nrf_rpc_cbor_ctx *ctx);

/** @brief Signalize that decoding is done. Use this function when you finish decoding of the
 *         received serialized packet.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 *
 * @retval True if decoding finshed with success.
 *         Otherwise, false will be returned.
 */
bool nrf_rpc_decoding_done_and_check(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx);

/** @brief Decode a command response as a boolean value.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void nrf_rpc_rsp_decode_bool(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *handler_data);

/** @brief Decode a command response as an unsigned integer value.
 *
 * On decoding failure, this function triggers the nRF RPC error handler.
 * It then sets the result to zero, ensuring consistent output in error scenarios.
 *
 * @param[in] group	nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] result Pointer to an integer variable to store the result.
 * @param[in] result_size Size of the integer variable to store the result.
 */
void nrf_rpc_rsp_decode_uint(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *result, size_t result_size);

/** @brief Decode a command response as an unsigned 8-bit integer value.
 *
 * On decoding failure, this function triggers the nRF RPC error handler.
 * It then sets the result to zero, ensuring consistent output in error scenarios.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void nrf_rpc_rsp_decode_u8(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			   void *handler_data);

/** @brief Decode a command response as an unsigned 16-bit integer value.
 *
 * On decoding failure, this function triggers the nRF RPC error handler.
 * It then sets the result to zero, ensuring consistent output in error scenarios.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void nrf_rpc_rsp_decode_u16(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data);

/** @brief Decode a command response as an unsigned 32-bit integer value.
 *
 * On decoding failure, this function triggers the nRF RPC error handler.
 * It then sets the result to zero, ensuring consistent output in error scenarios.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void nrf_rpc_rsp_decode_u32(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data);

/** @brief Decode a command response as a signed integer value.
 *
 * On decoding failure, this function triggers the nRF RPC error handler.
 * It then sets the result to zero, ensuring consistent output in error scenarios.
 *
 * @param[in] group	nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] result Pointer to an integer variable to store the result.
 * @param[in] result_size Size of the integer variable to store the result.
 */
void nrf_rpc_rsp_decode_int(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *result, size_t result_size);

/** @brief Decode a command response as a signed 8-bit integer value.
 *
 * On decoding failure, this function triggers the nRF RPC error handler.
 * It then sets the result to zero, ensuring consistent output in error scenarios.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void nrf_rpc_rsp_decode_i8(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			   void *handler_data);

/** @brief Decode a command response as a signed 16-bit integer value.
 *
 * On decoding failure, this function triggers the nRF RPC error handler.
 * It then sets the result to zero, ensuring consistent output in error scenarios.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void nrf_rpc_rsp_decode_i16(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data);

/** @brief Decode a command response as a signed 32-bit integer value.
 *
 * On decoding failure, this function triggers the nRF RPC error handler.
 * It then sets the result to zero, ensuring consistent output in error scenarios.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void nrf_rpc_rsp_decode_i32(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data);

/** @brief Decode a command response as a void value.
 *
 * @param[in] group nRF RPC group.
 * @param[in,out] ctx CBOR decoding context.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void nrf_rpc_rsp_decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *handler_data);

/** @brief Send response to a command as an integer value.
 *
 * @param[in] group nRF RPC group.
 * @param[in] response Integer value to send.
 */
void nrf_rpc_rsp_send_int(const struct nrf_rpc_group *group, int32_t response);

/** @brief Send response to a command as an unsigned integer value.
 *
 * @param[in] group nRF RPC group.
 * @param[in] response Unsigned integer value to send.
 */
void nrf_rpc_rsp_send_uint(const struct nrf_rpc_group *group, uint32_t response);

/** @brief Send response to a command as a boolean value.
 *
 * @param[in] group nRF RPC group.
 * @param[in] response Boolean value to send.
 */
void nrf_rpc_rsp_send_bool(const struct nrf_rpc_group *group, bool response);

/** @brief Send response to a command as a void.
 *
 * @param[in] group nRF RPC group.
 */
void nrf_rpc_rsp_send_void(const struct nrf_rpc_group *group);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NRF_RPC_SERIALIZE_H_ */

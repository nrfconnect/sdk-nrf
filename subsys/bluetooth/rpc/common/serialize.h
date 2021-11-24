/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_rpc_serialize Bluetooth RPC serialize API
 * @{
 * @brief API for the Bluetooth RPC serialization.
 */


#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include <tinycbor/cbor.h>
#include <net/buf.h>
#include <sys/util.h>

/** @brief Get a scratchpad item size aligned to 4-byte boundary.
 *
 * @param[in] size scratchpad item size
 *
 * @retval The scratchpad item size rounded up to the next multiple of 4.
 */
#define SCRATCHPAD_ALIGN(size) WB_UP(size)

/** @brief Alloc the scratchpad. Scratchpad is used to store a data when decoding serialized data.
 *
 *  @param[in] _scratchpad Scratchpad name.
 *  @param[in] _value Cbor value to decode. One unsigned integer will be decoded
 *                    from this value that contains scratchpad buffer size.
 */
#define SER_SCRATCHPAD_DECLARE(_scratchpad, _value)                                               \
	(_scratchpad)->value = _value;                                                          \
	uint32_t _scratchpad_size = ser_decode_uint(_value);                                    \
	uint32_t _scratchpad_data[SCRATCHPAD_ALIGN(_scratchpad_size) / sizeof(uint32_t)];       \
	net_buf_simple_init_with_data(&(_scratchpad)->buf, _scratchpad_data, _scratchpad_size); \
	net_buf_simple_reset(&(_scratchpad)->buf)


/** @brief Scratchpad structure. */
struct ser_scratchpad {
	/** Cbor value to decode. */
	CborValue *value;

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
static inline void *ser_scratchpad_add(struct ser_scratchpad *scratchpad, size_t size)
{
	return net_buf_simple_add(&scratchpad->buf, SCRATCHPAD_ALIGN(size));
}

/** @brief Encode a null value.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_null(CborEncoder *encoder);

/** @brief Encode an undefined value.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_undefined(CborEncoder *encoder);

/** @brief Encode a boolean value.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_bool(CborEncoder *encoder, bool value);

/** @brief Encode an unsigned integer value.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_uint(CborEncoder *encoder, uint32_t value);

/** @brief Encode an integer value.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_int(CborEncoder *encoder, int32_t value);

/** @brief Encode an unsigned 64-bit integer value.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_uint64(CborEncoder *encoder, uint64_t value);

/** @brief Encode a 64-bit integer value.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_int64(CborEncoder *encoder, int64_t value);

/** @brief Encode a string value.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 * @param[in] value String to encode.
 * @param[in] len String length.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_str(CborEncoder *encoder, const char *value, int len);

/** @brief Encode a buffer.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 * @param[in] data Buffer to encode.
 * @param[in] size Buffer size.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encode_buffer(CborEncoder *encoder, const void *data, size_t size);

/** @brief Encode a callback.
 *
 * This function will use callback proxy module to convert a callback pointer
 * to an integer value (slot number).
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 * @param[in] callback Callback to encode.
 */
void ser_encode_callback(CborEncoder *encoder, void *callback);

/** @brief Encode a callback slot number.
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 * @param[in] slot Callback slot number to encode.
 */
static inline void ser_encode_callback_call(CborEncoder *encoder, uint32_t slot)
{
	ser_encode_uint(encoder, slot);
}

/** @brief Put encode into an invalid state. All further encoding on this encoder will be ignored.
 *         Invalid state can be checked with the is_encoder_invalid() function
 *
 * @param[in, out] encoder Structure used to encode CBOR stream.
 */
void ser_encoder_invalid(CborEncoder *encoder);

/** @brief Skip one value to decode.
 *
 * @param[in] value Value parsed from the CBOR stream.
 */
void ser_decode_skip(CborValue *value);

/** @brief Check if value is a null. This function will not consume the value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval True, if value is a null.
 *         Otherwise, false will be returned.
 */
bool ser_decode_is_null(CborValue *value);

/** @brief Check if value is an undefined. This function will not consume the value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval True, if value is an undefined.
 *         Otherwise, false will be returned.
 */
bool ser_decode_is_undefined(CborValue *value);

/** @brief Decode a boolean value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval Decoded boolean value.
 */
bool ser_decode_bool(CborValue *value);

/** @brief Decode an unsigned integer value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval Decoded unsigned integer value.
 */
uint32_t ser_decode_uint(CborValue *value);

/** @brief Decode a integer value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval Decoded integer value.
 */
int32_t ser_decode_int(CborValue *value);

/** @brief Decode an unsigned 64-bit integer value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval Decoded an unsigned 64-bit integer value.
 */
uint64_t ser_decode_uint64(CborValue *value);

/** @brief Decode a 64-bit integer value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval Decoded a 64-bit integer value.
 */
int64_t ser_decode_int64(CborValue *value);

/** @brief Decode a string value.
 *
 * @param[in]  value Value parsed from the CBOR stream.
 * @param[out] buffer Buffer for decoded string.
 * @param[in]  size Buffer size.
 */
void ser_decode_str(CborValue *value, char *buffer, size_t size);

/** Decode a string value into a scratchpad.
 *
 * @param[in] scratchpad Pointer to the scratchpad.
 *
 * @retval Pointer to a decoded string.
 */
char *ser_decode_str_into_scratchpad(struct ser_scratchpad *scratchpad);

/** @brief Decode a buffer.
 *
 * @param[in]  value Value parsed from the CBOR stream.
 * @param[out] buffer Buffer for a decoded buffer data.
 * @param[in]  size Buffer size.
 *
 * @retval Pointer to a decoded buffer.
 */
void *ser_decode_buffer(CborValue *value, void *buffer, size_t buffer_size);

/** @brief Decode a buffer size. This function will not consume the buffer.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval Decoded buffer size.
 */
size_t ser_decode_buffer_size(CborValue *value);

/** @brief Decode buffer into a scratchpad.
 *
 * @param[in] scratchpad Pointer to the scratchpad.
 *
 * @retval Pointer to a decoded buffer data.
 */
void *ser_decode_buffer_into_scratchpad(struct ser_scratchpad *scratchpad);

/** @brief Decode a callback.
 *
 * This function will use callback proxy module to associate decoded integer
 * (slot number) with provided handler and returned function pointer.
 *
 * @param[in] value Value parsed from the CBOR stream.
 * @param[in] handler Function which will be called when callback returned by
 *                    this function is called. The handler must be defined by
 *                    @ref CBKPROXY_HANDLER.
 *
 * @retval Decoded callback.
 */
void *ser_decode_callback(CborValue *value, void *handler);

/** @brief Decode callback slot.
 *
 * This function will use callback proxy module to get callback associated with
 * the decoded slot number.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval Callback assigned to the slot encoded in the value parameter.
 */
void *ser_decode_callback_call(CborValue *value);

/** @brief Put decoder into an invalid state and set error code that caused it.
 *         All further decoding on this decoder will be ignored.
 *         Invalid state can be checked with the ser_decode_valid() function.
 *
 * @param[in] value Value parsed from the CBOR stream.
 * @param[in] err Cbor error code to set.
 */
void ser_decoder_invalid(CborValue *value, CborError err);

/** @brief Returns if decoder is in valid state.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval True if decoder is in valid state which means that no error occurred
 *         so far. Otherwise, false will be returned.
 */
bool ser_decode_valid(CborValue *value);

/** @brief Signalize that decoding is done. Use this function when you finish decoding of the
 *         received serialized packet.
 *
 * @param[in] value Value parsed from the CBOR stream.
 *
 * @retval True if decoding finshed with success.
 *         Otherwise, false will be returned.
 */
bool ser_decoding_done_and_check(CborValue *value);

/** @brief Decode a command response as a boolean value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void ser_rsp_decode_bool(CborValue *value, void *handler_data);

/** @brief Decode a command response as an unsigned 8-bit integer value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void ser_rsp_decode_u8(CborValue *value, void *handler_data);

/** @brief Decode a command response as an unsigned 16-bit integer value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void ser_rsp_decode_u16(CborValue *value, void *handler_data);

/** @brief Decode a command response as an integer value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void ser_rsp_decode_i32(CborValue *value, void *handler_data);

/** @brief Decode a command response as a void value.
 *
 * @param[in] value Value parsed from the CBOR stream.
 * @param[in] handler_data Pointer to place where value will be decoded.
 */
void ser_rsp_decode_void(CborValue *value, void *handler_data);

/** @brief Sent response to a command as an integer value.
 *
 * @param[in] response Integer value to send.
 */
void ser_rsp_send_int(int32_t response);

/** @brief Sent response to a command as an unsigned integer value.
 *
 * @param[in] response Unsigned integer value to send.
 */
void ser_rsp_send_uint(uint32_t response);

/** @brief Sent response to a command as a boolean value.
 *
 * @param[in] response Boolean value to send.
 */
void ser_rsp_send_bool(bool response);

/** @brief Sent response to a command as a void. */
void ser_rsp_send_void(void);

#endif /* SERIALIZE_H_ */

#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include "tinycbor/cbor.h"

#ifndef SERIALIZE
#define SERIALIZE(...)
#endif

#define SCRATCHPAD_ALIGN(size) (((size) + 3) & ~3);

#define SER_SCRATCHPAD_ALLOC(_scratchpad, _value) \
	(_scratchpad)->value = _value; \
	(_scratchpad)->size = ser_decode_uint(_value); \
	uint32_t _scratchpad_buf[((_scratchpad)->size + 3) & ~3]; \
	(_scratchpad)->data = (uint8_t *)_scratchpad_buf
	
#define SER_SCRATCHPAD_FREE(_scratchpad)

struct ser_scratchpad {
	CborValue *value;
	uint8_t *data;
	size_t size;
};

void *ser_scratchpad_get(struct ser_scratchpad *scratchpad, size_t size);

void ser_encode_null(CborEncoder *encoder);
void ser_encode_uint(CborEncoder *encoder, uint32_t value);
void ser_encode_int(CborEncoder *encoder, int32_t value);
void ser_encode_uint64(CborEncoder *encoder, uint64_t value);
void ser_encode_int64(CborEncoder *encoder, int64_t value);
void ser_encode_buffer(CborEncoder *encoder, const void *data, size_t size);
void ser_encode_callback(CborEncoder *encoder, void *callback);
static inline void ser_encode_callback_slot(CborEncoder *encoder, uint32_t slot);
void ser_encoder_invalid(CborEncoder *encoder);

void ser_decode_skip(CborValue *value);
uint32_t ser_decode_uint(CborValue *value);
int32_t ser_decode_int(CborValue *value);
uint64_t ser_decode_uint64(CborValue *value);
int64_t ser_decode_int64(CborValue *value);
void *ser_decode_buffer(CborValue *value, void *buffer, size_t buffer_size);
size_t ser_decode_buffer_size(CborValue *value);
void *ser_decode_buffer_sp(struct ser_scratchpad *scratchpad);
void *ser_decode_callback(CborValue *value, void* handler);
void *ser_decode_callback_slot(CborValue *value);
void ser_decoder_invalid(CborValue *value, CborError err);

bool ser_decoding_done_and_check(CborValue *value);

void ser_rsp_simple_i32(CborValue *value, void *handler_data);
void ser_rsp_simple_void(CborValue *value, void *handler_data);

void ser_rsp_send_int(int32_t response);
void ser_rsp_send_void();

static inline void ser_encode_callback_slot(CborEncoder *encoder, uint32_t slot)
{
	ser_encode_uint(encoder, slot);
}


#endif /* SERIALIZE_H_ */

#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include "tinycbor/cbor.h"

#ifndef SERIALIZE
#define SERIALIZE(...)
#endif

typedef uint32_t ser_callback_slot_t;

void ser_encode_uint(CborEncoder *encoder, uint32_t value);
void ser_encode_int(CborEncoder *encoder, int32_t value);
void ser_encode_uint64(CborEncoder *encoder, uint64_t value);
void ser_encode_int64(CborEncoder *encoder, int64_t value);
void ser_encode_callback(CborEncoder *encoder, void *callback);
static inline void ser_encode_callback_slot(CborEncoder *encoder, ser_callback_slot_t slot);

uint32_t ser_decode_uint(CborValue *value);
int32_t ser_decode_int(CborValue *value);
uint64_t ser_decode_uint64(CborValue *value);
int64_t ser_decode_int64(CborValue *value);
void* ser_decode_callback(CborValue *value, void* handler);
void* ser_decode_callback_slot(CborValue *value);

bool ser_decoding_done_and_check(CborValue *value);

void ser_rsp_simple_i32(CborValue *value, void *handler_data);

void ser_rsp_send_i32(int32_t response);


static inline void ser_encode_callback_slot(CborEncoder *encoder, ser_callback_slot_t slot)
{
	ser_encode_uint(encoder, slot);
}


#endif /* SERIALIZE_H_ */

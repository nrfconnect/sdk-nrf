#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include "tinycbor/cbor.h"

#ifndef SERIALIZE
#define SERIALIZE(...)
#endif

void ser_encode_callback(CborEncoder *encoder, void *callback);

int32_t ser_decode_i32(CborValue *value);
bool ser_decoding_done_and_check(CborValue *value);

void ser_rsp_simple_i32(CborValue *value, void *handler_data);

void ser_rsp_send_i32(int32_t response);

#endif /* SERIALIZE_H_ */

/* Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 */

#ifndef PET_DECODE_H__
#define PET_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_decode.h"
#include "types_pet_decode.h"



bool cbor_decode_Pet(
		const uint8_t * p_payload, size_t payload_len,
		Pet_t * p_result,
		bool complete);


#endif // PET_DECODE_H__

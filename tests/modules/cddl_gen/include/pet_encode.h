/* Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 */

#ifndef PET_ENCODE_H__
#define PET_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_encode.h"
#include "types_pet_encode.h"



bool cbor_encode_Pet(
		uint8_t * p_payload, size_t payload_len,
		const Pet_t * p_input,
		size_t *p_payload_len_out);


#endif // PET_ENCODE_H__

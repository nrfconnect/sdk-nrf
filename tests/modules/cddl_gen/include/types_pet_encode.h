/* Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 */

#ifndef TYPES_PET_ENCODE_H__
#define TYPES_PET_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_encode.h"


typedef struct {
	cbor_string_type_t _Pet_name_tstr[3];
	size_t _Pet_name_tstr_count;
	cbor_string_type_t _Pet_birthday;
	union {
		uint32_t _Pet_species_cat;
		uint32_t _Pet_species_dog;
		uint32_t _Pet_species_other;
	};
	enum {
		_Pet_species_cat,
		_Pet_species_dog,
		_Pet_species_other,
	} _Pet_species_choice;
} Pet_t;


#endif // TYPES_PET_ENCODE_H__

/* Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_encode.h"
#include "pet_encode.h"


static bool encode_Pet(
		cbor_state_t *p_state, const Pet_t * p_input)
{
	cbor_print("encode_Pet\n");
	uint32_t tmp_value;
	bool int_res;

	bool result = (((list_start_encode(p_state, 3) && (int_res = (((list_start_encode(p_state, 3) && (int_res = (multi_encode(1, 3, &(*p_input)._Pet_name_tstr_count, (void*)tstrx_encode, p_state, (&(*p_input)._Pet_name_tstr), sizeof(cbor_string_type_t))), ((list_end_encode(p_state, 3)) && int_res))))
	&& ((bstrx_encode(p_state, (&(*p_input)._Pet_birthday))))
	&& ((((*p_input)._Pet_species_choice == _Pet_species_cat) ? ((uintx32_encode(p_state, ((tmp_value=1, &tmp_value)))))
	: (((*p_input)._Pet_species_choice == _Pet_species_dog) ? ((uintx32_encode(p_state, ((tmp_value=2, &tmp_value)))))
	: (((*p_input)._Pet_species_choice == _Pet_species_other) ? ((uintx32_encode(p_state, ((tmp_value=3, &tmp_value)))))
	: false))))), ((list_end_encode(p_state, 3)) && int_res)))));

	if (!result)
	{
		cbor_trace();
	}

	return result;
}



bool cbor_encode_Pet(
		uint8_t * p_payload, size_t payload_len,
		const Pet_t * p_input,
		size_t *p_payload_len_out)
{
	cbor_state_t state = {
		.p_payload = p_payload,
		.p_payload_end = p_payload + payload_len,
		.elem_count = 1
	};

	cbor_state_t state_backups[3];

	cbor_state_backups_t backups = {
		.p_backup_list = state_backups,
		.current_backup = 0,
		.num_backups = 3,
	};

	state.p_backups = &backups;

	bool result = encode_Pet(&state, p_input);

	if (p_payload_len_out != NULL) {
		*p_payload_len_out = ((size_t)state.p_payload - (size_t)p_payload);
	}
	return result;
}

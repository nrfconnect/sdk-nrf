/* Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_decode.h"
#include "pet_decode.h"


static bool decode_Pet(
		cbor_state_t *p_state, Pet_t * p_result)
{
	cbor_print("decode_Pet\n");
	bool int_res;

	bool result = (((list_start_decode(p_state) && (int_res = (((list_start_decode(p_state) && (int_res = (multi_decode(1, 3, &(*p_result)._Pet_name_tstr_count, (void*)tstrx_decode, p_state, (&(*p_result)._Pet_name_tstr), sizeof(cbor_string_type_t))), ((list_end_decode(p_state)) && int_res))))
	&& ((bstrx_decode(p_state, (&(*p_result)._Pet_birthday))))
	&& ((union_start_code(p_state) && (int_res = ((((uintx32_expect_union(p_state, (1)))) && (((*p_result)._Pet_species_choice = _Pet_species_cat) || 1))
	|| (((uintx32_expect_union(p_state, (2)))) && (((*p_result)._Pet_species_choice = _Pet_species_dog) || 1))
	|| (((uintx32_expect_union(p_state, (3)))) && (((*p_result)._Pet_species_choice = _Pet_species_other) || 1))), union_end_code(p_state), int_res)))), ((list_end_decode(p_state)) && int_res)))));

	if (!result)
	{
		cbor_trace();
	}

	return result;
}



bool cbor_decode_Pet(
		const uint8_t * p_payload, size_t payload_len,
		Pet_t * p_result,
		bool complete)
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

	bool result = decode_Pet(&state, p_result);

	if (!complete) {
		return result;
	}
	if (result) {
		if (state.p_payload == state.p_payload_end) {
			return true;
		} else {
			cbor_state_t *p_state = &state; // for printing.
			cbor_print("p_payload_end: 0x%x\r\n", (uint32_t)state.p_payload_end);
			cbor_trace();
		}
	}
	return false;
}

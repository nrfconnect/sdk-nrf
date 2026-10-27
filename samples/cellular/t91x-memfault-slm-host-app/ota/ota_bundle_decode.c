/*
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "ota_bundle_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_ota_element(zcbor_state_t *state, struct ota_element *result);
static bool decode_ota_bundle(zcbor_state_t *state, struct ota_bundle *result);


static bool decode_ota_element(
		zcbor_state_t *state, struct ota_element *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_map_start_decode(state) && (((((zcbor_uint32_expect(state, (5))))
	&& (((zcbor_uint_decode(state, &(*result).target_choice, sizeof((*result).target_choice)))) && ((((((*result).target_choice == ota_element_target_nrf53_app_m_c) && ((1)))
	|| (((*result).target_choice == ota_element_target_nrf53_net_m_c) && ((1)))
	|| (((*result).target_choice == ota_element_target_nrf53_bl_m_c) && ((1)))
	|| (((*result).target_choice == ota_element_target_nrf91_app_m_c) && ((1)))
	|| (((*result).target_choice == ota_element_target_nrf91_bl_m_c) && ((1)))
	|| (((*result).target_choice == ota_element_target_nrf91_mdm_delta_m_c) && ((1)))
	|| (((*result).target_choice == ota_element_target_nrf91_mdm_full_m_c) && ((1)))) || (zcbor_error(state, ZCBOR_ERR_WRONG_VALUE), false)))))
	&& (((zcbor_uint32_expect(state, (6))))
	&& (zcbor_uint32_decode(state, (&(*result).size)))
	&& ((((((*result).size >= 1)
	&& ((*result).size <= UINT32_MAX)) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_ota_bundle(
		zcbor_state_t *state, struct ota_bundle *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_map_start_decode(state) && (((((zcbor_uint32_expect(state, (1))))
	&& (zcbor_uint32_expect(state, (1))))
	&& (((zcbor_uint32_expect(state, (2))))
	&& (zcbor_uint32_decode(state, (&(*result).bundle_size)))
	&& ((((((*result).bundle_size >= 1)
	&& ((*result).bundle_size <= UINT32_MAX)) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)))
	&& (((zcbor_uint32_expect(state, (3))))
	&& (zcbor_bstr_decode(state, (&(*result).bundle_sha256)))
	&& ((((*result).bundle_sha256.len >= 32)
	&& ((*result).bundle_sha256.len <= 32)) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)))
	&& (((zcbor_uint32_expect(state, (4))))
	&& (zcbor_list_start_decode(state) && ((zcbor_multi_decode(1, 7, &(*result).ota_element_m_count, (zcbor_decoder_t *)decode_ota_element, state, (&(*result).ota_element_m), sizeof(struct ota_element))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_map_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}



int cbor_decode_ota_bundle(
		const uint8_t *payload, size_t payload_len,
		struct ota_bundle *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[6];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_ota_bundle, sizeof(states) / sizeof(zcbor_state_t), 1);
}

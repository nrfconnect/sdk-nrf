/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include "zcbor_decode.h"
#include "zcbor_common.h"
#include "zcbor_print.h"
#include "zcbor_noncanonical_decode.h"

/** Return value length from additional value.
 */
static size_t additional_len(uint8_t additional)
{
	if (additional <= ZCBOR_VALUE_IN_HEADER) {
		return 0;
	} else if (ZCBOR_VALUE_IS_1_BYTE <= additional && additional <= ZCBOR_VALUE_IS_8_BYTES) {
		/* 24 => 1
		 * 25 => 2
		 * 26 => 4
		 * 27 => 8
		 */
		return 1U << (additional - ZCBOR_VALUE_IS_1_BYTE);
	}
	return 0xF;
}

static bool initial_checks(zcbor_state_t *state)
{
	ZCBOR_CHECK_ERROR();
	ZCBOR_CHECK_PAYLOAD();
	return true;
}

static bool type_check(zcbor_state_t *state, zcbor_major_type_t exp_major_type)
{
	if (!initial_checks(state)) {
		ZCBOR_FAIL();
	}
	zcbor_major_type_t major_type = ZCBOR_MAJOR_TYPE(*state->payload);

	if (major_type != exp_major_type) {
		ZCBOR_ERR(ZCBOR_ERR_WRONG_TYPE);
	}
	return true;
}

#define INITIAL_CHECKS()                                                                           \
	do {                                                                                       \
		if (!initial_checks(state)) {                                                      \
			ZCBOR_FAIL();                                                              \
		}                                                                                  \
	} while (0)

#define INITIAL_CHECKS_WITH_TYPE(exp_major_type)                                                   \
	do {                                                                                       \
		if (!type_check(state, exp_major_type)) {                                          \
			ZCBOR_FAIL();                                                              \
		}                                                                                  \
	} while (0)

static void err_restore(zcbor_state_t *state, int err)
{
	state->payload = state->payload_bak;
	state->elem_count++;
	zcbor_error(state, err);
}

#define ERR_RESTORE(err)                                                                           \
	do {                                                                                       \
		err_restore(state, err);                                                           \
		ZCBOR_FAIL();                                                                      \
	} while (0)

#define FAIL_RESTORE()                                                                             \
	do {                                                                                       \
		state->payload = state->payload_bak;                                               \
		state->elem_count++;                                                               \
		ZCBOR_FAIL();                                                                      \
	} while (0)

#define PRINT_FUNC() zcbor_log("%s ", __func__);

static void endian_copy(uint8_t *dst, const uint8_t *src, size_t src_len)
{
#ifdef ZCBOR_BIG_ENDIAN
	memcpy(dst, src, src_len);
#else
	for (size_t i = 0; i < src_len; i++) {
		dst[i] = src[src_len - 1 - i];
	}
#endif /* ZCBOR_BIG_ENDIAN */
}

/** Get a single value.
 *
 * @details @p ppayload must point to the header byte. This function will
 *          retrieve the value (either from within the additional info, or from
 *          the subsequent bytes) and return it in the result. The result can
 *          have arbitrary length.
 *
 *          The function will also validate
 *           - Min/max constraints on the value.
 *           - That @p payload doesn't overrun past @p payload_end.
 *           - That @p elem_count has not been exhausted.
 *
 *          @p ppayload and @p elem_count are updated if the function
 *          succeeds. If not, they are left unchanged.
 *
 *          CBOR values are always big-endian, so this function converts from
 *          big to little-endian if necessary (@ref ZCBOR_BIG_ENDIAN).
 */
static bool value_extract(zcbor_state_t *state, void *const result, size_t result_len)
{
	zcbor_trace(state, "value_extract");
	zcbor_assert_state(result_len != 0, "0-length result not supported.\r\n");
	zcbor_assert_state(result_len <= 8, "result sizes above 8 bytes not supported.\r\n");
	zcbor_assert_state(result != NULL, "result cannot be NULL.\r\n");

	INITIAL_CHECKS();
	ZCBOR_ERR_IF((state->elem_count == 0), ZCBOR_ERR_LOW_ELEM_COUNT);

	uint8_t additional = ZCBOR_ADDITIONAL(*state->payload);
	size_t len = additional_len(additional);
	uint8_t *result_offs = (uint8_t *)result + ZCBOR_ECPY_OFFS(result_len, MAX(1, len));

	ZCBOR_ERR_IF(additional > ZCBOR_VALUE_IS_8_BYTES, ZCBOR_ERR_ADDITIONAL_INVAL);
	ZCBOR_ERR_IF(len > result_len, ZCBOR_ERR_INT_SIZE);
	ZCBOR_ERR_IF((state->payload + len + 1) > state->payload_end, ZCBOR_ERR_NO_PAYLOAD);

	memset(result, 0, result_len);

	if (len == 0) {
		*result_offs = additional;
	} else {
		endian_copy(result_offs, state->payload + 1, len);
	}

	state->payload_bak = state->payload;
	(state->payload) += len + 1;
	(state->elem_count)--;
	return true;
}

static bool str_start_decode(zcbor_state_t *state, struct zcbor_string *result,
			     zcbor_major_type_t exp_major_type)
{
	INITIAL_CHECKS_WITH_TYPE(exp_major_type);

	if (!value_extract(state, &result->len, sizeof(result->len))) {
		ZCBOR_FAIL();
	}

	result->value = state->payload;
	return true;
}

static void partition_fragment(const zcbor_state_t *state, struct zcbor_string_fragment *result)
{
	result->fragment.len =
		MIN(result->fragment.len, (size_t)state->payload_end - (size_t)state->payload);
}

static bool start_decode_fragment(zcbor_state_t *state, struct zcbor_string_fragment *result,
				  zcbor_major_type_t exp_major_type)
{
	PRINT_FUNC();
	if (!str_start_decode(state, &result->fragment, exp_major_type)) {
		ZCBOR_FAIL();
	}

	result->offset = 0;
	result->total_len = result->fragment.len;
	partition_fragment(state, result);
	state->payload_end = state->payload + result->fragment.len;

	return true;
}

bool zcbor_noncanonical_bstr_start_decode_fragment(zcbor_state_t *state,
						   struct zcbor_string_fragment *result)
{
	PRINT_FUNC();
	if (!start_decode_fragment(state, result, ZCBOR_MAJOR_TYPE_BSTR)) {
		ZCBOR_FAIL();
	}
	if (!zcbor_new_backup(state, ZCBOR_MAX_ELEM_COUNT)) {
		FAIL_RESTORE();
	}
	return true;
}

static bool list_map_start_decode(zcbor_state_t *state, zcbor_major_type_t exp_major_type)
{
	size_t new_elem_count;
	bool indefinite_length_array = false;

	INITIAL_CHECKS_WITH_TYPE(exp_major_type);

	if (ZCBOR_ADDITIONAL(*state->payload) == ZCBOR_VALUE_IS_INDEFINITE_LENGTH) {
		/* Indefinite length array. */
		new_elem_count = ZCBOR_LARGE_ELEM_COUNT;
		ZCBOR_ERR_IF(state->elem_count == 0, ZCBOR_ERR_LOW_ELEM_COUNT);
		indefinite_length_array = true;
		state->payload_bak = state->payload++;
		state->elem_count--;
	} else {
		if (!value_extract(state, &new_elem_count, sizeof(new_elem_count))) {
			ZCBOR_FAIL();
		}
	}

	if (!zcbor_new_backup(state, new_elem_count)) {
		FAIL_RESTORE();
	}

	state->decode_state.indefinite_length_array = indefinite_length_array;

	return true;
}

bool zcbor_noncanonical_map_start_decode(zcbor_state_t *state)
{
	PRINT_FUNC();
	bool ret = list_map_start_decode(state, ZCBOR_MAJOR_TYPE_MAP);

	if (ret && !state->decode_state.indefinite_length_array) {
		if (state->elem_count >= (ZCBOR_MAX_ELEM_COUNT / 2)) {
			/* The new elem_count is too large. */
			ERR_RESTORE(ZCBOR_ERR_INT_SIZE);
		}
		state->elem_count *= 2;
	}
	return ret;
}

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zcbor_decode.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_mem_util.h>
#include <string.h>

#define SUIT_INVOKE_SYNCHRONOUS_KEY 1
#define SUIT_INVOKE_TIMEOUT_KEY	    2

suit_plat_err_t suit_plat_decode_component_id(struct zcbor_string *component_id, uint8_t *cpu_id,
					      intptr_t *run_address, size_t *size)
{
	if ((component_id == NULL) || (cpu_id == NULL) || (run_address == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS, 0);
	struct zcbor_string component_type;
	bool res;

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_decode(state, &component_type);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_int_decode(state, cpu_id, 1);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_size_decode(state, (size_t *)run_address);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_size_decode(state, (size_t *)size);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_list_end_decode(state);

	if (res) {
		*run_address = (intptr_t)suit_plat_mem_ptr_get(*run_address);
		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_CBOR_DECODING;
}

#ifdef CONFIG_SUIT_PLATFORM
suit_plat_err_t suit_plat_decode_component_type(struct zcbor_string *component_id,
						suit_component_type_t *type)
{
	if ((type == NULL) || (component_id == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS, 0);
	struct zcbor_string tmp;
	bool res;

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_tstr_decode(state, &tmp);
	res = res && zcbor_bstr_end_decode(state);

	if (!res) {
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	if ((tmp.len == strlen("MEM")) && (memcmp(tmp.value, "MEM", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_MEM;
	} else if ((tmp.len == strlen("CAND_IMG")) &&
		   (memcmp(tmp.value, "CAND_IMG", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_CAND_IMG;
	} else if ((tmp.len == strlen("CAND_MFST")) &&
		   (memcmp(tmp.value, "CAND_MFST", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_CAND_MFST;
	} else if ((tmp.len == strlen("INSTLD_MFST")) &&
		   (memcmp(tmp.value, "INSTLD_MFST", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_INSTLD_MFST;
	} else if ((tmp.len == strlen("SOC_SPEC")) &&
		   (memcmp(tmp.value, "SOC_SPEC", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_SOC_SPEC;
	} else if ((tmp.len == strlen("CACHE_POOL")) &&
		   (memcmp(tmp.value, "CACHE_POOL", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_CACHE_POOL;
	} else if ((tmp.len == strlen("MFST_VAR")) &&
		   (memcmp(tmp.value, "MFST_VAR", tmp.len) == 0)) {
		*type = SUIT_COMPONENT_TYPE_MFST_VAR;
	} else {
		*type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	return SUIT_PLAT_SUCCESS;
}
#endif /* CONFIG_SUIT_PLATFORM */

suit_plat_err_t suit_plat_decode_address_size(struct zcbor_string *component_id,
					      intptr_t *run_address, size_t *size)
{
	if ((component_id == NULL) || (size == NULL) || (run_address == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS, 0);
	struct zcbor_string component_type;
	bool res;
	uint8_t cpu_id;

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_decode(state, &component_type);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_int_decode(state, &cpu_id, 1);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_size_decode(state, (size_t *)run_address);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_size_decode(state, (size_t *)size);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_list_end_decode(state);

	if (res) {
		*run_address = (intptr_t)suit_plat_mem_ptr_get(*run_address);
		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_CBOR_DECODING;
}

suit_plat_err_t suit_plat_decode_component_number(struct zcbor_string *component_id,
						  uint32_t *number)
{
	if ((component_id == NULL) || (number == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS, 0);
	struct zcbor_string component_type;

	bool res = zcbor_list_start_decode(state);

	res = res && zcbor_bstr_decode(state, &component_type);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_uint32_decode(state, number);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_list_end_decode(state);

	if (res) {
		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_CBOR_DECODING;
}

static suit_plat_err_t suit_plat_decode_uint32(struct zcbor_string *bstr, uint32_t *value)
{
	if ((bstr == NULL) || (bstr->value == NULL) || (bstr->len == 0) || (value == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	ZCBOR_STATE_D(state, 2, bstr->value, bstr->len, 1, 0);

	if (zcbor_uint32_decode(state, value)) {
		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_CBOR_DECODING;
}

suit_plat_err_t suit_plat_decode_content_uint32(struct zcbor_string *content, uint32_t *value)
{
	return suit_plat_decode_uint32(content, value);
}

suit_plat_err_t suit_plat_decode_key_id(struct zcbor_string *key_id, uint32_t *integer_key_id)
{
	suit_plat_err_t ret = suit_plat_decode_uint32(key_id, integer_key_id);

	if (ret == SUIT_PLAT_ERR_INVAL) {
		return SUIT_PLAT_SUCCESS;
	}

	return ret;
}

#ifdef CONFIG_SUIT_METADATA
suit_plat_err_t suit_plat_decode_manifest_class_id(struct zcbor_string *component_id,
						   suit_manifest_class_id_t **class_id)
{
	struct zcbor_string component_type;
	struct zcbor_string class_id_bstr;
	bool res;

	if ((component_id == NULL) || (class_id == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	ZCBOR_STATE_D(state, 2, component_id->value, component_id->len,
		      SUIT_PLAT_MAX_NUM_COMPONENT_ID_PARTS, 0);

	res = zcbor_list_start_decode(state);
	res = res && zcbor_bstr_start_decode(state, NULL);
	res = res && zcbor_tstr_decode(state, &component_type);
	res = res && zcbor_bstr_end_decode(state);
	res = res && zcbor_bstr_decode(state, &class_id_bstr);
	res = res && zcbor_list_end_decode(state);

	if (res) {
		if ((component_type.len != strlen("INSTLD_MFST")) ||
		    (memcmp(component_type.value, "INSTLD_MFST", component_type.len) != 0)) {
			res = false;
		}
	}

	if (res) {
		if (class_id_bstr.len != sizeof(suit_manifest_class_id_t)) {
			res = false;
		} else {
			*class_id = (suit_uuid_t *)class_id_bstr.value;
		}
	}

	if (res) {
		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_CBOR_DECODING;
}
#endif /* CONFIG_SUIT_METADATA */

suit_plat_err_t suit_plat_decode_invoke_args(struct zcbor_string *invoke_args, bool *synchronous,
					     uint32_t *timeout_ms)
{
	bool cbor_synchronous = false;
	uint32_t cbor_timeout_ms = 0;
	bool res;

	if ((invoke_args == NULL) || (invoke_args->value == NULL) || (invoke_args->len == 0)) {
		/* If invoke arguments are not set - assume asynchronous invoke. */
		if (synchronous != NULL) {
			*synchronous = false;
		}
		if (timeout_ms != NULL) {
			*timeout_ms = 0;
		}

		return SUIT_PLAT_SUCCESS;
	}

	ZCBOR_STATE_D(state, 2, invoke_args->value, invoke_args->len, 1, 0);

	res = zcbor_map_start_decode(state);
	res = res && zcbor_uint32_expect(state, SUIT_INVOKE_SYNCHRONOUS_KEY);
	res = res && zcbor_bool_decode(state, &cbor_synchronous);
	if (res && cbor_synchronous) {
		/* Timeout value is mandatory if the synchronous flag is set. */
		res = res && zcbor_uint32_expect(state, SUIT_INVOKE_TIMEOUT_KEY);
		res = res && zcbor_uint32_decode(state, &cbor_timeout_ms);
	}
	res = res && zcbor_map_end_decode(state);

	if (res) {
		if (synchronous != NULL) {
			*synchronous = cbor_synchronous;
		}
		if (timeout_ms != NULL) {
			*timeout_ms = cbor_timeout_ms;
		}

		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_CBOR_DECODING;
}

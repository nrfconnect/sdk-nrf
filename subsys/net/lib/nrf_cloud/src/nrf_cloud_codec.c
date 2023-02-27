/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/nrf_cloud_codec.h>
#include "nrf_cloud_mem.h"
#include "nrf_cloud_codec_internal.h"

LOG_MODULE_REGISTER(nrf_cloud_codec, CONFIG_NRF_CLOUD_LOG_LEVEL);

int nrf_cloud_obj_input_decode(struct nrf_cloud_obj *const obj,
	const struct nrf_cloud_data *const input)
{
	if (!obj || !input || !input->ptr || !input->len) {
		return -EINVAL;
	}

	cJSON *json;

	obj->type = NRF_CLOUD_OBJ_TYPE__UNDEFINED;

	/* Only try to parse JSON if the data is null-terminated */
	if (memchr(input->ptr, '\0', input->len + 1) != NULL) {
		json = cJSON_Parse(input->ptr);
		if (json) {
			obj->json = json;
			obj->type = NRF_CLOUD_OBJ_TYPE_JSON;
		}
	} else {
		// TODO: CBOR
	}

	if (NRF_CLOUD_OBJ_TYPE_VALID(obj) == false) {
		return -ENOMSG;
	}

	return 0;
}

int nrf_cloud_obj_msg_check(const struct nrf_cloud_obj *const obj, const char *const app_id,
	const char *const msg_type)
{
	if (!obj || !obj->json || (!app_id && !msg_type)) {
		return -EINVAL;
	}


	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		char *str = NULL;
		bool match = true;

		if (!obj->json) {
			return -ENOENT;
		}

		if (app_id) {
			(void)get_string_from_obj(obj->json, NRF_CLOUD_JSON_APPID_KEY, &str);
			match &= (str && (strcmp(str, app_id) == 0));
		}

		if (msg_type) {
			str = NULL;
			(void)get_string_from_obj(obj->json, NRF_CLOUD_JSON_MSG_TYPE_KEY, &str);
			match &= (str && (strcmp(str, msg_type) == 0));
		}

		return match ? 0 : -ENOMSG;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_str_get(const struct nrf_cloud_obj *const obj, const char *const key,
	char **str)
{
	if (!obj || !key || !str) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		return get_string_from_obj(obj->json, key, str);
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_msg_init(struct nrf_cloud_obj *const obj, const char *const app_id,
	const char *const msg_type)
{
	if (!obj || !app_id) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (obj->json) {
			return -ENOTEMPTY;
		}

		int err;
		cJSON *new_obj = cJSON_CreateObject();

		err = (cJSON_AddStringToObjectCS(new_obj, NRF_CLOUD_JSON_APPID_KEY,
						 app_id) ? 0 : -ENOMEM);

		if (!err && msg_type) {
			err = (cJSON_AddStringToObjectCS(new_obj,
							 NRF_CLOUD_JSON_MSG_TYPE_KEY,
							 msg_type) ? 0 : -ENOMEM);
		}

		if (err) {
			cJSON_Delete(new_obj);
			return -ENOMEM;
		}

		obj->json = new_obj;
		return 0;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_init(struct nrf_cloud_obj *const obj)
{
	if (!obj) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (obj->json) {
			return -ENOTEMPTY;
		}

		obj->json = cJSON_CreateObject();
		return (obj->json ? 0 : -ENOMEM);
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_reset(struct nrf_cloud_obj *const obj)
{
	if (!obj) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		obj->json = NULL;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		return -ENOTSUP;
	}

	obj->encoded_data.len = 0;
	obj->encoded_data.ptr = NULL;

	return -ENOTSUP;
}

int nrf_cloud_obj_bulk_init(struct nrf_cloud_obj *const bulk)
{
	if (!bulk) {
		return -EINVAL;
	}

	switch (bulk->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (bulk->json) {
			return -ENOTEMPTY;
		}

		bulk->json = cJSON_CreateArray();
		return (bulk->json ? 0 : -ENOMEM);
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_cloud_encoded_free(struct nrf_cloud_obj *const obj)
{
	if (!obj) {
		return -EINVAL;
	}

	if (obj->enc_src != NRF_CLOUD_ENC_SRC_CLOUD_ENCODED) {
		return -EACCES;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (obj->encoded_data.ptr) {
			cJSON_free((void *)obj->encoded_data.ptr);
			obj->encoded_data.ptr = NULL;
		}

		obj->encoded_data.len = 0;
		obj->enc_src = NRF_CLOUD_ENC_SRC_NONE;
		return 0;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_cloud_pre_encoded_free(struct nrf_cloud_obj *const obj)
{
	if (!obj) {
		return -EINVAL;
	}

	if (obj->enc_src != NRF_CLOUD_ENC_SRC_PRE_ENCODED) {
		return -EACCES;
	}

	if (obj->encoded_data.ptr) {
		nrf_cloud_free((void *)obj->encoded_data.ptr);
		obj->encoded_data.ptr = NULL;
	}

	obj->encoded_data.len = 0;
	obj->enc_src = NRF_CLOUD_ENC_SRC_NONE;

	return 0;
}


int nrf_cloud_obj_free(struct nrf_cloud_obj *const obj)
{
	if (!obj) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		cJSON_Delete(obj->json);
		obj->json = NULL;
		return 0;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_bulk_add(struct nrf_cloud_obj *const bulk, struct nrf_cloud_obj *const obj)
{
	if (!bulk || !obj) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!bulk->json || !obj->json) {
			return -ENOENT;
		}

		if (!cJSON_IsArray(bulk->json)) {
			return -ENODEV;
		}

		return (cJSON_AddItemToArray(bulk->json, obj->json) ? 0 : -EIO);
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_ts_add(struct nrf_cloud_obj *const obj, const int64_t time_ms)
{
	if (!obj) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!obj->json) {
			return -ENOENT;
		}

		/* TODO: if time_ms <= 0, obtain valid timestamp (if Kconfigured) */
		return (cJSON_AddNumberToObjectCS(obj->json, NRF_CLOUD_MSG_TIMESTAMP_KEY,
						  time_ms) ? 0 : -ENOMEM);
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

static cJSON * data_obj_get(cJSON *const root_obj)
{
	cJSON *dest = NULL;

	dest = cJSON_GetObjectItem(root_obj, NRF_CLOUD_JSON_DATA_KEY);
	if (!dest) {
		dest = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_DATA_KEY);
	}

	return dest;
}

static cJSON * dest_json_get(struct nrf_cloud_obj *const obj, const bool data_child)
{
	return data_child ? data_obj_get(obj->json) : obj->json;
}

int nrf_cloud_obj_num_add(struct nrf_cloud_obj *const obj, const char *const key,
			  const double val, const bool data_child)
{
	if (!obj || !key) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!obj->json) {
			return -ENOENT;
		}
		return cJSON_AddNumberToObjectCS(dest_json_get(obj, data_child),
						 key, val) ? 0 : -ENOMEM;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_str_add(struct nrf_cloud_obj *const obj, const char *const key,
			  const char *const val, const bool data_child)
{
	if (!obj || !key || !val) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!obj->json) {
			return -ENOENT;
		}
		return cJSON_AddStringToObjectCS(dest_json_get(obj, data_child),
						 key, val) ? 0 : -ENOMEM;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_bool_add(struct nrf_cloud_obj *const obj, const char *const key,
			   const bool val, const bool data_child)
{
	if (!obj || !key) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!obj->json) {
			return -ENOENT;
		}
		return cJSON_AddBoolToObjectCS(dest_json_get(obj, data_child),
					       key, val) ? 0 : -ENOMEM;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_null_add(struct nrf_cloud_obj *const obj, const char *const key,
			   const bool data_child)
{
	if (!obj || !key) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!obj->json) {
			return -ENOENT;
		}
		return cJSON_AddNullToObjectCS(dest_json_get(obj, data_child), key) ? 0 : -ENOMEM;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_object_add(struct nrf_cloud_obj *const obj, const char *const key,
			     struct nrf_cloud_obj *const obj_to_add, const bool data_child)
{
	if (!obj || !key || !obj_to_add) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!obj->json || !obj_to_add->json) {
			return -ENOENT;
		}

		return cJSON_AddItemToObjectCS(dest_json_get(obj, data_child),
					       key, obj_to_add->json) ? 0 : -ENOMEM;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_int_array_add(struct nrf_cloud_obj *const obj, const char *const key,
				const uint32_t ints[], const uint32_t ints_cnt, const bool data_child)
{
	if (!obj || !key || !ints || !ints_cnt) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		cJSON *array = cJSON_CreateIntArray(ints, ints_cnt);

		return cJSON_AddItemToObjectCS(dest_json_get(obj, data_child),
					       key, array) ? 0 : -ENOMEM;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_str_array_add	(struct nrf_cloud_obj *const obj, const char *const key,
				 const char *const strs[], const uint32_t strs_cnt, const bool data_child)
{
	if (!obj || !key || !strs || !strs_cnt) {
		return -EINVAL;
	}
	if (!obj->json) {
		return -ENOENT;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		cJSON *array = cJSON_CreateStringArray(strs, strs_cnt);

		return cJSON_AddItemToObjectCS(dest_json_get(obj, data_child),
					       key, array) ? 0 : -ENOMEM;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_cloud_encode(struct nrf_cloud_obj *const obj)
{
	if (!obj) {
		return -EINVAL;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		obj->encoded_data.ptr = cJSON_PrintUnformatted(obj->json);

		if (obj->encoded_data.ptr == NULL) {
			return -ENOMEM;
		}

		obj->encoded_data.len = strlen(obj->encoded_data.ptr);
		obj->enc_src = NRF_CLOUD_ENC_SRC_CLOUD_ENCODED;

		return 0;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_gnss_msg_create(struct nrf_cloud_obj *const obj,
				  const struct nrf_cloud_gnss_data *const gnss)
{
	if (!gnss || !obj) {
		return -EINVAL;
	}
	if (!NRF_CLOUD_OBJ_TYPE_VALID(obj)) {
		return -EFTYPE;
	}

	int ret;
	NRF_CLOUD_OBJ_DEFINE(pvt_obj, obj->type);

	/* Add the app ID, message type, and timestamp */
	ret = nrf_cloud_obj_msg_init(obj, NRF_CLOUD_JSON_APPID_VAL_GNSS,
				     NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	if (ret) {
		goto cleanup;
	}

	if (gnss->ts_ms > NRF_CLOUD_NO_TIMESTAMP) {
		/* Add timestamp to message object */
		ret = nrf_cloud_obj_ts_add(obj, gnss->ts_ms);
		if (ret) {
			ret = -ENOMEM;
			goto cleanup;
		}
	}

	/* Add the specified GNSS data type */
	switch (gnss->type) {
	case NRF_CLOUD_GNSS_TYPE_MODEM_PVT:
	case NRF_CLOUD_GNSS_TYPE_PVT:

		ret = nrf_cloud_obj_init(&pvt_obj);
		if (ret) {
			goto cleanup;
		}

		/* Encode PVT data */
		if (gnss->type == NRF_CLOUD_GNSS_TYPE_PVT) {
			ret = nrf_cloud_obj_pvt_add(&pvt_obj, &gnss->pvt);
		} else {
#if defined(CONFIG_NRF_MODEM)
			ret = nrf_cloud_obj_modem_pvt_add(&pvt_obj, gnss->mdm_pvt);
#else
			ret = -ENOSYS;
#endif
		}

		if (ret) {
			goto cleanup;
		}

		/* Add the data object to the message */
		ret = nrf_cloud_obj_object_add(obj, NRF_CLOUD_JSON_DATA_KEY, &pvt_obj, false);
		if (ret) {
			goto cleanup;
		}

		/* The pvt object now belongs to the gnss msg object */
		pvt_obj.json = NULL;

		break;

	case NRF_CLOUD_GNSS_TYPE_MODEM_NMEA:
	case NRF_CLOUD_GNSS_TYPE_NMEA:
		const char *nmea = NULL;

		if (gnss->type == NRF_CLOUD_GNSS_TYPE_MODEM_NMEA) {
#if defined(CONFIG_NRF_MODEM)
			if (gnss->mdm_nmea) {
				nmea = gnss->mdm_nmea->nmea_str;
			}
#endif
		} else {
			nmea = gnss->nmea.sentence;
		}

		if (nmea == NULL) {
			ret = -EINVAL;
			goto cleanup;
		}

		if (memchr(nmea, '\0', NRF_MODEM_GNSS_NMEA_MAX_LEN) == NULL) {
			ret = -EFBIG;
			goto cleanup;
		}

		/* Add the NMEA sentence to the message */
		ret = nrf_cloud_obj_str_add(obj, NRF_CLOUD_JSON_DATA_KEY, nmea, false);
		if (ret) {
			ret = -ENOMEM;
			goto cleanup;
		}

		break;
	default:
		ret = -EPROTO;
		goto cleanup;
	}

	return 0;

cleanup:
	(void)nrf_cloud_obj_free(&pvt_obj);
	(void)nrf_cloud_obj_free(obj);
	return ret;
}

int nrf_cloud_obj_pvt_add(struct nrf_cloud_obj *const obj,
	const struct nrf_cloud_gnss_pvt *const pvt)
{
	if (!pvt || !obj) {
		return -EINVAL;
	}
	if (!NRF_CLOUD_OBJ_TYPE_VALID(obj)) {
		return -EFTYPE;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!obj->json) {
			return -ENOENT;
		}

		return nrf_cloud_pvt_data_encode(pvt, obj->json);
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
}

int nrf_cloud_obj_modem_pvt_add(struct nrf_cloud_obj *const obj,
	const struct nrf_modem_gnss_pvt_data_frame *const mdm_pvt)
{
#if defined(CONFIG_NRF_MODEM)
	if (!obj) {
		return -EINVAL;
	}
	if (!NRF_CLOUD_OBJ_TYPE_VALID(obj)) {
		return -EFTYPE;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (!obj->json) {
			return -ENOENT;
		}

		return nrf_cloud_modem_pvt_data_encode(mdm_pvt, obj->json);
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		break;
	}

	return -ENOTSUP;
#else
	ARG_UNUSED(obj);
	ARG_UNUSED(mdm_pvt);
	return -ENOSYS;
#endif
}

int nrf_cloud_obj_location_request_create(struct nrf_cloud_obj *const obj,
					  const struct lte_lc_cells_info *const cells_inf,
					  const struct wifi_scan_info *const wifi_inf,
					  const bool request_loc)
{
	if ((!cells_inf && !wifi_inf) || !obj) {
		return -EINVAL;
	}
	if (!cells_inf && (wifi_inf->cnt < NRF_CLOUD_LOCATION_WIFI_AP_CNT_MIN)) {
		return -EDOM;
	}
	if (!NRF_CLOUD_OBJ_TYPE_VALID(obj)) {
		return -EFTYPE;
	}

	int err;
	NRF_CLOUD_OBJ_DEFINE(data_obj, obj->type);

	/* Init obj with the appId and msgType */
	err = nrf_cloud_obj_msg_init(obj, NRF_CLOUD_JSON_APPID_VAL_LOCATION,
				     NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	if (err) {
		goto cleanup;
	}

	/* By default, nRF Cloud will send the location to the device */
	if (!request_loc &&
	    nrf_cloud_obj_num_add(obj, NRF_CLOUD_LOCATION_KEY_DOREPLY, 0, true)) {
		err = -ENOMEM;
		goto cleanup;
	}

	err = nrf_cloud_obj_init(&data_obj);
	if (err) {
		goto cleanup;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		if (cells_inf) {
			err = nrf_cloud_cell_pos_req_json_encode(cells_inf, data_obj.json);
			if ((err == -ENODATA) && (wifi_inf != NULL)) {
				LOG_WRN("No GCI cells, excluding cellular data from request");
			} else if (err) {
				LOG_ERR("Failed to add cell info to location request, error: %d",
					err);
				goto cleanup;
			}
		}

		if (wifi_inf) {
			err = nrf_cloud_wifi_req_json_encode(wifi_inf, data_obj.json);
			if (err) {
				LOG_ERR("Failed to add Wi-Fi info to location request, error: %d",
					err);
				goto cleanup;
			}
		}

		/* Add data object to the location request object */
		err = nrf_cloud_obj_object_add(obj, NRF_CLOUD_JSON_DATA_KEY, &data_obj, false);
		if (err) {
			goto cleanup;
		}

		/* The data object now belongs to the location request object */
		data_obj.json = NULL;

		break;
	}
	case NRF_CLOUD_OBJ_TYPE_CBOR:
	default:
		err = -ENOTSUP;
		goto cleanup;
	}

	return 0;

cleanup:
	(void)nrf_cloud_obj_free(&data_obj);
	(void)nrf_cloud_obj_free(obj);
	return err;
}

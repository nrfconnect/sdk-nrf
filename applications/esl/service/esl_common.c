/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/ead.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include "dk_buttons_and_leds.h"

#include "esl_common.h"
#include "esl_internal.h"

#if defined(CONFIG_BT_ESL_CLIENT)
#define ESL_COMMON_LOG_LEVEL CONFIG_BT_ESL_CLIENT_LOG_LEVEL
#elif defined(CONFIG_BT_ESL)
#define ESL_COMMON_LOG_LEVEL CONFIG_BT_ESL_LOG_LEVEL
#else
#define ESL_COMMON_LOG_LEVEL 3
#endif

LOG_MODULE_REGISTER(common_esl, ESL_COMMON_LOG_LEVEL);

void print_hex(const uint8_t *hex, size_t len)
{
	for (size_t idx = 0; idx < len; idx++) {
		printk("%02x", hex[idx]);
	}

	printk("\n");
}
void unpack_repeat_type_duration(uint8_t *data, uint8_t *repeat_type, uint16_t *repeat_duration)
{
	*repeat_type = data[0] & 0x1;
	*repeat_duration = (((data[0] & 0xfe) >> 1) | (data[1] << 8));
}

void pack_repeat_type_duration(uint8_t *data, uint8_t repeat_type, uint16_t repeat_duration)
{
	uint16_t val = (repeat_type & 0x01) | ((repeat_duration & REPEATS_DURATION_MAX) << 1);

	memcpy(data, &val, 2);
}

void dump_disp_inf(uint8_t *raw, uint16_t len, uint8_t *displayed_imgs)
{
	uint16_t width;
	uint16_t height;
	uint8_t type;

	if ((len % DISP_INF_SIZE) != 0) {
		LOG_ERR("disp_inf is not aligned");
		return;
	}

	for (uint16_t idx = 0; idx < len; idx += DISP_INF_SIZE) {
		width = sys_get_le16(&raw[idx]);
		height = sys_get_le16(&raw[idx + 2]);
		type = raw[idx + 4];
		if (displayed_imgs != NULL) {
			printk("display idx 0x%02x, width %d height %d type %d, img 0x%02x\n",
			       (idx / ESL_DISP_CHARA_LEN), width, height, type,
			       displayed_imgs[(idx / ESL_DISP_CHARA_LEN)]);
		} else {
			printk("display idx 0x%02x, width %d height %d type %d\n",
			       (idx / ESL_DISP_CHARA_LEN), width, height, type);
		}
	}
}

static uint8_t *mono_color_to_string(uint8_t color)
{
	if (CHECK_BIT(color, ESL_LED_BLUE_HI_BIT) || CHECK_BIT(color, ESL_LED_BLUE_LO_BIT)) {
		return "Blue";
	} else if (CHECK_BIT(color, ESL_LED_GREEN_HI_BIT) ||
		   CHECK_BIT(color, ESL_LED_GREEN_LO_BIT)) {
		return "Green";
	} else {
		return "Red";
	}
}

static void dump_led_inf(uint8_t led_type)
{
	uint8_t type;
	uint8_t color;

	type = (led_type & 0xC0) >> 6;
	color = (led_type & 0x3f);
	if (type == ESL_LED_SRGB) {
		printk("type sRGB color 0x%02x\n", color);
	} else if (type == ESL_LED_MONO) {
		printk("type Mono color 0x%02x:%s\n", color, mono_color_to_string(color));
	} else {
		printk("type RFU\n");
	}
}

void dump_led_infs(uint8_t *raw, uint16_t len)
{
	for (uint16_t idx = 0; idx < len; idx++) {
		printk("led idx %d, ", idx);
		dump_led_inf(raw[idx]);
	}
}

uint8_t parse_sensor_info_from_raw(struct esl_sensor_inf *sensors, uint8_t *raw, uint16_t len)
{
	uint8_t count = 0;
	uint16_t idx;

	for (idx = 0; idx < len;) {
		sensors[count].size = raw[idx];
		/* Mesh property ID */
		if (raw[idx] == 0) {
			sensors[count].property_id = raw[idx + 2] << 8 | raw[idx + 1];
			idx += 3;
			/* vendor_specific_sensor */
		} else if (raw[idx] == 1) {
			sensors[count].vendor_specific.company_id =
				raw[idx + 2] << 8 | raw[idx + 1];
			sensors[count].vendor_specific.sensor_code =
				raw[idx + 4] << 8 | raw[idx + 3];
			idx += 5;
		}
		count++;
	}
	return count;
}

void dump_sensor_inf(const struct esl_sensor_inf *sensors, uint8_t count)
{
	for (uint8_t idx = 0; idx < count; idx++) {
		/* Mesh property ID */
		if (sensors[idx].size == 0) {
			printk("Sensor %d Mesh property ID Sensor type: 0x%04x\n", idx,
			       sensors[idx].property_id);
			/* vendor_specific_sensor */
		} else if (sensors[idx].size == 1) {
			printk("Sensor %d vendor_specific_sensor Company ID: 0x%04x Sensor Code: "
			       "0x%04x\n",
			       idx, sensors[idx].vendor_specific.company_id,
			       sensors[idx].vendor_specific.sensor_code);
		}
	}
}

void dump_sensor_data(const uint8_t *raw, uint16_t len)
{
	uint8_t sensor_data_len = (raw[0] & 0xf0) >> 4;

	if (sensor_data_len != (len - ESL_RESPONSE_SENSOR_HEADER)) {
		LOG_WRN("sensor_data length not match %d %d\n", sensor_data_len,
			(len - ESL_RESPONSE_SENSOR_HEADER));
	}
	printk("#SENSOR:1,0x%02x,#DATA:0x", raw[1]);
	print_hex((uint8_t *)(raw + 2), sensor_data_len);
}

void print_binary(uint64_t hex, uint8_t len)
{
	printk("print_binary %llu\n", hex);
	for (int8_t idx = (len - 1); idx >= 0; idx--) {
		printk("%u ", ((hex >> idx) & 1) ? 1 : 0);
	}
	printk("\n");
}

void print_basic_state(atomic_t state)
{
	printk("SERVICE_NEEDED:%s\n",
	       atomic_test_bit(&state, ESL_SERVICE_NEEDED_BIT) ? "on" : "off");
	printk("SYNCHRONIZED:%s\n", atomic_test_bit(&state, ESL_SYNCHRONIZED) ? "on" : "off");
	printk("ACTIVE_LED:%s\n", atomic_test_bit(&state, ESL_ACTIVE_LED) ? "on" : "off");
	printk("PENDING_LED_UPDATE:%s\n",
	       atomic_test_bit(&state, ESL_PENDING_LED_UPDATE) ? "on" : "off");
	printk("PENDING_DISPLAY_UPDATE:%s\n",
	       atomic_test_bit(&state, ESL_PENDING_DISPLAY_UPDATE) ? "on" : "off");
	printk("BASIC_RFU:%s\n", atomic_test_bit(&state, ESL_BASIC_RFU) ? "on" : "off");
}

uint16_t dump_ots_client_handles(char *buf, struct bt_ots_client otc)
{
	uint16_t offset = 0;

	offset += sprintf(buf, "start_handle 0x%04x\n", otc.start_handle);
	offset += sprintf(buf + offset, "end_handle 0x%04x\n", otc.end_handle);
	offset += sprintf(buf + offset, "obj_feat 0x%04x\n", otc.feature_handle);
	offset += sprintf(buf + offset, "obj_name 0x%04x\n", otc.obj_name_handle);
	offset += sprintf(buf + offset, "obj_type 0x%04x\n", otc.obj_type_handle);
	offset += sprintf(buf + offset, "obj_size 0x%04x\n", otc.obj_size_handle);
	offset += sprintf(buf + offset, "obj_prop 0x%04x\n", otc.obj_properties_handle);
	offset += sprintf(buf + offset, "obj_created 0x%04x\n", otc.obj_created_handle);
	offset += sprintf(buf + offset, "obj_modified 0x%04x\n", otc.obj_modified_handle);
	offset += sprintf(buf + offset, "obj_id 0x%04x\n", otc.obj_id_handle);
	offset += sprintf(buf + offset, "obj_ocap 0x%04x\n", otc.oacp_handle);
	offset += sprintf(buf + offset, "obj_olcp 0x%04x\n", otc.olcp_handle);
	return offset;
}

bool check_ble_connection(struct bt_conn *conn)
{
	int err;
	struct bt_conn_info info;

	err = bt_conn_get_info(conn, &info);
	if (info.state == BT_CONN_STATE_CONNECTED && !err) {
		return true;
	}

	return false;
}

uint8_t *esl_state_to_string(uint8_t state)
{
	switch (state) {
	case SM_UNASSOCIATED:
		return "UNASSOCIATED";
	case SM_CONFIGURING:
		return "CONFIGURING";
	case SM_CONFIGURED:
		return "CONFIGURED";
	case SM_SYNCHRONIZED:
		return "SYNCHRONIZED";
	case SM_UPDATING:
		return "UPDATING";
	case SM_UNSYNCHRONIZED:
		return "UNSYNCHRONIZED";
	default:
		return "UNKNOWN";
	}
}

uint8_t *esl_rcp_error_to_string(enum ESL_RSP_ERR_CODE err_code)
{
	switch (err_code) {
	case ERR_RFU:
		return "RFU";
	case ERR_UNSPEC:
		return "Unspecified Error";
	case ERR_INV_OPCODE:
		return "Invalid Opcode";
	case ERR_INV_STATE:
		return "Invalid State";
	case ERR_INV_IMG_IDX:
		return "Invalid Image_Index";
	case ERR_IMG_NOT_AVAIL:
		return "Image Not Available";
	case ERR_INV_PARAMS:
		return "Invalid Parameter(s)";
	case ERR_CAP_LIMIT:
		return "Capacity Limit";
	case ERR_INSUFF_BAT:
		return "Insufficient Battery";
	case ERR_INSUFF_RES:
		return "Insufficient Resources";
	case ERR_RETRY:
		return "Retry";
	case ERR_QUEUE_FULL:
		return "Queue Full";
	case ERR_IMPLAUSIBLE_ABS:
		return "Implausible Absolute Time";
	default:
		if (err_code >= 0x0d && err_code <= 0xef) {
			return "RFU";
		} else if (err_code >= 0xf0 && err_code <= 0xff) {
			return "Vendor-specific Error";
		} else {
			return "UNKNOWN";
		}
	}
}

int ble_ctrl_version_get(uint16_t *ctrl_version)
{
	int ret;
	struct net_buf *rsp;

	ret = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, &rsp);
	if (ret) {
		return ret;
	}

	struct bt_hci_rp_read_local_version_info *rp = (void *)rsp->data;

	*ctrl_version = sys_le16_to_cpu(rp->hci_revision);

	net_buf_unref(rsp);

	return 0;
}

int esl_generate_ad_data_header(uint8_t *randomizer, uint8_t *ad_data, uint8_t esl_payload_len)
{
	int8_t output_len = 0;

	CHECKIF(randomizer == NULL) {
		LOG_ERR("randomizer is NULL");
		return -EINVAL;
	}

	CHECKIF(ad_data == NULL) {
		LOG_ERR("ad_data is NULL");
		return -EINVAL;
	}

	CHECKIF(esl_payload_len == 0) {
		LOG_ERR("esl_payload_len is 0");
		return -EINVAL;
	}

	memcpy(ad_data, randomizer, EAD_RANDOMIZER_LEN);
	ad_data[ESL_ENCRYTPTED_DATA_LEN_IDX] = esl_payload_len + 1;
	ad_data[ESL_ENCRYTPTED_DATA_LEN_IDX + 1] = BT_DATA_ESL;
	output_len = EAD_RANDOMIZER_LEN + AD_HEADER_LEN + esl_payload_len;
	LOG_DBG("payload_data_len %d output_len %d", esl_payload_len, output_len);
	LOG_HEXDUMP_DBG(ad_data, output_len, "Generated ESL plain AD data");

	return output_len;
}

inline void esl_generate_nonce(uint8_t *randomizer, uint8_t *iv, uint8_t *nonce)
{
	memcpy(nonce, randomizer, EAD_RANDOMIZER_LEN);
	memcpy((nonce + EAD_RANDOMIZER_LEN), iv, EAD_IV_LEN);
}

void esl_init_randomizer(uint8_t *randomizer)
{
	uint8_t err = 0;

	err = bt_rand(randomizer, EAD_RANDOMIZER_LEN);
	if (err != 0) {
		LOG_ERR("generate RANDOMIZER failed\n");
	}

	/** The directionBit of the CCM nonce shall be set to the most significant bit of the
	 *  Randomizer field.
	 **/
	randomizer[4] |= 1 << ESL_RAMDOMIZER_DIRECTION_BIT;
}

/* Encrypted Advertising Data should increase everytime new packet send*/
void esl_update_randomizer(uint8_t *randomizer, struct bt_esl_key_material key, uint8_t *nonce)
{
	esl_init_randomizer(randomizer);
	esl_generate_nonce(randomizer, key.key.iv, nonce);

	LOG_HEXDUMP_DBG(randomizer, EAD_RANDOMIZER_LEN, "current Randomizer");
	LOG_HEXDUMP_DBG(nonce, EAD_NONCE_LEN, "current Nonce");
}

uint8_t esl_compose_ad_data(uint8_t *pa_ad_data, uint8_t *esl_payload, uint8_t esl_payload_len,
			    uint8_t *randomizer, struct bt_esl_key_material key)
{
	int err = 0;
	uint8_t out_len;
	uint8_t nonce[EAD_NONCE_LEN];

	LOG_HEXDUMP_DBG(esl_payload, esl_payload_len, "input payload");
	esl_update_randomizer(randomizer, key, nonce);
	memcpy((pa_ad_data + AD_HEADER_LEN + EAD_RANDOMIZER_LEN + AD_HEADER_LEN), esl_payload,
	       esl_payload_len);
	(void)esl_generate_ad_data_header(randomizer, pa_ad_data + AD_HEADER_LEN, esl_payload_len);
	esl_payload_len += AD_HEADER_LEN;
	err = bt_ead_encrypt(key.key.session_key, key.key.iv,
			     pa_ad_data + AD_HEADER_LEN + EAD_RANDOMIZER_LEN, esl_payload_len,
			     pa_ad_data + AD_HEADER_LEN);
	if (err != 0) {
		LOG_ERR("Error during encryption.\n");
	}

	out_len = SYNC_PKK_MIN_LEN + esl_payload_len;
	pa_ad_data[0] = out_len - sizeof(uint8_t);
	pa_ad_data[1] = BT_DATA_ENCRYPTED_AD_DATA;
#if defined(DEBUG_DECRYPT)
	uint8_t decrypted_text[out_len - AD_HEADER_LEN];

	err = bt_ead_decrypt(key.key.session_key, key.key.iv, pa_ad_data + AD_HEADER_LEN,
			     out_len - AD_HEADER_LEN, decrypted_text);
	LOG_INF("decrypt err %d", err);
	LOG_HEXDUMP_INF(decrypted_text, esl_payload_len, "decrypt");
#endif
	if (IS_ENABLED(CONFIG_BT_ESL_LED_INDICATION) && IS_ENABLED(CONFIG_BT_ESL_CLIENT)) {
		dk_set_led(DK_LED3, 1);
	}

	return out_len;
}

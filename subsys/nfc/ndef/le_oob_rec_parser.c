/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <nfc/ndef/le_oob_rec_parser.h>
#include <nfc/ndef/payload_type_common.h>

LOG_MODULE_REGISTER(nfc_ndef_le_oob_rec_parser,
		    CONFIG_NFC_NDEF_LE_OOB_REC_PARSER_LOG_LEVEL);

/* Size of AD fields. */
#define AD_TYPE_FIELD_SIZE 1UL
#define AD_LEN_FIELD_SIZE 1UL

/* Incorrect Value of AD Structure Length field. */
#define AD_LEN_INC_VAL  1UL

/* Size of terminator character required for BLE Device name. */
#define NULL_TERMINATOR_SIZE 1UL

static uint8_t *memory_allocate(struct net_buf_simple *mem_alloc,
			     uint32_t alloc_size)
{
	if (net_buf_simple_tailroom(mem_alloc) < alloc_size) {
		return NULL;
	}

	return net_buf_simple_add(mem_alloc, alloc_size);
}

static int ble_device_address_decode(const uint8_t *payload_buf,
				     uint32_t payload_len,
				     bt_addr_le_t *addr)
{
	if (!addr) {
		return -ENOMEM;
	}

	if (payload_len != sizeof(*addr)) {
		return -EINVAL;
	}

	memcpy(&addr->a, payload_buf, sizeof(addr->a));
	payload_buf += sizeof(addr->a);

	addr->type = *payload_buf;

	return 0;
}

static int le_role_decode(const uint8_t *payload_buf, uint32_t payload_len,
			  enum nfc_ndef_le_oob_rec_le_role *le_role)
{
	if (!le_role) {
		return -ENOMEM;
	}

	if (payload_len != sizeof(*le_role)) {
		return -EINVAL;
	}

	*le_role = *payload_buf;

	return 0;
}

static int tk_value_decode(const uint8_t *payload_buf, uint32_t payload_len,
			   uint8_t *tk_value)
{
	if (!tk_value) {
		return -ENOMEM;
	}

	if (payload_len != NFC_NDEF_LE_OOB_REC_TK_LEN) {
		return -EINVAL;
	}

	memcpy(tk_value, payload_buf, NFC_NDEF_LE_OOB_REC_TK_LEN);

	return 0;
}

static int le_sc_confirm_value_decode(const uint8_t *payload_buf,
				      uint32_t payload_len,
				      struct bt_le_oob_sc_data *le_sc_data)
{
	if (!le_sc_data) {
		return -ENOMEM;
	}

	if (payload_len != sizeof(le_sc_data->c)) {
		return -EINVAL;
	}

	memcpy(le_sc_data->c, payload_buf, sizeof(le_sc_data->c));

	return 0;
}

static int le_sc_random_value_decode(const uint8_t *payload_buf,
				     uint32_t payload_len,
				     struct bt_le_oob_sc_data *le_sc_data)
{
	if (!le_sc_data) {
		return -ENOMEM;
	}

	if (payload_len != sizeof(le_sc_data->r)) {
		return -EINVAL;
	}

	memcpy(le_sc_data->r, payload_buf, sizeof(le_sc_data->r));

	return 0;
}

static int appearance_decode(const uint8_t *payload_buf, uint32_t payload_len,
			     uint16_t *appearance)
{
	if (!appearance) {
		return -ENOMEM;
	}

	if (payload_len != sizeof(*appearance)) {
		return -EINVAL;
	}

	*appearance = sys_get_le16(payload_buf);

	return 0;
}

static int flags_decode(const uint8_t *payload_buf, uint32_t payload_len,
			uint8_t *flags)
{
	if (!flags) {
		return -ENOMEM;
	}

	if (payload_len != sizeof(*flags)) {
		return -EINVAL;
	}

	*flags = payload_buf[0];

	return 0;
}

static int nfc_ndef_le_oob_rec_payload_parse(const uint8_t *payload_buf,
					     uint32_t payload_len,
					     uint8_t *result_buf,
					     uint32_t *result_buf_len)
{
	int err = 0;
	const uint32_t total_buf_size = *result_buf_len;
	uint32_t index = 0;
	struct nfc_ndef_le_oob_rec_payload_desc *le_oob_rec_desc;
	struct net_buf_simple mem_alloc;

	if (!payload_buf) {
		return -EINVAL;
	}

	/* Initialize memory allocator with buffer memory block and its size. */
	net_buf_simple_init_with_data(&mem_alloc, result_buf, total_buf_size);
	net_buf_simple_reset(&mem_alloc);

	/* Allocate memory for LE OOB Record descriptor and initialize it. */
	le_oob_rec_desc = (struct nfc_ndef_le_oob_rec_payload_desc *)
		memory_allocate(&mem_alloc, sizeof(*le_oob_rec_desc));
	if (!le_oob_rec_desc) {
		return -ENOMEM;
	}
	memset(le_oob_rec_desc, 0, sizeof(*le_oob_rec_desc));

	/* Parse record payload. */
	while (index < payload_len) {
		uint8_t ad_len;

		ad_len = payload_buf[index];
		index += AD_LEN_FIELD_SIZE;

		if ((ad_len == AD_LEN_INC_VAL) ||
		    (index + ad_len > payload_len)) {
			return -EINVAL;
		}

		uint8_t ad_type = payload_buf[index];
		const uint8_t *ad_data =
			&payload_buf[index + AD_TYPE_FIELD_SIZE];
		uint8_t ad_data_len = ad_len - AD_TYPE_FIELD_SIZE;

		/* Decode AD payload. */
		switch (ad_type) {
		/* Mandatory fields. */
		case BT_DATA_LE_BT_DEVICE_ADDRESS:
			le_oob_rec_desc->addr = (bt_addr_le_t *)
				memory_allocate(&mem_alloc,
					sizeof(*le_oob_rec_desc->addr));
			err = ble_device_address_decode(ad_data, ad_data_len,
				le_oob_rec_desc->addr);
			break;
		case BT_DATA_LE_ROLE:
			le_oob_rec_desc->le_role = (enum nfc_ndef_le_oob_rec_le_role *)
				memory_allocate(&mem_alloc,
					sizeof(*le_oob_rec_desc->le_role));
			err = le_role_decode(ad_data, ad_data_len,
				le_oob_rec_desc->le_role);
			break;

		case BT_DATA_SM_TK_VALUE:
			le_oob_rec_desc->tk_value = memory_allocate(&mem_alloc,
								    NFC_NDEF_LE_OOB_REC_TK_LEN);
			err = tk_value_decode(ad_data, ad_data_len,
					      le_oob_rec_desc->tk_value);
			break;

		/* Optional fields */
		case BT_DATA_LE_SC_CONFIRM_VALUE:
		case BT_DATA_LE_SC_RANDOM_VALUE:
			if (!le_oob_rec_desc->le_sc_data) {
				le_oob_rec_desc->le_sc_data = (struct bt_le_oob_sc_data *)
					memory_allocate(&mem_alloc,
						sizeof(*le_oob_rec_desc->le_sc_data));
			}

			if (ad_type == BT_DATA_LE_SC_CONFIRM_VALUE) {
				err = le_sc_confirm_value_decode(ad_data,
					ad_data_len,
					le_oob_rec_desc->le_sc_data);
			} else {
				err = le_sc_random_value_decode(ad_data,
					ad_data_len,
					le_oob_rec_desc->le_sc_data);
			}
			break;
		case BT_DATA_GAP_APPEARANCE:
			le_oob_rec_desc->appearance = (uint16_t *)
				memory_allocate(&mem_alloc,
					sizeof(*le_oob_rec_desc->appearance));
			err = appearance_decode(ad_data, ad_data_len,
				le_oob_rec_desc->appearance);
			break;
		case BT_DATA_FLAGS:
			le_oob_rec_desc->flags = (uint8_t *)
				memory_allocate(&mem_alloc,
					sizeof(*le_oob_rec_desc->flags));
			err = flags_decode(ad_data, ad_data_len,
				le_oob_rec_desc->flags);
			break;
		case BT_DATA_NAME_SHORTENED:
		case BT_DATA_NAME_COMPLETE:
		{
			uint8_t *name;

			name = memory_allocate(&mem_alloc, ad_data_len +
					       NULL_TERMINATOR_SIZE);
			if (name) {
				memcpy(name, ad_data, ad_data_len);
				name[ad_data_len] = 0;
			} else {
				return -ENOMEM;
			}
			le_oob_rec_desc->local_name = (const char *) name;
			break;
		}
		default:
			break;
		}

		if (err) {
			return err;
		}

		index += ad_len;
	}

	*result_buf_len =
		(total_buf_size - net_buf_simple_tailroom(&mem_alloc));

	return 0;
}

bool nfc_ndef_le_oob_rec_check(const struct nfc_ndef_record_desc *rec_desc)
{
	if (!rec_desc || !rec_desc->type) {
		return false;
	}

	if (rec_desc->tnf != TNF_MEDIA_TYPE) {
		return false;
	}

	if (rec_desc->type_length != sizeof(nfc_ndef_le_oob_rec_type_field)) {
		return false;
	}

	if (memcmp(rec_desc->type, nfc_ndef_le_oob_rec_type_field,
		   sizeof(nfc_ndef_le_oob_rec_type_field)) != 0) {
		return false;
	}

	return true;
}

int nfc_ndef_le_oob_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			      uint8_t *result_buf, uint32_t *result_buf_len)
{
	int err;
	struct nfc_ndef_bin_payload_desc *payload_desc;

	if (!result_buf || !result_buf_len) {
		return -EINVAL;
	}

	if (!nfc_ndef_le_oob_rec_check(rec_desc)) {
		return -EINVAL;
	}

	if (rec_desc->payload_constructor !=
		(payload_constructor_t) nfc_ndef_bin_payload_memcopy) {
		return -ENOTSUP;
	}

	payload_desc = (struct nfc_ndef_bin_payload_desc *)
		rec_desc->payload_descriptor;

	err = nfc_ndef_le_oob_rec_payload_parse(payload_desc->payload,
						payload_desc->payload_length,
						result_buf, result_buf_len);
	return err;
}

static const char * const le_role_strings[] = {
	"Only Peripheral Role supported",
	"Only Central Role supported",
	"Both Roles supported - Peripheral Role preferred",
	"Both Roles supported - Central Role preferred",
};

void nfc_ndef_le_oob_rec_printout(
	const struct nfc_ndef_le_oob_rec_payload_desc *le_oob_rec_desc)
{
	LOG_INF("NDEF LE OOB Record payload");

	if (le_oob_rec_desc->addr) {
		static char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(le_oob_rec_desc->addr, addr_str,
			sizeof(addr_str));
		LOG_INF("\tDevice Address:\t%s", addr_str);
	}

	if (le_oob_rec_desc->local_name) {
		LOG_INF("\tLocal Name:\t%s", le_oob_rec_desc->local_name);
	}

	if (le_oob_rec_desc->le_role) {
		enum nfc_ndef_le_oob_rec_le_role le_role;
		const char *le_role_str;

		le_role = *le_oob_rec_desc->le_role;
		if (le_role < NFC_NDEF_LE_OOB_REC_LE_ROLE_OPTIONS_NUM) {
			le_role_str = le_role_strings[le_role];
		} else {
			le_role_str = "Unknown Role";
		}
		LOG_INF("\tLE Role:\t%s", le_role_str);
	}

	if (le_oob_rec_desc->appearance) {
		LOG_INF("\tAppearance:\t%d", *le_oob_rec_desc->appearance);
	}

	if (le_oob_rec_desc->flags) {
		LOG_INF("\tFlags:\t\t0x%02X", *le_oob_rec_desc->flags);
	}

	if (le_oob_rec_desc->tk_value) {
		LOG_HEXDUMP_INF(le_oob_rec_desc->tk_value,
				NFC_NDEF_LE_OOB_REC_TK_LEN,
				"\tTK Value:");
	}

	if (le_oob_rec_desc->le_sc_data) {
		LOG_HEXDUMP_INF(le_oob_rec_desc->le_sc_data->c,
				sizeof(le_oob_rec_desc->le_sc_data->c),
				"\tLE SC Confirm Value:");
		LOG_HEXDUMP_INF(le_oob_rec_desc->le_sc_data->r,
				sizeof(le_oob_rec_desc->le_sc_data->r),
				"\tLE SC Random Value:");
	}
}

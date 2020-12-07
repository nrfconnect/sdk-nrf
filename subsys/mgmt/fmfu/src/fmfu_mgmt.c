/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <logging/log.h>
#include <mgmt/mgmt.h>
#include <nrfx_ipc.h>
#include <nrf_modem_full_dfu.h>
#include <mgmt/fmfu_mgmt.h>
#include "fmfu_mgmt_internal.h"
#include <cborattr/cborattr.h>
#include <stats/stats.h>

LOG_MODULE_REGISTER(mgmt_fmfu, CONFIG_MGMT_FMFU_LOG_LEVEL);

/* Full modem update mgmt spesific SMP return codes */
#define MGMT_ERR_EMODEM_INVALID_COMMAND 200
#define MGMT_ERR_EMODEM_FAULT           201

/* Full modem IPC failures*/
#define FMFU_ERR_COMMAND_FAILED -3
#define FMFU_ERR_COMMAND_FAULT -4

#define FIRMWARE_SHA_LENGTH 32

/* Struct used for parsing upload requests. */
struct firmware_packet {
	uint8_t data[SMP_PACKET_MTU];
	uint32_t target_address;
	uint32_t data_len;
};

static int unpack(struct mgmt_ctxt *ctxt, struct firmware_packet *packet,
		  bool *whole_file_received)
{
	static uint32_t file_length;
	/*
	 * These data types are long long so that we don't have to typecast
	 * and get compiler warnings when using them as assign data in
	 * cbor_attr_t
	 */
	uint64_t offset;
	uint64_t read_file_length;
	size_t data_len;
	size_t firmware_sha_len;
	uint8_t firmware_sha[FIRMWARE_SHA_LENGTH];

	CborError cbor_err = 0;
	uint64_t fw_target_address = 0;
	/* Description/skeleton of CBOR object */
	const struct cbor_attr_t off_attr[] = {
		[0] = {
			.attribute = "data",
			.type = CborAttrByteStringType,
			.addr.bytestring.data = packet->data,
			.addr.bytestring.len = &data_len,
			.len = SMP_PACKET_MTU
		},
		[1] = {
			.attribute = "len",
			.type = CborAttrUnsignedIntegerType,
			.addr.uinteger = &read_file_length,
			.nodefault = true
		},
		[2] = {
			.attribute = "off",
			.type = CborAttrUnsignedIntegerType,
			.addr.uinteger = &offset,
			.nodefault = true
		},
		/* Unused but needed for parsing the CBOR*/
		[3] = {
			.attribute = "sha",
			.type = CborAttrByteStringType,
			.addr.bytestring.data = firmware_sha,
			.addr.bytestring.len = &firmware_sha_len,
			.len = FIRMWARE_SHA_LENGTH,
		},
		[4] = {
			.attribute = "addr",
			.type = CborAttrUnsignedIntegerType,
			.addr.uinteger = &fw_target_address,
			.nodefault = true
		},
		/* End of array */
		[5] = { 0 },
	};

	cbor_err = cbor_read_object(&ctxt->it, off_attr);
	if (cbor_err) {
		return -MGMT_ERR_EINVAL;
	}

	if (offset == 0) {
		file_length = (uint32_t)read_file_length;
	}

	packet->target_address  = (uint32_t)fw_target_address;
	packet->data_len = (uint32_t)data_len;
	uint32_t new_offset = (uint32_t)offset + packet->data_len;
	*whole_file_received = (((uint32_t)offset + packet->data_len)
				== file_length);
	LOG_INF("Writing %d/%d bytes", new_offset, file_length);

	return offset + packet->data_len;
}

static int get_mgmt_err_from_modem_ret_err(int err)
{
	switch (err) {
	case FMFU_ERR_COMMAND_FAILED:
		LOG_ERR("RPC_COMMAND_FAILED");
		return MGMT_ERR_EMODEM_INVALID_COMMAND;
	case FMFU_ERR_COMMAND_FAULT:
		LOG_ERR("RPC_COMMAND_FAULT");
		return MGMT_ERR_EMODEM_FAULT;
	default:
		LOG_ERR("MODEM_BAD_STATE: %d", err);
		return MGMT_ERR_EBADSTATE;
	}
}

static int encode_response(struct mgmt_ctxt *ctx, uint32_t expected_offset)
{
	CborError err = 0;
	int status = 0;

	err |= cbor_encode_text_stringz(&ctx->encoder, "rc");
	err |= cbor_encode_int(&ctx->encoder, status);
	err |= cbor_encode_text_stringz(&ctx->encoder, "off");
	err |= cbor_encode_uint(&ctx->encoder, expected_offset);

	if (err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return 0;
}

static int fmfu_firmware_upload(struct mgmt_ctxt *ctx)
{
	static bool bootloader = true;

	int rc;
	bool whole_file_received;
	struct firmware_packet packet;
	uint32_t next_expected_offset;

	rc = unpack(ctx, &packet, &whole_file_received);
	if (rc < 0) {
		return rc;
	}

	next_expected_offset = rc;

	if (packet.data_len > 0) {
		if (bootloader) {
			rc = nrf_modem_full_dfu_bl_write(packet.data_len,
					packet.data);
			if (rc != 0) {
				LOG_ERR("Error in starting transfer");
				return get_mgmt_err_from_modem_ret_err(rc);
			}
		} else {
			LOG_DBG("next_offset: 0x%x, target_address: 0x%x,"
				"packet len: %d", next_expected_offset,
						  packet.target_address,
						  packet.data_len);

			rc = nrf_modem_full_dfu_fw_write(packet.target_address,
							 packet.data_len,
							 packet.data);
			if (rc != 0) {
				LOG_ERR("Error in writing data, rc: %d,"
					" errno: %d", rc, errno);
				return get_mgmt_err_from_modem_ret_err(
						rc);
			}
		}
	}

	if (whole_file_received) {
		rc = nrf_modem_full_dfu_apply();
		if (rc != 0) {
			LOG_ERR("Error in transfer_end");
			return get_mgmt_err_from_modem_ret_err(rc);
		}
		bootloader = false;
	}

	return encode_response(ctx, next_expected_offset);
}

static int fmfu_get_memory_hash(struct mgmt_ctxt *ctxt)
{
	uint64_t start;
	uint64_t end;
	struct nrf_modem_full_dfu_digest digest;

	/* We expect two variables: the start and end address of a memory region
	 * that we want to verify (obtain a hash for)
	 */
	const struct cbor_attr_t off_attr[] = {
		[0] = {
			.attribute = "start",
			.type = CborAttrUnsignedIntegerType,
			.addr.uinteger = &start,
			.nodefault = true
		},
		[1] = {
			.attribute = "end",
			.type = CborAttrUnsignedIntegerType,
			.addr.uinteger = &end,
			.nodefault = true
		},
		[2] = { 0 },
	};
	start = 0;
	end = 0;

	int rc = cbor_read_object(&ctxt->it, off_attr);

	if (rc || start == 0 || end == 0) {
		return MGMT_ERR_EINVAL;
	}

	rc = nrf_modem_full_dfu_digest(start, end - start, &digest);

	if (rc != 0) {
		LOG_ERR("nrf_fmfu_hash_get failed, errno: %d.", errno);
		LOG_ERR("start:%d end: %d\n.", (uint32_t)start, (uint32_t)end);
		return get_mgmt_err_from_modem_ret_err(rc);
	}

	/* Put the digest response in the response */
	CborError cbor_err = 0;

	cbor_err |= cbor_encode_text_stringz(&ctxt->encoder, "digest");
	cbor_err |= cbor_encode_byte_string(&ctxt->encoder,
			(uint8_t *)digest.data, NRF_MODEM_FULL_DFU_DIGEST_LEN);
	cbor_err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
	cbor_err |= cbor_encode_int(&ctxt->encoder, rc);
	if (cbor_err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return MGMT_ERR_EOK;
}

static const struct mgmt_handler mgmt_fmfu_handlers[] = {
	[FMFU_MGMT_ID_GET_HASH] = {
		.mh_read  = fmfu_get_memory_hash,
		.mh_write = fmfu_get_memory_hash,
	},
	[FMFU_MGMT_ID_UPLOAD] = {
		.mh_read  = NULL,
		.mh_write = fmfu_firmware_upload
	},
};

static struct mgmt_group fmfu_mgmt_group = {
	.mg_handlers = (struct mgmt_handler *)mgmt_fmfu_handlers,
	.mg_handlers_count = ARRAY_SIZE(mgmt_fmfu_handlers),
	.mg_group_id = (MGMT_GROUP_ID_PERUSER + 1),
};

int fmfu_mgmt_init(void)
{
	struct nrf_modem_full_dfu_digest digest;

	int err = nrf_modem_full_dfu_init(&digest);

	if (err != 0) {
		LOG_ERR("nrf_fmfu_init failed, errno: %d\n.", errno);
		return err;
	}
	mgmt_register_group(&fmfu_mgmt_group);
	return err;
}

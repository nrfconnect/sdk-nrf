/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <nrfx_ipc.h>
#include <nrf_modem_bootloader.h>
#include <mgmt/fmfu_mgmt.h>
#include "fmfu_mgmt_internal.h"
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <zephyr/stats/stats.h>

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

static int unpack(struct smp_streamer *ctxt, struct firmware_packet *packet,
		  bool *whole_file_received)
{
	zcbor_state_t *zsd = ctxt->reader->zs;
	static uint32_t file_length;
	/*
	 * These data types are long long so that we don't have to typecast
	 * and get compiler warnings when using them as assign data in
	 * cbor_attr_t
	 */
	uint64_t offset;
	uint64_t read_file_length;
	struct zcbor_string firmware_sha;
	struct zcbor_string data;
	uint64_t fw_target_address = 0;
	bool ok;
	size_t decoded = 0;
	struct zcbor_map_decode_key_val fw_upload_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(data, zcbor_bstr_decode, &data),
		ZCBOR_MAP_DECODE_KEY_VAL(len, zcbor_uint64_decode, &read_file_length),
		ZCBOR_MAP_DECODE_KEY_VAL(off, zcbor_uint64_decode, &offset),
		ZCBOR_MAP_DECODE_KEY_VAL(sha, zcbor_bstr_decode, &firmware_sha),
		ZCBOR_MAP_DECODE_KEY_VAL(addr, zcbor_uint64_decode, &fw_target_address)
	};

	ok = zcbor_map_decode_bulk(zsd, fw_upload_decode,
		ARRAY_SIZE(fw_upload_decode), &decoded) == 0;

	if (!ok || data.len > SMP_PACKET_MTU) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(packet->data, data.value, data.len);

	if (offset == 0) {
		file_length = (uint32_t)read_file_length;
	}

	packet->target_address  = (uint32_t)fw_target_address;
	packet->data_len = (uint32_t)data.len;
	uint32_t new_offset = (uint32_t)offset + packet->data_len;
	*whole_file_received = (((uint32_t)offset + packet->data_len)
				== file_length);
	LOG_INF("Writing %d/%d bytes", new_offset, file_length);

	return offset + packet->data_len;
}

static int encode_response(struct smp_streamer *ctxt, uint32_t expected_offset)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "rc")			&&
	     zcbor_int32_put(zse, MGMT_ERR_EOK)			&&
	     zcbor_tstr_put_lit(zse, "off")			&&
	     zcbor_int32_put(zse, expected_offset);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
}

static int fmfu_firmware_upload(struct smp_streamer *ctx)
{
	static bool bootloader = true;

	int rc;
	bool whole_file_received = false;
	struct firmware_packet packet;
	uint32_t next_expected_offset;

	rc = unpack(ctx, &packet, &whole_file_received);
	if (rc < 0) {
		return rc;
	}

	next_expected_offset = rc;

	if (packet.data_len > 0) {
		if (bootloader) {
			rc = nrf_modem_bootloader_bl_write(packet.data, packet.data_len);
			if (rc != 0) {
				LOG_ERR("Error in starting transfer");
				return MGMT_ERR_EBADSTATE;
			}
		} else {
			LOG_DBG("next_offset: 0x%x, target_address: 0x%x,"
				"packet len: %d", next_expected_offset,
						  packet.target_address,
						  packet.data_len);

			rc = nrf_modem_bootloader_fw_write(packet.target_address,
							   packet.data,
							   packet.data_len);
			if (rc != 0) {
				LOG_ERR("Error in writing data, err: %d", rc);
				return MGMT_ERR_EBADSTATE;
			}
		}
	}

	if (whole_file_received) {
		rc = nrf_modem_bootloader_update();
		if (rc != 0) {
			LOG_ERR("Error in transfer_end");
			return MGMT_ERR_EBADSTATE;
		}
		bootloader = false;
	}

	return encode_response(ctx, next_expected_offset);
}

static int fmfu_get_memory_hash(struct smp_streamer *ctxt)
{
	uint64_t start;
	uint64_t end;
	struct nrf_modem_bootloader_digest digest;
	struct zcbor_string zdigest;
	bool ok;
	size_t decoded = 0;
	zcbor_state_t *zsd = ctxt->reader->zs;
	zcbor_state_t *zse = ctxt->writer->zs;
	struct zcbor_map_decode_key_val mem_hash_decode[] = {
		ZCBOR_MAP_DECODE_KEY_VAL(start, zcbor_uint64_decode, &start),
		ZCBOR_MAP_DECODE_KEY_VAL(end, zcbor_uint64_decode, &end),
	};
	int rc;

	/* We expect two variables: the start and end address of a memory region
	 * that we want to verify (obtain a hash for)
	 */
	start = 0;
	end = 0;

	ok = zcbor_map_decode_bulk(zsd, mem_hash_decode,
		ARRAY_SIZE(mem_hash_decode), &decoded) == 0;


	if (!ok || start == 0 || end == 0) {
		return MGMT_ERR_EINVAL;
	}

	rc = nrf_modem_bootloader_digest(start, end - start, &digest);

	if (rc != 0) {
		LOG_ERR("nrf_fmfu_hash_get failed, err: %d.", rc);
		LOG_ERR("start:%d end: %d\n.", (uint32_t)start, (uint32_t)end);
		return MGMT_ERR_EBADSTATE;
	}

	/* Put the digest response in the response */
	zdigest.value = digest.data;
	zdigest.len = NRF_MODEM_BOOTLOADER_DIGEST_LEN;

	ok = zcbor_tstr_put_lit(zse, "digest")			&&
	     zcbor_bstr_encode(zse, &zdigest)			&&
	     zcbor_tstr_put_lit(zse, "rc")			&&
	     zcbor_int32_put(zse, rc);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ENOMEM;
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
	mgmt_register_group(&fmfu_mgmt_group);
	return 0;
}

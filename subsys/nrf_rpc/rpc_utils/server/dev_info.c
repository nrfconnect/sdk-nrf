/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ncs_commit.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/dev_info.h>
#include <rpc_utils_group.h>
#include <zephyr/debug/coredump.h>
#include <zephyr/drivers/coredump.h>
#include <zephyr/random/random.h>

#include <nrf_rpc_cbor.h>

#define DUMP_META_VERSION 1
#define ASSERT_FILENAME_SIZE 255

#define DUMP_METADATA_SIZE(meta) \
	sizeof((meta).uuid) + 1 + \
	sizeof((meta).reason) + 1 + \
	sizeof((meta).lr) + 1 + \
	sizeof((meta).pc) + 1 + \
	sizeof((meta).xpsr) + 1 + \
	sizeof((meta).sp) + 1 + \
	sizeof((meta).assert_line) + 1 + \
	(meta.assert_line ? (strlen(meta.assert_filename) + 3) : 1)

struct arm_arch_block {
	struct {
		uint32_t r0;
		uint32_t r1;
		uint32_t r2;
		uint32_t r3;
		uint32_t r12;
		uint32_t lr;
		uint32_t pc;
		uint32_t xpsr;
		uint32_t sp;

		/* callee registers - optionally collected in V2 */
		uint32_t r4;
		uint32_t r5;
		uint32_t r6;
		uint32_t r7;
		uint32_t r8;
		uint32_t r9;
		uint32_t r10;
		uint32_t r11;
	} r;
} __packed;

struct dump_metadata
{
	struct dump_metadata_header {
		uint8_t magic[8];
		uint32_t uuid;
		uint8_t version;
		uint8_t has_assert_info;
	} __packed header;
	uint32_t assert_line;
	const char assert_filename[ASSERT_FILENAME_SIZE];
} __packed;

static struct dump_metadata next_dump_info;

static struct coredump_mem_region_node dump_region = {
	.start = (uintptr_t)&next_dump_info,
	.size = sizeof(next_dump_info.header)
};

static const uint8_t CRASH_INFO_MAGIC[] = { 'D', 'U', 'M', 'P', 'I', 'N', 'F', 'O' };

static int load_coredump_fragment(uint16_t offset, void *buffer, size_t length)
{
	struct coredump_cmd_copy_arg copy_params;

	copy_params.offset = offset;
	copy_params.buffer = (uint8_t *)buffer;
	copy_params.length = length;

	return coredump_cmd(COREDUMP_CMD_COPY_STORED_DUMP, &copy_params);
}

static int load_crash_info(struct nrf_rpc_crash_info *info)
{
	int rc;
	int size;
	uint16_t offset = 0;
	uint16_t max_offset;
	struct coredump_hdr_t dump_hdr;
	struct coredump_arch_hdr_t arch_hdr;
	struct arm_arch_block arch_blk;
	struct coredump_threads_meta_hdr_t thread_meta_hdr;
	struct coredump_mem_hdr_t mem_hdr;
	struct dump_metadata metadata;
	uint8_t header_id;

	rc = coredump_cmd(COREDUMP_CMD_VERIFY_STORED_DUMP, NULL);
	if (rc == 0) {
		return -ENOENT;
	} else if (rc != 1) {
		return rc;
	}

	size = coredump_query(COREDUMP_QUERY_GET_STORED_DUMP_SIZE, NULL);
	if (rc <= 0) {
		return rc;
	}

	max_offset = offset + size;

	rc = load_coredump_fragment(offset, &dump_hdr, sizeof(dump_hdr));
	if (rc < 0) {
		return rc;
	}

	info->reason = sys_le16_to_cpu(dump_hdr.reason);
	offset = sizeof(dump_hdr);

	while (offset < max_offset) {
		rc = load_coredump_fragment(offset, &header_id, sizeof(uint8_t));
		if (rc < 0) {
			return rc;
		}

		switch (header_id) {
		case COREDUMP_ARCH_HDR_ID:
			/* Fill out available registers */
			rc = load_coredump_fragment(offset, &arch_hdr, sizeof(arch_hdr));
			if (rc < 0) {
				return rc;
			}

			offset += sizeof(arch_hdr);

			rc = load_coredump_fragment(offset, &arch_blk, arch_hdr.num_bytes);
			if (rc < 0) {
				return rc;
			}

			offset += arch_hdr.num_bytes;

			info->pc = arch_blk.r.pc;
			info->lr = arch_blk.r.lr;
			info->xpsr = arch_blk.r.xpsr;
			info->sp = arch_blk.r.sp;

			break;
		case THREADS_META_HDR_ID:
			/* Intentionally skip over */
			rc = load_coredump_fragment(offset, &thread_meta_hdr,
						    sizeof(thread_meta_hdr));
			if (rc < 0) {
				return rc;
			}

			offset += sizeof(thread_meta_hdr) + thread_meta_hdr.num_bytes;

			break;
		case COREDUMP_MEM_HDR_ID:
			rc = load_coredump_fragment(offset, &mem_hdr, sizeof(mem_hdr));
			if (rc < 0) {
				return rc;
			}

			size = mem_hdr.end - mem_hdr.start;
			offset += sizeof(mem_hdr);

			if (size >= sizeof(struct dump_metadata_header)) {
				rc = load_coredump_fragment(offset, &metadata.header,
							    sizeof(struct dump_metadata_header));
				if (rc < 0) {
					return rc;
				}

				offset += sizeof(struct dump_metadata_header);
				size -= sizeof(struct dump_metadata_header);

				if (memcmp((void *)metadata.header.magic, CRASH_INFO_MAGIC,
					   sizeof(CRASH_INFO_MAGIC)) == 0) {
					info->uuid = metadata.header.uuid;

					if (metadata.header.has_assert_info) {
						rc = load_coredump_fragment(offset,
									    &metadata.assert_line,
									    size);
						if (rc < 0) {
							return rc;
						}

						info->assert_line = metadata.assert_line;
						strncpy(info->assert_filename,
							metadata.assert_filename,
							sizeof(info->assert_filename));
					}
				}
			}

			offset += size;

			break;
		}
	}
}

static void get_server_version(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	struct nrf_rpc_cbor_ctx rsp_ctx;
	size_t cbor_buffer_size = 0;

	cbor_buffer_size += 2 + sizeof(NCS_COMMIT_STRING) - 1;
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, cbor_buffer_size);

	nrf_rpc_encode_str(&rsp_ctx, NCS_COMMIT_STRING, sizeof(NCS_COMMIT_STRING) - 1);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, get_server_version, RPC_UTIL_DEV_INFO_GET_VERSION,
			 get_server_version, NULL);

static void get_crash_info(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			   void *handler_data)
{
	int rc;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	struct nrf_rpc_crash_info info = { 0 };

	nrf_rpc_cbor_decoding_done(group, ctx);

	rc = load_crash_info(&info);
	if (rc) {
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);

		nrf_rpc_encode_null(&rsp_ctx);
	} else {
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, DUMP_METADATA_SIZE(info));

		nrf_rpc_encode_uint(&rsp_ctx, info.uuid);
		nrf_rpc_encode_uint(&rsp_ctx, info.reason);
		nrf_rpc_encode_uint(&rsp_ctx, info.pc);
		nrf_rpc_encode_uint(&rsp_ctx, info.lr);
		nrf_rpc_encode_uint(&rsp_ctx, info.sp);
		nrf_rpc_encode_uint(&rsp_ctx, info.xpsr);
		nrf_rpc_encode_uint(&rsp_ctx, info.assert_line);
		nrf_rpc_encode_str(&rsp_ctx, info.assert_filename, -1);
	}

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, get_crash_info, RPC_UTIL_DEV_INFO_GET_CRASH_INFO,
			 get_crash_info, NULL);

#ifndef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(const char *file, unsigned int line)
{
	next_dump_info.assert_line = line;
	next_dump_info.header.has_assert_info = true;

	strncpy(next_dump_info.assert_filename, file, sizeof(next_dump_info.assert_filename));

	dump_region.size = sizeof(next_dump_info);

	k_panic();
}
#endif

static int init_dump_metadata(void)
{
	const struct device *const coredump_dev = DEVICE_DT_GET(DT_NODELABEL(coredump_device));

	memset(&next_dump_info, 0, sizeof(next_dump_info));
	memcpy(next_dump_info.header.magic, CRASH_INFO_MAGIC, sizeof(next_dump_info.header.magic));

	next_dump_info.header.version = DUMP_META_VERSION;
	next_dump_info.header.uuid = sys_rand32_get();

	coredump_device_register_memory(coredump_dev, &dump_region);

	return 0;
}

SYS_INIT(init_dump_metadata, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

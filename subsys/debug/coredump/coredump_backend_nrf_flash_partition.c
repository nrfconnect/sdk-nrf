/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/debug/coredump.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_NRFX_NVMC)
#include <nrfx_nvmc.h>
#elif defined(CONFIG_NRFX_RRAMC)
#include <nrfx_rramc.h>
#endif

#include <string.h>

/* Check DTS prerequisites */

#if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf51_flash_controller))
#define DT_DRV_COMPAT nordic_nrf51_flash_controller
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf52_flash_controller))
#define DT_DRV_COMPAT nordic_nrf52_flash_controller
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf53_flash_controller))
#define DT_DRV_COMPAT nordic_nrf53_flash_controller
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf91_flash_controller))
#define DT_DRV_COMPAT nordic_nrf91_flash_controller
#elif DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_rram_controller))
#define DT_DRV_COMPAT nordic_rram_controller
#else
#error "Unsupported internal flash controller"
#endif

#define PARTITION_LABEL coredump_partition

/* Extract DTS properties */

#define FLASH_NODE	 DT_INST(0, soc_nv_flash)
#define FLASH_ADDR	 DT_REG_ADDR(FLASH_NODE)
#define FLASH_WRITE_SIZE DT_PROP(FLASH_NODE, write_block_size)
#if DT_NODE_HAS_PROP(FLASH_NODE, erase_block_size)
#define FLASH_ERASE_SIZE DT_PROP(FLASH_NODE, erase_block_size)
#else
#define FLASH_ERASE_SIZE FLASH_WRITE_SIZE
#endif

#define PARTITION_OFFSET FIXED_PARTITION_OFFSET(PARTITION_LABEL)
#define PARTITION_SIZE	 FIXED_PARTITION_SIZE(PARTITION_LABEL)
#define PARTITION_ADDR	 (FLASH_ADDR + PARTITION_OFFSET)

BUILD_ASSERT(FIXED_PARTITION_EXISTS(PARTITION_LABEL),
	     "Missing fixed partition named 'coredump_partition'");
BUILD_ASSERT(PARTITION_OFFSET % FLASH_ERASE_SIZE == 0,
	     "Core dump partition unaligned to erase block size");

struct header {
	uint8_t magic[4];    /* "CD01" */
	uint32_t offset;     /* Core dump data start */
	uint32_t size;	     /* Core dump data size */
	uint16_t dump_crc;   /* Core dump data CRC16 */
	uint16_t header_crc; /* Header CRC16 (up to this field) */
} __packed;

static const uint8_t MAGIC[4] = {'C', 'D', '0', '1'};

enum {
	HEADER_SIZE = ROUND_UP(sizeof(struct header), FLASH_WRITE_SIZE),
	WRITE_BUF_SIZE = ROUND_UP(CONFIG_DEBUG_COREDUMP_BACKEND_NRF_FLASH_PARTITION_WRITE_BUF_SIZE,
				  FLASH_WRITE_SIZE),
};

static int write_error;			  /* Error occurred when writing a core dump */
static uint8_t write_buf[WRITE_BUF_SIZE]; /* Write buffer to assure aligned flash access */
static size_t write_buf_pos;		  /* # of dump data bytes buffered in the write buffer */
static size_t write_pos;		  /* # of dump data bytes already written to flash */
static uint16_t dump_crc;		  /* CRC16 of already written or buffered bytes */

static inline const struct header *get_stored_header(void)
{
	return (const struct header *)(PARTITION_ADDR);
}

static inline const uint8_t *get_stored_dump(const struct header *header)
{
	return (const uint8_t *)(PARTITION_ADDR) + header->offset;
}

static inline uint16_t calc_header_crc(const struct header *header)
{
	return crc16_ccitt(0xffff, (const uint8_t *)header, offsetof(struct header, header_crc));
}

static inline uint16_t calc_dump_crc(const struct header *header)
{
	return crc16_ccitt(0xffff, get_stored_dump(header), header->size);
}

static inline bool validate_header(const struct header *header)
{
	return (memcmp(header->magic, MAGIC, sizeof(MAGIC)) == 0) &&
	       (calc_header_crc(header) == header->header_crc);
}

static inline bool validate_dump(const struct header *header)
{
	return calc_dump_crc(header) == header->dump_crc;
}

static inline bool is_within_partition(uint32_t offset, size_t size)
{
	return (offset < PARTITION_SIZE) && (size <= PARTITION_SIZE - offset);
}

static void write(uint32_t offset, const uint8_t *data, size_t size)
{
	if (write_error != 0) {
		return;
	}

	if (!is_within_partition(offset, size)) {
		write_error = -ENOMEM;
		return;
	}

#ifdef CONFIG_NRFX_NVMC
	for (uint32_t i = 0; i < size; i += sizeof(uint32_t)) {
		nrfx_nvmc_word_write(PARTITION_OFFSET + offset + i,
				     UNALIGNED_GET((const uint32_t *)&data[i]));
	}
	while (!nrfx_nvmc_write_done_check()) {
	}
#elif defined(CONFIG_NRFX_RRAMC)
	nrf_rramc_config_t config = {
		.mode_write = true,
		.write_buff_size = 1,
	};

	nrf_rramc_config_set(NRF_RRAMC, &config);

	memcpy((void *)(PARTITION_ADDR + offset), data, size);
	barrier_dmem_fence_full();

	/*
	 * The write buffer flush trigger can be skipped as long as the write block size
	 * divides the configured write buffer size (1 * 16B).
	 */
	BUILD_ASSERT(FLASH_WRITE_SIZE % 16 == 0,
		     "Write block size not divisible by write buffer word size");

	config.mode_write = false;
	nrf_rramc_config_set(NRF_RRAMC, &config);
#endif
}

static void erase(uint32_t offset, size_t size)
{
	if (write_error != 0) {
		return;
	}

	if (!is_within_partition(offset, size)) {
		write_error = -ENOMEM;
		return;
	}

#ifdef CONFIG_NRFX_NVMC
	for (uint32_t end = offset + size; offset < end; offset += FLASH_ERASE_SIZE) {
		(void)nrfx_nvmc_page_erase(PARTITION_OFFSET + offset);
	}
#elif defined(CONFIG_NRFX_RRAMC)
	nrf_rramc_config_t config = {
		.mode_write = true,
		.write_buff_size = 1,
	};

	nrf_rramc_config_set(NRF_RRAMC, &config);

	memset((void *)(PARTITION_ADDR + offset), 0xff, size);
	barrier_dmem_fence_full();

	/*
	 * The write buffer flush trigger can be skipped as long as the erase block size
	 * divides the configured write buffer size (1 * 16B).
	 */
	BUILD_ASSERT(FLASH_ERASE_SIZE % 16 == 0,
		     "Erase block size not divisible by write buffer word size");

	config.mode_write = false;
	nrf_rramc_config_set(NRF_RRAMC, &config);
#endif
}

static int copy_stored_dump(off_t offset, uint8_t *buffer, size_t size)
{
	const struct header *header = get_stored_header();

	if (!validate_header(header)) {
		return 0;
	}

	if (offset >= header->size) {
		return -EINVAL;
	}

	size = MIN(size, header->size - offset);
	memcpy(buffer, get_stored_dump(header) + offset, size);

	return (int)size;
}

static void coredump_nrf_flash_backend_start(void)
{
	/*
	 * For flash: erase the entire partition to prepare it for write.
	 * For RRAM:  erase the previously written header only.
	 */
	erase(0, IS_ENABLED(CONFIG_NRFX_NVMC) ? PARTITION_SIZE : HEADER_SIZE);

	write_buf_pos = 0;
	write_pos = 0;
	dump_crc = 0xffff;
}

static void coredump_nrf_flash_backend_end(void)
{
	struct header header;
	uint8_t buffer[HEADER_SIZE] = {};

	if (write_buf_pos > 0) {
		/* Flush the write buffer */
		memset(&write_buf[write_buf_pos], 0, WRITE_BUF_SIZE - write_buf_pos);
		write(HEADER_SIZE + write_pos, write_buf, WRITE_BUF_SIZE);
		write_pos += write_buf_pos;
	}

	if (write_error != 0) {
		return;
	}

	memcpy(header.magic, MAGIC, sizeof(MAGIC));
	header.offset = HEADER_SIZE;
	header.size = write_pos;
	header.dump_crc = dump_crc;
	header.header_crc = calc_header_crc(&header);

	memcpy(buffer, &header, sizeof(header));
	write(0, buffer, HEADER_SIZE);
}

static void coredump_nrf_flash_backend_buffer_output(uint8_t *data, size_t size)
{
	size_t chunk_size;

	/*
	 * Write all data using intermediate buffer to assure proper write alignment,
	 * and to calculate a valid CRC even if the data keeps changing during write
	 * (for example, if the data is within the IRQ stack region).
	 */

	while (size > 0) {
		chunk_size = MIN(size, WRITE_BUF_SIZE - write_buf_pos);
		memcpy(&write_buf[write_buf_pos], data, chunk_size);
		dump_crc = crc16_ccitt(dump_crc, &write_buf[write_buf_pos], chunk_size);
		write_buf_pos += chunk_size;
		data += chunk_size;
		size -= chunk_size;

		if (write_buf_pos == WRITE_BUF_SIZE) {
			write(HEADER_SIZE + write_pos, write_buf, WRITE_BUF_SIZE);
			write_pos += WRITE_BUF_SIZE;
			write_buf_pos = 0;
		}
	}
}

static int coredump_nrf_flash_backend_query(enum coredump_query_id query_id, void *arg)
{
	int ret;
	const struct header *header;

	switch (query_id) {
	case COREDUMP_QUERY_GET_ERROR:
		ret = write_error;
		break;
	case COREDUMP_QUERY_HAS_STORED_DUMP:
		header = get_stored_header();
		ret = validate_header(header) ? 1 : 0;
		break;
	case COREDUMP_QUERY_GET_STORED_DUMP_SIZE:
		header = get_stored_header();
		ret = validate_header(header) ? header->size : 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int coredump_nrf_flash_backend_cmd(enum coredump_cmd_id cmd_id, void *arg)
{
	int ret;
	const struct header *header;
	struct coredump_cmd_copy_arg *copy_arg;

	switch (cmd_id) {
	case COREDUMP_CMD_CLEAR_ERROR:
		write_error = 0;
		ret = 0;
		break;
	case COREDUMP_CMD_VERIFY_STORED_DUMP:
		header = get_stored_header();
		ret = (validate_header(header) && validate_dump(header)) ? 1 : 0;
		break;
	case COREDUMP_CMD_ERASE_STORED_DUMP:
		erase(0, PARTITION_SIZE);
		ret = 0;
		break;
	case COREDUMP_CMD_COPY_STORED_DUMP:
		if (arg != NULL) {
			copy_arg = arg;
			ret = copy_stored_dump(copy_arg->offset, copy_arg->buffer,
					       copy_arg->length);
		} else {
			ret = -EINVAL;
		}
		break;
	case COREDUMP_CMD_INVALIDATE_STORED_DUMP:
		erase(0, HEADER_SIZE);
		ret = 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

struct coredump_backend_api coredump_backend_other = {
	.start = coredump_nrf_flash_backend_start,
	.end = coredump_nrf_flash_backend_end,
	.buffer_output = coredump_nrf_flash_backend_buffer_output,
	.query = coredump_nrf_flash_backend_query,
	.cmd = coredump_nrf_flash_backend_cmd,
};

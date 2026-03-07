/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Dump RAM log history to the log_history flash partition on fatal error,
 * using direct nrfx writes (same pattern as coredump_backend_nrf_flash_partition.c)
 * so the fault path avoids Zephyr flash API and locks.
 * Stores preformatted text (same format as RPC client log) so the client can
 * display without decoding cbprintf packages.
 */

#include "log_backend_rpc_history.h"

#include <zephyr/logging/log_msg.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_NRFX_NVMC)
#include <nrfx_nvmc.h>
#elif defined(CONFIG_NRFX_RRAMC)
#include <nrfx_rramc.h>
#endif

#define PARTITION_LABEL log_history

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
#error "Unsupported internal flash controller for log history dump"
#endif

#define FLASH_NODE     DT_INST(0, soc_nv_flash)
#define FLASH_ADDR    DT_REG_ADDR(FLASH_NODE)
#define FLASH_WRITE_SIZE DT_PROP(FLASH_NODE, write_block_size)
#if DT_NODE_HAS_PROP(FLASH_NODE, erase_block_size)
#define FLASH_ERASE_SIZE DT_PROP(FLASH_NODE, erase_block_size)
#else
#define FLASH_ERASE_SIZE FLASH_WRITE_SIZE
#endif

#define PARTITION_OFFSET FIXED_PARTITION_OFFSET(PARTITION_LABEL)
#define PARTITION_SIZE   FIXED_PARTITION_SIZE(PARTITION_LABEL)
#define PARTITION_ADDR   (FLASH_ADDR + PARTITION_OFFSET)

BUILD_ASSERT(FIXED_PARTITION_EXISTS(PARTITION_LABEL),
	     "Missing fixed partition named 'log_history'");
BUILD_ASSERT(PARTITION_OFFSET % FLASH_ERASE_SIZE == 0,
	     "Log history partition unaligned to erase block size");

struct log_history_dump_header {
	uint8_t magic[4];    /* "LH01" */
	uint32_t offset;     /* Payload start offset in partition */
	uint32_t size;       /* Payload size in bytes */
} __packed;

static const uint8_t MAGIC[4] = {'L', 'H', '0', '1'};

/* Text format marker at start of payload (so client can detect format) */
static const uint8_t PAYLOAD_TEXT_MAGIC[4] = {'L', 'H', '0', '2'};

enum {
	HEADER_SIZE = ROUND_UP(sizeof(struct log_history_dump_header), FLASH_WRITE_SIZE),
	WRITE_BUF_SIZE = ROUND_UP(64, FLASH_WRITE_SIZE),
	LINE_BUF_SIZE = 256,
};

static int write_error;
static uint8_t write_buf[WRITE_BUF_SIZE];
static size_t write_buf_pos;
static size_t write_pos;

static bool is_within_partition(uint32_t offset, size_t size)
{
	return (offset < PARTITION_SIZE) && (size <= PARTITION_SIZE - offset);
}

static void partition_write(uint32_t offset, const uint8_t *data, size_t size)
{
	if (write_error != 0) {
		return;
	}
	if (!is_within_partition(offset, size)) {
		write_error = -ENOMEM;
		return;
	}

#if defined(CONFIG_NRFX_NVMC)
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
	BUILD_ASSERT(FLASH_WRITE_SIZE % 16 == 0,
		     "Write block size not divisible by write buffer word size");
	config.mode_write = false;
	nrf_rramc_config_set(NRF_RRAMC, &config);
#endif
}

static void partition_erase(uint32_t offset, size_t size)
{
	if (write_error != 0) {
		return;
	}
	if (!is_within_partition(offset, size)) {
		write_error = -ENOMEM;
		return;
	}

#if defined(CONFIG_NRFX_NVMC)
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
	BUILD_ASSERT(FLASH_ERASE_SIZE % 16 == 0,
		     "Erase block size not divisible by write buffer word size");
	config.mode_write = false;
	nrf_rramc_config_set(NRF_RRAMC, &config);
#endif
}

static void history_dump_start(void)
{
	partition_erase(0, IS_ENABLED(CONFIG_NRFX_NVMC) ? PARTITION_SIZE : HEADER_SIZE);
	write_buf_pos = 0;
	write_pos = 0;
}

static void history_dump_buffer_output(const uint8_t *data, size_t size)
{
	size_t chunk_size;

	while (size > 0) {
		chunk_size = MIN(size, WRITE_BUF_SIZE - write_buf_pos);
		memcpy(&write_buf[write_buf_pos], data, chunk_size);
		write_buf_pos += chunk_size;
		data += chunk_size;
		size -= chunk_size;

		if (write_buf_pos == WRITE_BUF_SIZE) {
			partition_write(HEADER_SIZE + write_pos, write_buf, WRITE_BUF_SIZE);
			write_pos += WRITE_BUF_SIZE;
			write_buf_pos = 0;
		}
	}
}

/**
 * Dump current RAM log history to the log_history flash partition.
 * Uses direct nrfx writes (no Zephyr flash API / locks) so it is safe to call
 * from the fatal error handler. Only effective when log history storage is RAM.
 * Writes preformatted text (same format as RPC/client log, payload magic "LH02").
 */
static void history_dump_end(void)
{
	struct log_history_dump_header header;
	uint8_t header_buf[HEADER_SIZE];

	memset(header_buf, 0, sizeof(header_buf));

	if (write_buf_pos > 0) {
		memset(&write_buf[write_buf_pos], 0, WRITE_BUF_SIZE - write_buf_pos);
		partition_write(HEADER_SIZE + write_pos, write_buf, WRITE_BUF_SIZE);
		write_pos += write_buf_pos;
	}

	if (write_error != 0) {
		return;
	}

	memcpy(header.magic, MAGIC, sizeof(MAGIC));
	header.offset = HEADER_SIZE;
	header.size = write_pos;
	memcpy(header_buf, &header, sizeof(header));
	partition_write(0, header_buf, HEADER_SIZE);
}

void log_rpc_history_dump_to_flash(void)
{
	union log_msg_generic *msg;
	static char line_buf[LINE_BUF_SIZE];

	write_error = 0;
	history_dump_start();

	/* Payload format marker so client can detect text format */
	history_dump_buffer_output(PAYLOAD_TEXT_MAGIC, sizeof(PAYLOAD_TEXT_MAGIC));

	while ((msg = log_rpc_history_pop()) != NULL) {
		if (z_log_item_is_msg((const union log_msg_generic *)msg)) {
			size_t len = log_rpc_format_msg_to_buf(&msg->log, (uint8_t *)line_buf,
							       sizeof(line_buf));

			if (len > 0) {
				history_dump_buffer_output((const uint8_t *)line_buf, len);
			}
		}
		log_rpc_history_free(msg);
	}

	history_dump_end();
}

static inline const struct log_history_dump_header *history_dump_get_stored_header(void)
{
	return (const struct log_history_dump_header *)PARTITION_ADDR;
}

static inline bool history_dump_validate_header(const struct log_history_dump_header *h)
{
	return h != NULL && memcmp(h->magic, MAGIC, sizeof(MAGIC)) == 0;
}

bool log_rpc_history_dump_has_stored(void)
{
	return history_dump_validate_header(history_dump_get_stored_header());
}

size_t log_rpc_history_dump_get_stored_size(void)
{
	const struct log_history_dump_header *h = history_dump_get_stored_header();

	if (!history_dump_validate_header(h)) {
		return 0;
	}
	return h->size;
}

int log_rpc_history_dump_copy(size_t offset, uint8_t *buffer, size_t size)
{
	const struct log_history_dump_header *h = history_dump_get_stored_header();

	if (!history_dump_validate_header(h) || buffer == NULL) {
		return -EINVAL;
	}
	if (offset >= h->size) {
		return 0;
	}
	size = MIN(size, h->size - offset);
	memcpy(buffer, (const uint8_t *)PARTITION_ADDR + HEADER_SIZE + offset, size);
	return (int)size;
}

int log_rpc_history_dump_clear(void)
{
	const struct log_history_dump_header *h = history_dump_get_stored_header();

	if (!history_dump_validate_header(h)) {
		return 0; /* Already clear */
	}
	partition_erase(0, IS_ENABLED(CONFIG_NRFX_NVMC) ? PARTITION_SIZE : HEADER_SIZE);
	return 0;
}

/*
 * Copyright (c) 2024 Junho Lee <tot0roprog@gmail.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/hash_function.h>

#include "blob_xfer.h"

#define BLOB_AREA_ID FLASH_AREA_ID(blob_storage)
#define BUF_SIZE 0x100

static struct bt_mesh_blob_srv *blob_srv;
static struct bt_mesh_blob_cli *blob_cli;

const uint8_t SAMPLE_BLOB_DATA[] = {0xC0, 0xFF, 0xEE};

static struct bt_mesh_blob_io_flash blob_rx_flash_stream;
static struct {
	struct bt_mesh_blob_cli_inputs inputs;
	struct bt_mesh_blob_target targets[MAX_TARGET_NODES];
	struct bt_mesh_blob_target_pull pull[MAX_TARGET_NODES];
	uint8_t target_count;
	struct bt_mesh_blob_xfer xfer;
	struct bt_mesh_blob_io_flash blob_tx_flash_stream;
} blob_tx;

void write_flash_area(uint8_t id, off_t offset, size_t size)
{
	const struct flash_area *area;
	uint32_t align;
	uint32_t hash = 0;
	uint8_t buf[BUF_SIZE];
	int i = 0;
	int err;
	off_t start, end;
	off_t aligned_offset, aligned_end;
	size_t aligned_size, chunk_size;
	size_t total_written = 0;
	off_t erase_offset;
	size_t erase_size;

	if((err = flash_area_open(id, &area)) != 0) {
		printk("Failed to open flash area: %d\n", err);
		return;
	}

	/* Erase flash area */
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_info page;
	const struct device *flash_dev;


	flash_dev = flash_area_get_device(area);
	if (!flash_dev) {
		printk("Failed to get device for flash area\n");
		return;
	}

	err = flash_get_page_info_by_offs(flash_dev,
					  offset, &page);
	if (err) {
		printk("Failed to get page info: %d\n", err);
		return;
	}

	erase_offset = ROUND_DOWN(offset, page.size);
	erase_size = page.size * DIV_ROUND_UP(ROUND_UP(offset + size, page.size) - erase_offset,
					      page.size);
#else
	erase_offset = ROUND_DOWN(offset, align);
	erase_size = offset % align + size + ROUND_UP(i, align);
#endif
	printk("Erase flash: offset: %#lx, size: %#x\n", erase_offset, erase_size);
	if ((err = flash_area_erase(area, erase_offset, erase_size) != 0)) {
		printk("Failed to erase flash area: %d\n", err);
		return;
	}

	/* Write flash area */
	printk("Write data to flash (size: %#x)\n", size);

	align = flash_area_align(area);
	aligned_offset = ROUND_DOWN(offset, align);
	aligned_end = ROUND_UP(offset + size, align);
	aligned_size = aligned_end - aligned_offset;

	while (total_written < size) {
		chunk_size = aligned_size > BUF_SIZE ? BUF_SIZE : aligned_size;

		memset(buf, 0xFF, chunk_size);

		start = (total_written == 0) ? offset - aligned_offset : 0;
		end = (chunk_size < size - total_written) ? chunk_size : size - total_written +
											      start;

		for (i = start; i < end; i ++) {
			buf[i] = SAMPLE_BLOB_DATA[total_written++ % ARRAY_SIZE(SAMPLE_BLOB_DATA)];
		}
		hash += sys_hash32(&buf[start], end - start);

		if ((err = flash_area_write(area, aligned_offset, buf, chunk_size) != 0)) {
			printk("Failed to write data to flash: %d\n", err);
			return;
		}

		aligned_offset += chunk_size;
		aligned_size -= chunk_size;
	}

	flash_area_close(area);

	printk("Hash value of written data: %#8.8x\n", hash);
}

void read_flash_area(uint8_t id, off_t offset, size_t size)
{
	const struct flash_area *area;
	uint8_t buf[BUF_SIZE];
	uint32_t align;
	off_t aligned_offset, aligned_end;
	off_t start, end;
	size_t aligned_size, chunk_size;
	size_t total_read = 0;
	int i;
	int err;

	if((err = flash_area_open(id, &area)) != 0) {
		printk("Failed to open flash area: %d\n", err);
		return;
	}

	align = flash_area_align(area);

	aligned_offset = ROUND_DOWN(offset, align);
	aligned_end = ROUND_UP(offset + size, align);
	aligned_size = aligned_end - aligned_offset;

	while (total_read < size) {
		chunk_size = aligned_size > BUF_SIZE ? BUF_SIZE : aligned_size;

		if ((err = flash_area_read(area, aligned_offset, buf, chunk_size)) != 0) {
			printk("Failed to read: %d\n", err);
			return;
		}

		start = (total_read == 0) ? offset - aligned_offset : 0;
		end = (chunk_size < size - total_read) ? chunk_size : size - total_read + start;

		for (i = start; i < end; i++) {
			if (total_read % 32 == 0) {
				printk("\n%#8.8lx to %#8.8lx: ", offset + total_read,
				       MIN(offset + total_read + 31, offset + size - 1));
			}
			printk("%2.2x", buf[i]);
			total_read++;
		}
		aligned_offset += chunk_size;
		aligned_size -= chunk_size;
	}
	printk("\n");

	flash_area_close(area);
}

void hash_flash_area(uint8_t id, off_t offset, size_t size)
{
	const struct flash_area *area;
	uint8_t buf[BUF_SIZE];
	uint32_t align;
	uint32_t hash = 0;
	off_t aligned_offset, aligned_end;
	off_t start, end;
	size_t aligned_size, chunk_size;
	size_t total_read = 0;
	int err;

	if((err = flash_area_open(id, &area)) != 0) {
		printk("Failed to open flash area: %d\n", err);
		return;
	}

	align = flash_area_align(area);

	aligned_offset = ROUND_DOWN(offset, align);
	aligned_end = ROUND_UP(offset + size, align);
	aligned_size = aligned_end - aligned_offset;

	while (total_read < size) {
		chunk_size = aligned_size > BUF_SIZE ? BUF_SIZE : aligned_size;

		if ((err = flash_area_read(area, aligned_offset, buf, chunk_size)) != 0) {
			printk("Failed to read: %d\n", err);
			return;
		}

		start = (total_read == 0) ? offset - aligned_offset : 0;
		end = (chunk_size < size - total_read) ? chunk_size : size - total_read + start;

		hash += sys_hash32(&buf[start], end - start);

		aligned_offset += chunk_size;
		aligned_size -= chunk_size;
		total_read += end - start;
	}
	printk("\n");

	flash_area_close(area);

	printk("Hash value of read data: %#8.8x\n", hash);
}

uint8_t get_flash_area_id(void)
{
	return BLOB_AREA_ID;
}

static int blob_xfer_srv_start(struct bt_mesh_blob_srv *srv, struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_blob_xfer *xfer)
{
	printk("BLOB Transfer RX Start\n");
	return 0;
}

static void blob_xfer_srv_end(struct bt_mesh_blob_srv *srv, uint64_t id, bool success)
{
	struct bt_mesh_blob_io_flash *flash_stream =
		CONTAINER_OF(srv->io, struct bt_mesh_blob_io_flash, io);
	size_t size;
	off_t offset;

	if (success == 1){
		printk("BLOB Transfer RX End: Success - id(%16.16llx)\n", id);
		size = srv->state.xfer.size;
		offset = flash_stream->offset;
		hash_flash_area(flash_stream->area_id, offset, size);
	}
	else {
		printk("BLOB Transfer RX End: Failed - id(%16.16llx)\n", id);
	}
}

static void blob_xfer_srv_suspended(struct bt_mesh_blob_srv *srv)
{
	printk("BLOB Transfer RX Suspended\n");
}

static void blob_xfer_srv_resume(struct bt_mesh_blob_srv *srv)
{
	printk("BLOB Transfer RX Resume\n");
}

static int blob_xfer_srv_recover(struct bt_mesh_blob_srv *srv, struct bt_mesh_blob_xfer *xfer,
				const struct bt_mesh_blob_io **io)
{
	printk("BLOB Transfer RX Recover\n");
	*io = &blob_rx_flash_stream.io;

	return 0;
}

static void blob_xfer_cli_caps(struct bt_mesh_blob_cli *b, const struct bt_mesh_blob_cli_caps *caps)
{
	int err;

	printk("BLOB Transfer TX Caps\n");

	blob_tx.xfer.block_size_log = caps->max_block_size_log;
	blob_tx.xfer.chunk_size = caps->max_chunk_size;

	if (!(blob_tx.xfer.mode & BT_MESH_BLOB_XFER_MODE_ALL)) {
		blob_tx.xfer.mode =
			caps->modes == BT_MESH_BLOB_XFER_MODE_ALL ?
			BT_MESH_BLOB_XFER_MODE_PUSH : caps->modes;
	} else {
		blob_tx.xfer.mode =
			caps->modes == BT_MESH_BLOB_XFER_MODE_ALL ?
			blob_tx.xfer.mode : caps->modes;
	}

	err = bt_mesh_blob_cli_send(b, &blob_tx.inputs, &blob_tx.xfer,
				    &blob_tx.blob_tx_flash_stream.io);
	if (err) {
		printk("Starting BLOB xfer failed: %d\n", err);
	}
}

static void blob_xfer_cli_lost_target(struct bt_mesh_blob_cli *b, struct bt_mesh_blob_target *blobt,
				     enum bt_mesh_blob_status reason)
{
	printk("BLOB Transfer TX Lost Target: %4.4x reason %d\n", blobt->addr, reason);
}

static void blob_xfer_cli_suspended(struct bt_mesh_blob_cli *b)
{
	printk("BLOB Transfer TX Suspended\n");
}

static void blob_xfer_cli_end(struct bt_mesh_blob_cli *b, const struct bt_mesh_blob_xfer *xfer,
			     bool success)
{
	if (success) {
		printk("BLOB Transfer TX Success\n");
		return;
	}
	printk("BLOB Transfer TX Fail\n");
}

/* API */

int blob_tx_flash_init(off_t offset, size_t size)
{
	int err = 0;

	err = bt_mesh_blob_io_flash_init(&blob_tx.blob_tx_flash_stream, BLOB_AREA_ID,
					 ROUND_DOWN(offset, 4));

	if (err)
		return err;

	blob_tx.xfer.size = ROUND_UP(size, 4);

	return 0;
}

int blob_tx_flash_is_init(void)
{
	return !!blob_tx.xfer.size && blob_tx.blob_tx_flash_stream.offset >= STORAGE_START;
}

void blob_tx_targets_init(int targets[], int target_count)
{
	int i;

	sys_slist_init(&blob_tx.inputs.targets);
	for (i = 0; i < target_count; i++) {
		uint16_t addr = targets[i];
		memset(&blob_tx.targets[i], 0, sizeof(struct bt_mesh_blob_target));
		memset(&blob_tx.pull[i], 0, sizeof(struct bt_mesh_blob_target_pull));
		blob_tx.targets[i].addr = addr;
		blob_tx.targets[i].pull = &blob_tx.pull[i];

		sys_slist_append(&blob_tx.inputs.targets, &blob_tx.targets[i].n);
	}
}

int blob_tx_start(uint64_t blob_id, uint16_t base_timeout, uint16_t group, uint16_t appkey_index,
		  uint8_t ttl, enum bt_mesh_blob_xfer_mode mode)
{
	blob_tx.inputs.app_idx = appkey_index;
	blob_tx.inputs.group = group;
	blob_tx.inputs.ttl = ttl;
	blob_tx.inputs.timeout_base = base_timeout;
	blob_tx.xfer.id = blob_id;
	blob_tx.xfer.mode = mode;

	hash_flash_area(blob_tx.blob_tx_flash_stream.area_id, blob_tx.blob_tx_flash_stream.offset,
			blob_tx.xfer.size);

	return bt_mesh_blob_cli_caps_get(blob_cli, &blob_tx.inputs);
}

int blob_tx_cancel(void)
{
	if (!bt_mesh_blob_cli_is_busy(blob_cli))
		return -1;

	bt_mesh_blob_cli_cancel(blob_cli);

	return 0;
}

int blob_tx_suspend(void)
{
	return bt_mesh_blob_cli_suspend(blob_cli);;
}

int blob_tx_resume(void)
{
	return bt_mesh_blob_cli_resume(blob_cli);;
}

int blob_rx_flash_init(off_t offset)
{
	int err = 0;

	err = bt_mesh_blob_io_flash_init(&blob_rx_flash_stream, BLOB_AREA_ID,
					 ROUND_DOWN(offset, 4));

	if (err)
		return err;

	return 0;
}

int blob_rx_flash_is_init(void)
{
	return blob_rx_flash_stream.offset >= STORAGE_START;
}

int blob_rx_start(uint64_t blob_id, uint16_t base_timeout, uint8_t ttl)
{
	return bt_mesh_blob_srv_recv(blob_srv, blob_id, &blob_rx_flash_stream.io, ttl,
				     base_timeout);
}

int blob_rx_cancel(void)
{
	return bt_mesh_blob_srv_cancel(blob_srv);
}

int blob_rx_progress(void)
{
	if (!bt_mesh_is_provisioned())
		return -1;

	return bt_mesh_blob_srv_progress(blob_srv);
}

static const struct bt_mesh_blob_srv_cb blob_srv_cb = {
	.start = blob_xfer_srv_start,
	.end = blob_xfer_srv_end,
	.suspended = blob_xfer_srv_suspended,
	.resume = blob_xfer_srv_resume,
	.recover = blob_xfer_srv_recover
};

static const struct bt_mesh_blob_cli_cb blob_cli_cb = {
	.caps = blob_xfer_cli_caps,
	.lost_target = blob_xfer_cli_lost_target,
	.suspended = blob_xfer_cli_suspended,
	.end = blob_xfer_cli_end
};

void blob_xfer_srv_init(struct bt_mesh_blob_srv *srv)
{
	srv->cb = &blob_srv_cb;
	blob_srv = srv;
}

void blob_xfer_cli_init(struct bt_mesh_blob_cli *cli)
{
	cli->cb = &blob_cli_cb;
	cli->xfer = &blob_tx.xfer,
	cli->inputs = &blob_tx.inputs,
	cli->io = &blob_tx.blob_tx_flash_stream.io,
	blob_cli = cli;
}

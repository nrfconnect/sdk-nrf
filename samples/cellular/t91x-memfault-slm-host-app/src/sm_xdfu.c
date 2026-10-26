/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psa/crypto.h>
#include <modem_update_decode.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/pm/device_runtime.h>

#include "sm_xdfu.h"

LOG_MODULE_REGISTER(sm_xdfu, LOG_LEVEL_INF);

#define XDFU_CHUNK_SIZE 4096
#define XDFU_WRITE_DONE BIT(0)
#define XDFU_APPLY_DONE BIT(1)
#define XDFU_FAILURE BIT(2)
#define XDFU_READY BIT(3)
#define XDFU_BOOTLOADER_READY BIT(4)
#define XDFU_VERSION_READY BIT(5)
#define XDFU_WRITE_TIMEOUT K_SECONDS(10)

struct xdfu_full_image {
	struct Segments segments;
	size_t header_length;
	uint8_t metadata[1024];
};

struct xdfu_context {
	struct modem_chat chat;
	struct k_event events;
	struct modem_pipe *pipe;
	enum sm_xdfu_image_type type;
	bool data_mode;
	struct xdfu_full_image *full;
	char slm_version[64];
	uint8_t receive_buf[1024];
	uint8_t command[96];
	uint8_t *argv[4];
	uint8_t transfer_buf[256];
};

static K_MUTEX_DEFINE(xdfu_lock);

static bool xdfu_parse_number(const char *text, long *value)
{
	char *end;

	errno = 0;
	*value = strtol(text, &end, 10);
	if (end == text || errno == ERANGE) {
		return false;
	}
	while (*end == ' ' || *end == '\t') {
		end++;
	}
	return *end == '\0';
}

static void xdfu_on_result(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct xdfu_context *ctx = user_data;
	long type;
	long operation;
	long status;

	ARG_UNUSED(chat);
	if (argc != 4 || !xdfu_parse_number(argv[1], &type) ||
	    !xdfu_parse_number(argv[2], &operation) || !xdfu_parse_number(argv[3], &status)) {
		k_event_post(&ctx->events, XDFU_FAILURE);
		return;
	}
	if (type != ctx->type) {
		return;
	}
	if (status != 0) {
		k_event_post(&ctx->events, XDFU_FAILURE);
	} else if (operation == 1) {
		k_event_post(&ctx->events, XDFU_WRITE_DONE);
	} else if (operation == 2) {
		k_event_post(&ctx->events, XDFU_APPLY_DONE);
	}
}

static void xdfu_on_ready(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct xdfu_context *ctx = user_data;

	ARG_UNUSED(chat);
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);
	k_event_post(&ctx->events, XDFU_READY);
}

static void xdfu_on_bootloader_ready(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	struct xdfu_context *ctx = user_data;

	ARG_UNUSED(chat);
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);
	k_event_post(&ctx->events, XDFU_BOOTLOADER_READY);
}

static void xdfu_on_slm_version(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	struct xdfu_context *ctx = user_data;
	const char *version;
	size_t length;

	ARG_UNUSED(chat);
	if (argc < 2) {
		k_event_post(&ctx->events, XDFU_FAILURE);
		return;
	}
	version = argv[1];
	while (*version == ' ' || *version == '\t' || *version == '"') {
		version++;
	}
	length = strcspn(version, "\"\r\n");
	if (length == 0 || length >= sizeof(ctx->slm_version)) {
		k_event_post(&ctx->events, XDFU_FAILURE);
		return;
	}
	memcpy(ctx->slm_version, version, length);
	ctx->slm_version[length] = '\0';
	k_event_post(&ctx->events, XDFU_VERSION_READY);
}

MODEM_CELLULAR_COMMON_CHAT_MATCHES();
MODEM_CHAT_MATCHES_DEFINE(xdfu_unsol,
	MODEM_CHAT_MATCH("#XDATAMODE: ", ",", NULL),
	MODEM_CHAT_MATCH("#XDFU:", ",", xdfu_on_result),
	MODEM_CHAT_MATCH("#XSMVER: ", ",", xdfu_on_slm_version),
	MODEM_CHAT_MATCH("Bootloader mode ready", NULL, xdfu_on_bootloader_ready),
	MODEM_CHAT_MATCH("Ready", NULL, xdfu_on_ready));

static int xdfu_wait(struct xdfu_context *ctx, uint32_t event, k_timeout_t timeout)
{
	uint32_t result = k_event_wait(&ctx->events, event | XDFU_FAILURE, false, timeout);

	if (result & XDFU_FAILURE) {
		return -EIO;
	}
	return (result & event) ? 0 : -ETIMEDOUT;
}

static int xdfu_command(struct xdfu_context *ctx, const char *format, ...)
{
	va_list args;
	int length;

	va_start(args, format);
	length = vsnprintf((char *)ctx->command, sizeof(ctx->command), format, args);
	va_end(args);
	if (length < 0 || length >= sizeof(ctx->command)) {
		return -ENOBUFS;
	}

	struct modem_chat_script_chat command = {
		.request = ctx->command,
		.request_size = length,
		.response_matches = &ok_match,
		.response_matches_size = 1,
		.timeout = 1000,
	};
	struct modem_chat_script script = {
		.name = "xdfu_command",
		.script_chats = &command,
		.script_chats_size = 1,
		.abort_matches = abort_matches,
		.abort_matches_size = ARRAY_SIZE(abort_matches),
		.timeout = 30,
	};

	return modem_chat_run_script(&ctx->chat, &script);
}

static int xdfu_transmit(struct xdfu_context *ctx, const uint8_t *buffer, size_t length)
{
	k_timepoint_t deadline = sys_timepoint_calc(XDFU_WRITE_TIMEOUT);

	while (length > 0) {
		int written = modem_pipe_transmit(ctx->pipe, buffer, length);

		if (written < 0) {
			return written;
		}
		buffer += written;
		length -= written;
		if (length == 0) {
			return 0;
		}
		if (sys_timepoint_expired(deadline)) {
			return -ETIMEDOUT;
		}
		if (written == 0) {
			k_msleep(1);
		}
	}
	return 0;
}

static int xdfu_write_chunk(struct xdfu_context *ctx, struct fs_file_t *file, size_t length)
{
	while (length > 0) {
		size_t count = MIN(length, sizeof(ctx->transfer_buf));
		ssize_t bytes_read = fs_read(file, ctx->transfer_buf, count);

		if (bytes_read <= 0) {
			return bytes_read < 0 ? (int)bytes_read : -ENODATA;
		}
		int err = xdfu_transmit(ctx, ctx->transfer_buf, bytes_read);

		if (err < 0) {
			return err;
		}
		length -= bytes_read;
	}
	return 0;
}

static int xdfu_write_range(struct xdfu_context *ctx, struct fs_file_t *file,
			    uint32_t address, size_t length)
{
	for (size_t written = 0; written < length;) {
		size_t count = MIN(length - written, XDFU_CHUNK_SIZE);
		int err;

		k_event_clear(&ctx->events, XDFU_WRITE_DONE | XDFU_FAILURE);
		ctx->data_mode = true;
		err = xdfu_command(ctx, "AT#XDFUWRITE=%d,%u,%zu", ctx->type, address, count);
		if (err < 0) {
			return err;
		}
		err = xdfu_write_chunk(ctx, file, count);
		if (err < 0) {
			return err;
		}
		err = xdfu_wait(ctx, XDFU_WRITE_DONE, XDFU_WRITE_TIMEOUT);
		if (err < 0) {
			return err;
		}
		ctx->data_mode = false;
		written += count;
		address += count;
		LOG_INF("nRF91 XDFU type %d: %zu/%zu bytes", ctx->type, written, length);
	}
	return 0;
}

static int xdfu_apply(struct xdfu_context *ctx)
{
	int err;

	k_event_clear(&ctx->events, XDFU_APPLY_DONE | XDFU_FAILURE);
	err = xdfu_command(ctx, "AT#XDFUAPPLY=%d", ctx->type);
	if (err == 0) {
		err = xdfu_wait(ctx, XDFU_APPLY_DONE, K_SECONDS(30));
	}
	return err;
}

static int xdfu_prepare_full(struct xdfu_context *ctx, struct fs_file_t *file,
			     off_t offset, size_t length)
{
	struct COSE_Sign1_Manifest wrapper;
	psa_hash_operation_t hash = PSA_HASH_OPERATION_INIT;
	uint8_t digest[32];
	size_t bytes_read = 0;
	size_t decoded_length;
	size_t remaining;
	int err;

	ctx->full = calloc(1, sizeof(*ctx->full));
	if (!ctx->full) {
		return -ENOMEM;
	}
	while (bytes_read < MIN(length, sizeof(ctx->full->metadata))) {
		ssize_t count = fs_read(file, ctx->full->metadata + bytes_read,
				MIN(length, sizeof(ctx->full->metadata)) - bytes_read);

		if (count <= 0) {
			return count < 0 ? (int)count : -ENODATA;
		}
		bytes_read += count;
	}
	if (cbor_decode_Wrapper(ctx->full->metadata, bytes_read, &wrapper,
				&ctx->full->header_length) != ZCBOR_SUCCESS ||
	    ctx->full->header_length >= length) {
		LOG_ERR("Invalid full-modem CBOR wrapper");
		return -EBADMSG;
	}
	const struct Manifest *manifest = &wrapper.COSE_Sign1_Manifest_payload_cbor;

	if (manifest->Manifest_blob_hash.len != sizeof(digest) ||
	    cbor_decode_Segments(manifest->Manifest_segments.value,
				 manifest->Manifest_segments.len, &ctx->full->segments,
				 &decoded_length) != ZCBOR_SUCCESS ||
	    decoded_length != manifest->Manifest_segments.len ||
	    ctx->full->segments.Segments_Segment_m_count < 2) {
		LOG_ERR("Invalid full-modem segment table");
		return -EBADMSG;
	}

	remaining = length - ctx->full->header_length;
	for (size_t index = 0; index < ctx->full->segments.Segments_Segment_m_count; index++) {
		const struct Segment *segment = &ctx->full->segments.Segments_Segment_m[index];

		if (segment->Segment_len == 0 || segment->Segment_len > remaining ||
		    segment->Segment_len > UINT32_MAX - segment->Segment_target_addr) {
			LOG_ERR("Full-modem segment %zu is outside the supplied image range", index);
			return -EBADMSG;
		}
		remaining -= segment->Segment_len;
	}
	if (remaining != 0) {
		LOG_ERR("Full-modem segment sizes do not match the image size");
		return -EBADMSG;
	}
	err = fs_seek(file, offset + ctx->full->header_length, FS_SEEK_SET);
	if (err < 0) {
		return err;
	}
	err = psa_crypto_init();
	if (err == PSA_SUCCESS) {
		err = psa_hash_setup(&hash, PSA_ALG_SHA_256);
	}
	if (err != PSA_SUCCESS) {
		return -EIO;
	}
	remaining = length - ctx->full->header_length;
	while (remaining > 0) {
		ssize_t count = fs_read(file, ctx->transfer_buf,
				MIN(remaining, sizeof(ctx->transfer_buf)));

		if (count <= 0) {
			(void)psa_hash_abort(&hash);
			return count < 0 ? (int)count : -ENODATA;
		}
		err = psa_hash_update(&hash, ctx->transfer_buf, count);
		if (err != PSA_SUCCESS) {
			(void)psa_hash_abort(&hash);
			return -EIO;
		}
		remaining -= count;
	}
	err = psa_hash_finish(&hash, digest, sizeof(digest), &decoded_length);
	(void)psa_hash_abort(&hash);
	if (err != PSA_SUCCESS || decoded_length != sizeof(digest)) {
		return -EIO;
	}
	if (memcmp(digest, manifest->Manifest_blob_hash.value, sizeof(digest)) != 0) {
		LOG_ERR("Full-modem firmware blob SHA-256 mismatch");
		return -EBADMSG;
	}
	return fs_seek(file, offset + ctx->full->header_length, FS_SEEK_SET);
}

static int xdfu_full_run(struct xdfu_context *ctx, struct fs_file_t *file)
{
	int err;

	k_event_clear(&ctx->events, XDFU_BOOTLOADER_READY | XDFU_READY | XDFU_FAILURE);
	err = xdfu_command(ctx, "AT#XDFUINIT=2");
	if (err < 0) {
		return err;
	}
	err = xdfu_wait(ctx, XDFU_BOOTLOADER_READY, K_SECONDS(30));
	if (err < 0) {
		return err;
	}
	LOG_WRN("Full-modem update: interrupted programming requires a complete retry");
	for (size_t index = 0; index < ctx->full->segments.Segments_Segment_m_count; index++) {
		const struct Segment *segment = &ctx->full->segments.Segments_Segment_m[index];

		LOG_INF("Full-modem segment %zu/%zu: address 0x%x, size %u", index + 1,
			ctx->full->segments.Segments_Segment_m_count,
			segment->Segment_target_addr, segment->Segment_len);
		err = xdfu_write_range(ctx, file, segment->Segment_target_addr, segment->Segment_len);
		if (err < 0) {
			return err;
		}
		if (index == 0) {
			err = xdfu_apply(ctx);
			if (err < 0) {
				return err;
			}
		}
	}
	k_event_clear(&ctx->events, XDFU_READY | XDFU_FAILURE);
	err = xdfu_apply(ctx);
	if (err == 0) {
		err = xdfu_wait(ctx, XDFU_READY, K_SECONDS(120));
	}
	return err;
}

int sm_xdfu_run(const struct device *modem, enum sm_xdfu_image_type type,
	       const char *file, off_t offset, size_t length)
{
	struct fs_dirent entry;
	struct fs_file_t fd;
	struct xdfu_context *ctx;
	bool attached = false;
	bool initialized = false;
	int err;

	if (!modem || !file || offset < 0 || length == 0 ||
	    (type != SM_XDFU_TYPE_APP && type != SM_XDFU_TYPE_DELTA_MFW &&
	     type != SM_XDFU_TYPE_FULL_MFW)) {
		return -EINVAL;
	}
	err = fs_stat(file, &entry);
	if (err < 0) {
		return err;
	}
	if (entry.type != FS_DIR_ENTRY_FILE || (uint64_t)offset > entry.size ||
	    length > entry.size - (size_t)offset) {
		return -EINVAL;
	}
	if (!device_is_ready(modem)) {
		return -ENODEV;
	}

	struct modem_cellular_data *data = modem->data;
	const struct modem_cellular_config *config = modem->config;

	if (!device_is_ready(config->uart)) {
		return -ENODEV;
	}
	if (pm_device_runtime_usage(modem) > 0 || pm_device_runtime_usage(config->uart) > 0) {
		return -EBUSY;
	}
	err = k_mutex_lock(&xdfu_lock, K_NO_WAIT);
	if (err < 0) {
		return -EBUSY;
	}
	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		err = -ENOMEM;
		goto out_unlock;
	}
	k_event_init(&ctx->events);
	ctx->type = type;
	fs_file_t_init(&fd);
	err = fs_open(&fd, file, FS_O_READ);
	if (err < 0) {
		goto out_free;
	}
	err = fs_seek(&fd, offset, FS_SEEK_SET);
	if (err < 0) {
		goto out_file;
	}
	if (type == SM_XDFU_TYPE_FULL_MFW) {
		err = xdfu_prepare_full(ctx, &fd, offset, length);
		if (err < 0) {
			goto out_file;
		}
	}

	const struct modem_chat_config chat_config = {
		.receive_buf = ctx->receive_buf,
		.receive_buf_size = sizeof(ctx->receive_buf),
		.delimiter = "\r",
		.delimiter_size = 1,
		.filter = "\n",
		.filter_size = 1,
		.argv = ctx->argv,
		.argv_size = ARRAY_SIZE(ctx->argv),
		.unsol_matches = xdfu_unsol,
		.unsol_matches_size = ARRAY_SIZE(xdfu_unsol),
		.user_data = ctx,
	};

	err = modem_chat_init(&ctx->chat, &chat_config);
	if (err < 0) {
		goto out_file;
	}
	ctx->pipe = data->uart_pipe;
	err = modem_pipe_close(ctx->pipe, K_SECONDS(1));
	if (err < 0) {
		goto out_file;
	}
	modem_pipe_release(ctx->pipe);
	err = modem_pipe_open(ctx->pipe, K_SECONDS(1));
	if (err < 0) {
		goto out_file;
	}
	err = modem_chat_attach(&ctx->chat, ctx->pipe);
	if (err < 0) {
		goto out_pipe;
	}
	attached = true;
	if (type == SM_XDFU_TYPE_FULL_MFW) {
		err = xdfu_full_run(ctx, &fd);
		if (err < 0) {
			LOG_ERR("Full-modem update failed (%d); retain the bundle and retry full DFU", err);
			if (ctx->data_mode) {
				initialized = true;
				goto out_reset;
			}
		}
		goto out_chat;
	}
	err = xdfu_command(ctx, "AT#XDFUINIT=%d,%zu", type, length);
	if (err < 0) {
		goto out_chat;
	}
	initialized = true;

	err = xdfu_write_range(ctx, &fd, 0, length);
	if (err == 0) {
		err = xdfu_apply(ctx);
	}

out_reset:
	if (initialized) {
		int reset_err;

		k_event_clear(&ctx->events, XDFU_READY | XDFU_FAILURE);
		if (ctx->data_mode) {
			if (config->reset_gpio.port == NULL) {
				reset_err = -ENOTSUP;
			} else {
				reset_err = gpio_pin_set_dt(&config->reset_gpio, 1);
				if (reset_err == 0) {
					k_msleep(MAX(config->vendor->reset_pulse_duration_ms, 1));
					reset_err = gpio_pin_set_dt(&config->reset_gpio, 0);
				}
			}
		} else {
			reset_err = xdfu_command(ctx, "AT#XRESET");
		}
		if (reset_err == 0) {
			reset_err = xdfu_wait(ctx, XDFU_READY, K_SECONDS(120));
		}
		if (reset_err < 0) {
			LOG_ERR("nRF91 reset/ready failed: %d", reset_err);
			if (err == 0) {
				err = reset_err;
			}
		}
	}
out_chat:
	if (attached) {
		modem_chat_release(&ctx->chat);
	}
out_pipe: {
	int close_err = modem_pipe_close(ctx->pipe, K_SECONDS(1));

	if (err == 0) {
		err = close_err;
	}
}
out_file: {
	int close_err = fs_close(&fd);

	if (err == 0) {
		err = close_err;
	}
}
out_free:
	free(ctx->full);
	free(ctx);
out_unlock:
	k_mutex_unlock(&xdfu_lock);
	return err;
}

int sm_xdfu_get_slm_version(const struct device *modem, char *version, size_t version_size)
{
	struct modem_cellular_data *data;
	const struct modem_cellular_config *config;
	struct xdfu_context *ctx;
	bool attached = false;
	int err;

	if (!modem || !version || version_size == 0 || !device_is_ready(modem)) {
		return -EINVAL;
	}
	data = modem->data;
	config = modem->config;
	if (!device_is_ready(config->uart)) {
		return -ENODEV;
	}
	if (pm_device_runtime_usage(modem) > 0 || pm_device_runtime_usage(config->uart) > 0) {
		return -EBUSY;
	}
	err = k_mutex_lock(&xdfu_lock, K_NO_WAIT);
	if (err < 0) {
		return -EBUSY;
	}
	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		err = -ENOMEM;
		goto out_unlock;
	}
	k_event_init(&ctx->events);
	const struct modem_chat_config chat_config = {
		.receive_buf = ctx->receive_buf,
		.receive_buf_size = sizeof(ctx->receive_buf),
		.delimiter = "\r",
		.delimiter_size = 1,
		.filter = "\n",
		.filter_size = 1,
		.argv = ctx->argv,
		.argv_size = ARRAY_SIZE(ctx->argv),
		.unsol_matches = xdfu_unsol,
		.unsol_matches_size = ARRAY_SIZE(xdfu_unsol),
		.user_data = ctx,
	};
	err = modem_chat_init(&ctx->chat, &chat_config);
	if (err < 0) {
		goto out_free;
	}
	ctx->pipe = data->uart_pipe;
	err = modem_pipe_close(ctx->pipe, K_SECONDS(1));
	if (err < 0) {
		goto out_free;
	}
	modem_pipe_release(ctx->pipe);
	err = modem_pipe_open(ctx->pipe, K_SECONDS(1));
	if (err < 0) {
		goto out_pipe;
	}
	err = modem_chat_attach(&ctx->chat, ctx->pipe);
	if (err < 0) {
		goto out_pipe;
	}
	attached = true;
	k_event_clear(&ctx->events, XDFU_VERSION_READY | XDFU_FAILURE);
	err = xdfu_command(ctx, "AT#XSMVER");
	if (err == 0) {
		err = xdfu_wait(ctx, XDFU_VERSION_READY, K_SECONDS(5));
	}
	if (err == 0) {
		size_t length = strlen(ctx->slm_version);

		if (length >= version_size) {
			err = -ENOSPC;
		} else {
			memcpy(version, ctx->slm_version, length + 1);
		}
	}
	if (attached) {
		modem_chat_release(&ctx->chat);
	}
out_pipe: {
	int close_err = modem_pipe_close(ctx->pipe, K_SECONDS(1));

	if (err == 0) {
		err = close_err;
	}
}
out_free:
	free(ctx);
out_unlock:
	k_mutex_unlock(&xdfu_lock);
	return err;
}
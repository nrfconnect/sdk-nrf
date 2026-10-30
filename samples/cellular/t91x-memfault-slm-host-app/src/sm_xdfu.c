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
#define XDFU_WRITE_TIMEOUT K_SECONDS(10)

struct xdfu_context {
	struct modem_chat chat;
	struct k_event events;
	struct modem_pipe *pipe;
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
	if (type != 0) {
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

MODEM_CELLULAR_COMMON_CHAT_MATCHES();
MODEM_CHAT_MATCHES_DEFINE(xdfu_unsol,
	MODEM_CHAT_MATCH("#XDATAMODE: ", ",", NULL),
	MODEM_CHAT_MATCH("#XDFU:", ",", xdfu_on_result),
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

int sm_xdfu_app_run(const struct device *modem, const char *file, off_t offset, size_t length)
{
	struct fs_dirent entry;
	struct fs_file_t fd;
	struct xdfu_context *ctx;
	bool attached = false;
	bool initialized = false;
	bool data_mode = false;
	int err;

	if (!modem || !file || offset < 0 || length == 0) {
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
	fs_file_t_init(&fd);
	err = fs_open(&fd, file, FS_O_READ);
	if (err < 0) {
		goto out_free;
	}
	err = fs_seek(&fd, offset, FS_SEEK_SET);
	if (err < 0) {
		goto out_file;
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
	err = xdfu_command(ctx, "AT#XDFUINIT=0,%zu", length);
	if (err < 0) {
		goto out_chat;
	}
	initialized = true;

	for (size_t written = 0; written < length;) {
		size_t count = MIN(length - written, XDFU_CHUNK_SIZE);

		k_event_clear(&ctx->events, XDFU_WRITE_DONE | XDFU_FAILURE);
		data_mode = true;
		err = xdfu_command(ctx, "AT#XDFUWRITE=0,%zu,%zu", written, count);
		if (err < 0) {
			goto out_reset;
		}
		err = xdfu_write_chunk(ctx, &fd, count);
		if (err < 0) {
			goto out_reset;
		}
		err = xdfu_wait(ctx, XDFU_WRITE_DONE, XDFU_WRITE_TIMEOUT);
		if (err < 0) {
			goto out_reset;
		}
		data_mode = false;
		written += count;
		LOG_INF("nRF91 application: %zu/%zu bytes", written, length);
	}

	k_event_clear(&ctx->events, XDFU_APPLY_DONE | XDFU_FAILURE);
	err = xdfu_command(ctx, "AT#XDFUAPPLY=0");
	if (err == 0) {
		err = xdfu_wait(ctx, XDFU_APPLY_DONE, K_SECONDS(30));
	}

out_reset:
	if (initialized) {
		int reset_err;

		k_event_clear(&ctx->events, XDFU_READY | XDFU_FAILURE);
		if (data_mode) {
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
	free(ctx);
out_unlock:
	k_mutex_unlock(&xdfu_lock);
	return err;
}
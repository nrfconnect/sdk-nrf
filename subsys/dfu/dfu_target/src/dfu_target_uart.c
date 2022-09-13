/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/types.h>
#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <dfu/dfu_target_uart.h>
#include <dfu_target_uart_encode.h>
#include <dfu_target_uart_decode.h>

LOG_MODULE_REGISTER(dfu_target_uart, CONFIG_DFU_TARGET_LOG_LEVEL);

#define RESP_TIMEOUT K_MSEC(CONFIG_DFU_TARGET_UART_TIMEOUT)

RING_BUF_DECLARE(rx_rb, 16);

static K_SEM_DEFINE(rx_sem, 0, 1);
static size_t out_cursor;
static const struct dfu_target_uart_params *target_params;
static bool first_packet;
static const struct device *uart_dev;

static const struct device *find_interface(uint32_t id)
{
	__ASSERT(target_params, "The target type is not configured");

	struct dfu_target_uart_remote *cursor;

	for (int i = 0; i < target_params->remote_count; i++) {
		cursor = target_params->remotes + i;
		__ASSERT(cursor->device, "The device is not set");
		if (cursor->identifier == id) {
			return cursor->device;
		}
	}

	return NULL;
}

static void send_out_buffer(void)
{
	__ASSERT(uart_dev, "No UART set");

	for (int i = 0; i < out_cursor; i++) {
		uart_poll_out(uart_dev, target_params->buffer[i]);
	}

	out_cursor = 0;
}

static void uart_cb(const struct device *x, void *p)
{
	ARG_UNUSED(p);

	int rx;
	uint32_t ret;
	static uint8_t byte[10];

	while (uart_irq_update(x) && uart_irq_rx_ready(x)) {
		rx = uart_fifo_read(x, byte, ARRAY_SIZE(byte));
		if (rx > 0) {
			ret = ring_buf_put(&rx_rb, byte, rx);
			if (ret != rx) {
				k_sem_give(&rx_sem);
				break;
			}
		}
	}
	k_sem_give(&rx_sem);
}

static int wait_resp(struct DFU_UART_target_RemoteResp *resp,
		     k_timeout_t timeout)
{
	int err;
	uint8_t *cursor;
	size_t cursor_len;
	size_t dec_len = 0;

	while (true) {
		err = k_sem_take(&rx_sem, timeout);
		if (err) {
			return err;
		}

		cursor_len = ring_buf_get_claim(&rx_rb, &cursor, UINT32_MAX);
		if (cursor_len == 0) {
			return -ENODATA;
		}

		err = cbor_decode_DFU_UART_target_RemoteResp(
			cursor,
			cursor_len,
			resp,
			&dec_len);
		if (err) {
			ring_buf_get_finish(&rx_rb, 0);
			/* Try again */
			continue;
		}

		break;
	}

	LOG_DBG("Valid response of length %zu received", dec_len);
	ring_buf_get_finish(&rx_rb, dec_len);
	return 0;
}

static int check_response(struct DFU_UART_target_RemoteResp *resp,
			  uint8_t func,
			  size_t *offset)
{
	int err;

	if (resp->type_choice != func) {
		LOG_ERR("Wrong function type received: %d instead of %d",
			resp->type_choice, func);
		return -EBADMSG;
	}

	if (resp->type_choice != _DFU_UART_target_RemoteFnc_type_offset) {
		return resp->error;
	}

	err = resp->error;
	if (err < 0) {
		return err;
	}

	*offset = err;
	return 0;
}

static int parse_envelope(struct DFU_UART_target_Envelope *env,
			  const uint8_t *buffer, size_t size)
{
	int err;
	size_t olen;

	err = cbor_decode_DFU_UART_target_Envelope(buffer,
						  size,
						  env,
						  &olen);
	if (err) {
		return err;
	}

	return olen;
}

static int acquire_uart(const struct DFU_UART_target_Envelope *env)
{
	uart_dev = find_interface(env->rem_target);
	if (!device_is_ready(uart_dev)) {
		LOG_ERR("Could not get for remote target %d", env->rem_target);
		return -ENOSYS;
	}

	uart_irq_callback_set(uart_dev, uart_cb);
	uart_irq_rx_enable(uart_dev);

	return 0;
}

static int send_fragment(const void *buf, size_t size)
{
	int len;
	int ret;
	struct DFU_UART_target_Envelope envelope;

	struct DFU_UART_target_RemoteFnc fnc = {
		.type_choice = _DFU_UART_target_RemoteFnc_type_write,
		.data_present = true,
		.data = {
			.data_choice = _DFU_UART_target_RemoteFnc_data_payload,
		}
	};

	if (first_packet) {
		len = parse_envelope(&envelope, buf, size);
		if (len < 0) {
			LOG_ERR("Could not parse envelope, error %d", len);
			LOG_HEXDUMP_ERR(buf, size, "Malformed envelope");
			return len;
		}

		/* Remove envelope */
		LOG_DBG("Removing %d bytes from packet", len);
		buf += len;
		size -= len;

		first_packet = false;
	}

	fnc.data.payload.value = buf;
	fnc.data.payload.len = size;

	ret = cbor_encode_DFU_UART_target_RemoteFnc(target_params->buffer,
						    target_params->buf_size,
						    &fnc,
						    &out_cursor);
	if (ret) {
		LOG_ERR("Could not encode fragment, error %d", ret);
		return ret;
	}

	send_out_buffer();
	return 0;
}

static int send_init(size_t fs, uint8_t im)
{
	int err;
	struct DFU_UART_target_RemoteResp resp = {0};
	const struct DFU_UART_target_RemoteFnc fnc = {
		.type_choice = _DFU_UART_target_RemoteFnc_type_init,
		.data = {
			.file_size = fs,
			.image_num = im,
			.data_choice = _data__file_size
		},
		.data_present = 1
	};

	err = cbor_encode_DFU_UART_target_RemoteFnc(target_params->buffer,
						    target_params->buf_size,
						    &fnc,
						    &out_cursor);
	if (err) {
		return err;
	}

	send_out_buffer();

	err = wait_resp(&resp, RESP_TIMEOUT);
	if (err) {
		return err;
	}

	return check_response(
		&resp,
		_DFU_UART_target_RemoteFnc_type_init,
		NULL);
}

static int send_offset(void)
{
	int err;
	const struct DFU_UART_target_RemoteFnc fnc = {
		.type_choice = _DFU_UART_target_RemoteFnc_type_offset,
		.data_present = 0
	};

	err = cbor_encode_DFU_UART_target_RemoteFnc(target_params->buffer,
						    target_params->buf_size,
						    &fnc,
						    &out_cursor);
	if (err) {
		LOG_ERR("Could not encode CBOR with type offset, error %d",
			err);
		return err;
	}

	send_out_buffer();
	return 0;
}

static int send_done(bool success)
{
	int err;
	const struct DFU_UART_target_RemoteFnc fnc = {
		.type_choice = _DFU_UART_target_RemoteFnc_type_done,
		.data_present = 1,
		.data = {
			.data_choice = _DFU_UART_target_RemoteFnc_data_success,
			.success = success,
		}
	};

	err = cbor_encode_DFU_UART_target_RemoteFnc(target_params->buffer,
						    target_params->buf_size,
						    &fnc,
						    &out_cursor);
	if (err) {
		return err;
	}

	send_out_buffer();
	return 0;
}

bool dfu_target_uart_identify(const void *const buf, size_t len)
{
	int err;
	struct DFU_UART_target_Envelope envelope;

	err = parse_envelope(&envelope, buf, len);
	if (err < 0) {
		LOG_ERR("Could not parse envelope, error %d", err);
		return false;
	}

	if (envelope.magic != DFU_UART_HEADER_MAGIC) {
		LOG_WRN("Wrong magic number: %u instead of %u", envelope.magic,
			DFU_UART_HEADER_MAGIC);
		return false;
	}

	err = acquire_uart(&envelope);
	if (err) {
		return false;
	}

	return true;
}

int
dfu_target_uart_init(size_t size, int img_num, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);

	LOG_DBG("DFU target UART init with file size: %zu, image num: %d",
		size, img_num);

	/* This flag needs to be reset so the write function knows it must remove the envelope */
	first_packet = true;

	return send_init(size, img_num);
}

int dfu_target_uart_offset_get(size_t *offset)
{
	int err;
	struct DFU_UART_target_RemoteResp resp = {0};

	LOG_DBG("DFU target UART get offset");

	err = send_offset();
	if (err) {
		LOG_ERR("Could not send offset message, error %d", err);
		return err;
	}

	err = wait_resp(&resp, RESP_TIMEOUT);
	if (err) {
		return err;
	}

	return check_response(
		&resp,
		_DFU_UART_target_RemoteFnc_type_offset,
		offset);
}

int dfu_target_uart_write(const void *const buf, size_t len)
{
	int err;
	struct DFU_UART_target_RemoteResp resp = {0};

	LOG_DBG("Sending firmware fragment of length: %d", len);
	err = send_fragment(buf, len);
	if (err) {
		return err;
	}

	err = wait_resp(&resp, RESP_TIMEOUT);
	if (err) {
		return err;
	}

	return check_response(
		&resp,
		_DFU_UART_target_RemoteFnc_type_write,
		NULL);
}

int dfu_target_uart_done(bool successful)
{
	int err;
	struct DFU_UART_target_RemoteResp resp = {0};

	err = send_done(successful);
	if (err) {
		LOG_ERR("Could not send done message, error %d", err);
		return err;
	}

	err = wait_resp(&resp, RESP_TIMEOUT);
	if (err) {
		return err;
	}

	return check_response(
		&resp,
		_DFU_UART_target_RemoteFnc_type_done,
		NULL);
}

int dfu_target_uart_schedule_update(int img_num)
{
	ARG_UNUSED(img_num);

	return 0;
}

int dfu_target_uart_cfg(const struct dfu_target_uart_params *params)
{
	__ASSERT_NO_MSG(params);
	__ASSERT_NO_MSG(params->buffer);
	__ASSERT_NO_MSG(params->remotes);
	__ASSERT_NO_MSG(params->remote_count);

	target_params = params;
	return 0;
}

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <drivers/uart.h>
#include <string.h>
#include <dfu_target_uart_host.h>
#include <dfu/dfu_target_mcuboot.h>
#include <dfu/dfu_target.h>
#include <zephyr/logging/log.h>
#include <stdnoreturn.h>
#include <dfu_target_uart_encode.h>
#include <dfu_target_uart_decode.h>
#include <zephyr/sys/ring_buffer.h>

LOG_MODULE_REGISTER(dfu_t_uart_host, CONFIG_DFU_TARGET_UART_HOST_LOG_LEVEL);

static K_THREAD_STACK_DEFINE(uart_handler_stack,
		      CONFIG_DFU_TARGET_UART_HOST_HANDLER_STACK_SIZE);
static struct k_thread uart_handler_thread;
static uint8_t in_buf[CONFIG_DFU_TARGET_UART_HOST_IN_BUFFER_SIZE];

static struct dfu_target_uart_host_ctx {
	bool init_done;

	/* Identifier of the UART host thread */
	k_tid_t tid;

	/* The UART device used for the DFU */
	const struct device *uart_dev;

	/* The ring buffer used to store received from the UART bus */
	struct ring_buf rx_rb;

	/* Semaphore used to wait for data from UART bus */
	struct k_sem lte_uart_sem;

	size_t file_size;
	uint8_t image_num;
	bool download_success;

	/* Chunk of upgrade application to write */
	const uint8_t *to_write;
	size_t to_write_len;

	/* The output buffer that will be sent to the host core */
	uint8_t out_buffer[CONFIG_DFU_TARGET_UART_HOST_OUT_BUFFER_SIZE];
	size_t out_buf_cursor;

	bool stop_thread;
	void (*on_done)(bool success);
} context;

static void send_out_buffer(void)
{
	LOG_DBG("Sending %zu bytes", context.out_buf_cursor);

	for (int i = 0; i < context.out_buf_cursor; i++) {
		uart_poll_out(context.uart_dev, context.out_buffer[i]);
	}

	context.out_buf_cursor = 0;
}

static void send_error(int func, int error)
{
	int err;

	const struct DFU_UART_target_RemoteResp fnc = {
		.type_choice = func,
		.error = error
	};

	err = cbor_encode_DFU_UART_target_RemoteResp(
		context.out_buffer,
		ARRAY_SIZE(context.out_buffer),
		&fnc,
		&context.out_buf_cursor);
	if (err) {
		LOG_ERR("Could not encode response, error %d", err);
		return;
	}

	if (context.out_buf_cursor == 0) {
		LOG_ERR("Could not send error, no data in buffer");
		return;
	}

	send_out_buffer();
}

static void send_offset(int error, size_t offset)
{
	int err;
	int val = error == 0 ? (int) offset : error;

	const struct DFU_UART_target_RemoteResp fnc = {
		.type_choice = _DFU_UART_target_RemoteFnc_type_offset,
		.error = val
	};

	err = cbor_encode_DFU_UART_target_RemoteResp(
		context.out_buffer,
		ARRAY_SIZE(context.out_buffer),
		&fnc,
		&context.out_buf_cursor);
	if (err) {
		LOG_ERR("Could not encode offset, error %d", err);
		return;
	}

	if (context.out_buf_cursor == 0) {
		LOG_ERR("Could not send offset, no data in buffer");
		return;
	}

	send_out_buffer();
}

static void uart_flush(const struct device *dev)
{
	uint8_t c;

	while (uart_fifo_read(dev, &c, 1) > 0) {
		continue;
	}
}

static void uart_cb(const struct device *x, void *p)
{
	ARG_UNUSED(p);

	int rx;
	uint32_t ret;
	static uint8_t read_buf[128];

	while (uart_irq_update(context.uart_dev) &&
		uart_irq_rx_ready(context.uart_dev)) {
		rx = uart_fifo_read(x, read_buf, ARRAY_SIZE(read_buf));
		if (rx > 0) {
			ret = ring_buf_put(&context.rx_rb, read_buf, rx);
			if (ret != rx) {
				uart_flush(context.uart_dev);
				k_sem_give(&context.lte_uart_sem);
				break;
			}
		}
	}
	k_sem_give(&context.lte_uart_sem);
}

static int parse_init(const struct DFU_UART_target_RemoteFnc *fnc)
{
	context.file_size = fnc->data.file_size;
	context.image_num = fnc->data.image_num;

	return 0;
}

static int parse_write(const struct DFU_UART_target_RemoteFnc *fnc)
{
	context.to_write_len = fnc->data.payload.len;
	context.to_write = fnc->data.payload.value;

	return 0;
}

static int parse_done(const struct DFU_UART_target_RemoteFnc *fnc)
{
	context.download_success = fnc->data.success;

	return 0;
}

static void execute_input_fnc(const struct DFU_UART_target_RemoteFnc *fnc)
{
	int err;
	size_t offs_get;
	static size_t progress;

	LOG_DBG("DFU function called: %d", fnc->type_choice);

	switch (fnc->type_choice) {
	case _DFU_UART_target_RemoteFnc_type_init:
		parse_init(fnc);
		progress = 0;
		err = dfu_target_mcuboot_init(context.file_size,
					      context.image_num,
					      NULL);
		send_error(
			_DFU_UART_target_RemoteFnc_type_init,
			err);
		break;
	case _DFU_UART_target_RemoteFnc_type_write:
		parse_write(fnc);
		err = dfu_target_mcuboot_write(context.to_write,
					       context.to_write_len);
		progress += context.to_write_len;
		LOG_INF("Progress: %d %%", progress * 100 / context.file_size);
		send_error(
			_DFU_UART_target_RemoteFnc_type_write,
			err);
		break;
	case _DFU_UART_target_RemoteFnc_type_offset:
		err = dfu_target_mcuboot_offset_get(&offs_get);
		send_offset(err, offs_get);
		break;
	case _DFU_UART_target_RemoteFnc_type_done:
		parse_done(fnc);

		err = dfu_target_mcuboot_done(context.download_success);
		if (err) {
			send_error(
				_DFU_UART_target_RemoteFnc_type_done, err);
			if (context.on_done) {
				context.on_done(false);
			}
			break;
		}

		err = dfu_target_mcuboot_schedule_update(
			context.image_num);
		send_error(
			_DFU_UART_target_RemoteFnc_type_done,
			err);
		if (context.on_done) {
			context.on_done(context.download_success);
		}
		break;
	default:
		break;
	}
}

static void process_input_data(void)
{
	int err;
	uint8_t *cursor;
	size_t cursor_len;
	size_t dec_len = 0;
	struct DFU_UART_target_RemoteFnc fnc = {0};

	while (true) {
		cursor_len = ring_buf_get_claim(&context.rx_rb, &cursor, UINT32_MAX);
		if (cursor_len == 0) {
			/* Let's reset it to prevent wrapping */
			ring_buf_reset(&context.rx_rb);
			return;
		}

		/* We try to put the whole buffer in the decode function.
		 * If there is not enough data, the decode will throw an error.
		 * If there is too much data, no error will be thrown and
		 * the correct amount of decoded data will be given by the
		 * decode function
		 */
		err = cbor_decode_DFU_UART_target_RemoteFnc(
			cursor,
			cursor_len,
			&fnc,
			&dec_len);
		if (err) {
			ring_buf_get_finish(&context.rx_rb, 0);
			return;
		}

		execute_input_fnc(&fnc);

		/* We need to remove the amount of consumed data */
		err = ring_buf_get_finish(&context.rx_rb, dec_len);
		if (err) {
			LOG_ERR("ring_buf_get_finish returned %d", err);
		}
	}
}

noreturn static void uart_input_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (!context.stop_thread) {
		k_sem_take(&context.lte_uart_sem, K_FOREVER);
		process_input_data();
	}

	ring_buf_reset(&context.rx_rb);
}

int dfu_target_uart_host_start(const struct device *uart, void (*on_done)(bool success))
{
	if (!device_is_ready(uart)) {
		return -EINVAL;
	}

	context.uart_dev = uart;
	context.out_buf_cursor = 0;
	context.stop_thread = false;
	context.on_done = on_done;

	if (!context.init_done) {
		k_sem_init(&context.lte_uart_sem, 0, 1);
		ring_buf_init(&context.rx_rb, ARRAY_SIZE(in_buf), in_buf);
	}

	context.tid = k_thread_create(&uart_handler_thread, uart_handler_stack,
			      K_THREAD_STACK_SIZEOF(uart_handler_stack),
			      uart_input_handler, NULL, NULL, NULL,
			      CONFIG_DFU_TARGET_UART_HOST_HANDLER_PRIORITY,
			      0,
			      K_NO_WAIT);
	k_thread_name_set(&uart_handler_thread, "dfu_uart");

	uart_irq_callback_set(uart, uart_cb);
	uart_irq_rx_enable(uart);

	return 0;
}

int dfu_target_uart_host_stop(void)
{
	if (!context.uart_dev) {
		return -EAGAIN;
	}

	/* Release the UART */
	uart_irq_rx_disable(context.uart_dev);

	/* Set the stop flag */
	context.stop_thread = true;

	/* Give the semaphore to force a loop in the thread */
	k_sem_give(&context.lte_uart_sem);

	/* The ring buffer clean up is done by the thread directly. No need to do it here */
	return 0;
}

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: Communication interface for library */

#include <assert.h>
#include <stdint.h>

#include "uart_proc.h"
#ifdef USE_USB
#include "usb_proc.h"
#endif
#include "comm_proc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(comm);

#include "ptt_uart_api.h"

static int32_t comm_send_cb(const uint8_t *pkt, ptt_pkt_len_t len, bool add_crlf);

static void comm_symbols_process(struct text_proc_s *text_proc, const uint8_t *buf, uint32_t len);
static inline void text_process(struct text_proc_s *text_proc);
static inline void input_state_reset(struct text_proc_s *text_proc);

void comm_input_process(struct text_proc_s *text_proc, const uint8_t *buf, uint32_t len)
{
	switch (text_proc->state) {
	case INPUT_STATE_IDLE:
		if (text_proc->len != 0) {
			LOG_WRN("text_proc->len is not 0 when input state is idle");
			text_proc->len = 0;
		}

		/* start the timer after first received symbol */
		k_timer_start(&text_proc->timer, K_MSEC(COMM_PER_COMMAND_TIMEOUT), K_MSEC(0));

		text_proc->state = INPUT_STATE_WAITING_FOR_NEWLINE;

		comm_symbols_process(text_proc, buf, len);
		break;

	case INPUT_STATE_WAITING_FOR_NEWLINE:
		comm_symbols_process(text_proc, buf, len);
		break;

	case INPUT_STATE_TEXT_PROCESSING:
		LOG_WRN("received a command when processing previous one, discarded");
		break;

	default:
		LOG_ERR("incorrect input state");
		break;
	}
}

K_FIFO_DEFINE(text_processing_fifo);

void comm_text_processor_fn(struct k_work *work)
{
	LOG_DBG("Called work queue function");
	struct text_proc_s *text_proc = CONTAINER_OF(work, struct text_proc_s, work);

	/* force string termination for further processing */
	text_proc->buf[text_proc->len] = '\0';
	LOG_DBG("received packet to lib: len %d, %s", text_proc->len, log_strdup(text_proc->buf));
	text_process(text_proc);
}

void comm_input_timeout_handler(struct k_timer *timer)
{
	LOG_DBG("push packet to lib via timer");
	struct text_proc_s *text_proc = CONTAINER_OF(timer, struct text_proc_s, timer);

	k_work_submit(&text_proc->work);
}

static void comm_symbols_process(struct text_proc_s *text_proc, const uint8_t *buf, uint32_t len)
{
	for (uint32_t cnt = 0; cnt < len; cnt++) {
		if (('\n' != buf[cnt]) && ('\r' != buf[cnt])) {
			text_proc->buf[text_proc->len] = buf[cnt];
			++text_proc->len;

			if (text_proc->len >= COMM_MAX_TEXT_DATA_SIZE) {
				LOG_ERR("cannot parse %d bytes received, freeing input buffer",
					COMM_MAX_TEXT_DATA_SIZE);

				/* Stop the timeout timer */
				k_timer_stop(&text_proc->timer);
				input_state_reset(text_proc);
			}
		} else {
			LOG_DBG("push packet to lib via newline");
			/* Stop the timeout timer */
			k_timer_stop(&text_proc->timer);
			k_work_submit(&text_proc->work);

			text_proc->state = INPUT_STATE_TEXT_PROCESSING;
			break;
		}
	}
}

static inline void input_state_reset(struct text_proc_s *text_proc)
{
	text_proc->len = 0;
	text_proc->state = INPUT_STATE_IDLE;
}

static inline void text_process(struct text_proc_s *text_proc)
{
	ptt_uart_push_packet((uint8_t *)text_proc->buf, text_proc->len);
	input_state_reset(text_proc);
}

void comm_init(void)
{
	/* won't initialize UART if the device has DUT mode only */
#if CONFIG_PTT_DUT_MODE
	LOG_INF("Device is in DUT mode only.");
#else
	LOG_INF("Init");
	uart_init();
#ifdef USE_USB
	usb_init();
#endif
	ptt_uart_init(comm_send_cb);
#endif
}

static int32_t comm_send_cb(const uint8_t *pkt, ptt_pkt_len_t len, bool add_crlf)
{
	if ((add_crlf && (len >= (COMM_TX_BUFFER_SIZE - COMM_APPENDIX_SIZE))) ||
	    (!add_crlf && (len >= COMM_TX_BUFFER_SIZE))) {
		LOG_WRN("%s: not enough of tx buffer size", __func__);
		return -1;
	}

	uart_send(pkt, len, add_crlf);

#ifdef USE_USB
	usb_send(pkt, len, add_crlf);
#endif

	return len;
}

void comm_proc(void)
{
#ifdef USE_USB
	usb_task();
#endif
}

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: communication interfaces */

#ifndef COMM_PROC_H__
#define COMM_PROC_H__

#include <stdint.h>

#include <kernel.h>

/**< Maximum expected size of received packet */
#define COMM_MAX_TEXT_DATA_SIZE (320u)

/**< Time to wait for a mark of end of a packet (new line) */
#define COMM_PER_COMMAND_TIMEOUT (500u)

/**< Maximum size of transmitted packet */
#define COMM_TX_BUFFER_SIZE (320u)

/**< Additional symbols in the end of transmitted packet */
#define COMM_APPENDIX "\r\n"
#define COMM_APPENDIX_SIZE sizeof(COMM_APPENDIX)

/**< Stated of COMM packet parser */
enum input_state {
	INPUT_STATE_IDLE = 0, /**< Wait for the first symbol */
	INPUT_STATE_WAITING_FOR_NEWLINE, /**< Wait for new line symbol or timeout */
	INPUT_STATE_TEXT_PROCESSING, /**< Passes received string to the library */
};

struct text_proc_s {
	struct k_timer timer;
	struct k_work work;
	char buf[COMM_MAX_TEXT_DATA_SIZE];
	uint16_t len;
	enum input_state state;
};

void comm_init(void);

void comm_input_process(struct text_proc_s *text_proc, const uint8_t *buf, uint32_t len);

void comm_text_processor_fn(struct k_work *work);

void comm_proc(void);

void comm_input_timeout_handler(struct k_timer *timer);

#endif /* COMM_PROC_H__ */

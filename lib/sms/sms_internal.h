/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SMS_INTERNAL_INCLUDE_H_
#define _SMS_INTERNAL_INCLUDE_H_

/** @brief Size of the internal buffer. */
#define SMS_BUF_TMP_LEN 512

/**
 * @brief Temporary buffer that can be used in various functions where data buffer is required.
 *
 * @details Will be used at least when encoding SMS-SUBMIT message.
 * Be careful when using this internal buffer so that the caller hasn't already passed the data
 * using the same buffer.
 */
extern uint8_t sms_buf_tmp[SMS_BUF_TMP_LEN];

/**
 * @brief Temporary buffer for SMS payload.
 *
 * @details Will be used at least as output for encoding and decoding of SMS payload.
 * However, this is a temporary internal buffer that can be used for other purposes too.
 * Be careful when using this internal buffer so that the caller hasn't already passed the data
 * using the same buffer.
 */
extern uint8_t sms_payload_tmp[SMS_MAX_PAYLOAD_LEN_CHARS];

#endif

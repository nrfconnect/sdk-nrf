/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SMS_SUBMIT_INCLUDE_H_
#define _SMS_SUBMIT_INCLUDE_H_

/**
 * @brief Send SMS message, which is called SMS-SUBMIT message in SMS protocol.
 *
 * SMS-SUBMIT message format is specified in 3GPP TS 23.040 chapter 9.2.2.2.
 *
 * @param[in] number Recipient number.
 * @param[in] text Text to be sent.
 *
 * @retval -EINVAL Invalid parameter.
 * @return Zero on success, otherwise error code.
 */
int sms_submit_send(const char *number, const char *text);

#endif

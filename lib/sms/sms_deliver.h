/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SMS_DELIVER_INCLUDE_H_
#define _SMS_DELIVER_INCLUDE_H_

/* Forward declaration */
struct sms_data;

/**
 * @brief Decode received SMS message, i.e., SMS-DELIVER message as specified
 * in 3GPP TS 23.040 Section 9.2.2.1.
 *
 * @param[in] pdu SMS-DELIVER PDU.
 * @param[out] out SMS message decoded into a structure.
 *
 * @retval -EINVAL Invalid parameter.
 * @retval -ENOMEM No memory to register new observers.
 * @return Zero on success, otherwise error code.
 */
int sms_deliver_pdu_parse(const char *pdu, struct sms_data *out);

#endif

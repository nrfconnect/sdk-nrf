/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_SMS_H
#define MOSH_SMS_H

#define SMS_NUMBER_NONE -1

int sms_register(void);
int sms_unregister(void);
int sms_send_msg(char *number, char *text, enum sms_data_type type);
int sms_recv(bool arg_receive_start);

#endif /* MOSH_SMS_H */

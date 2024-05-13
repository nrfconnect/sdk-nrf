/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_DEFINES_H
#define MOSH_DEFINES_H

#define MOSH_EMPTY_STRING "\0"

#define MOSH_APN_STR_MAX_LEN (64)
#define MOSH_ARG_NOT_SET -6

#define MOSH_STRING_NULL_CHECK(string) \
	((string != NULL) ? string : MOSH_EMPTY_STRING)

#define MOSH_AT_CMD_RESPONSE_MAX_LEN 2701

enum mosh_signals {
	MOSH_SIGNAL_KILL,
};

/* For location status indication, use LED1 for Thingy:91 or Thingy:91 X, and LED4 for DKs.
 * Connecting an nRF7002 EK to nRF9151 or nRF9161 DK causes GPIO conflict with LED1 and Button 1.
 */
#if defined(CONFIG_BOARD_THINGY91_NRF9160_NS) || defined(CONFIG_BOARD_THINGY91X_NRF9151_NS)
#define LOCATION_STATUS_LED            DK_LED1
#else
#define LOCATION_STATUS_LED            DK_LED4
#endif

#define GPIO_STATUS_LED                DK_LED2
#define REGISTERED_STATUS_LED          DK_LED3

#endif /* MOSH_DEFINES_H */

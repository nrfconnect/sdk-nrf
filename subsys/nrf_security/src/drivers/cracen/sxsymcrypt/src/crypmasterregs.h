/** CryptoMaster register definitions
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRYPMASTERREGS_HEADER_FILE
#define CRYPMASTERREGS_HEADER_FILE

#define REG_FETCH_ADDR	0x00
#define REG_FETCH_LEN	0x08
#define REG_FETCH_TAG	0x0C
#define REG_PUSH_ADDR	0x10
#define REG_PUSH_LEN	0x18
#define REG_INT_EN	0x1C
#define REG_INT_STATRAW 0x28
#define REG_INT_STAT	0x2C
#define REG_INT_STATCLR 0x30
#define REG_CONFIG	0x34
#define REG_CONFIG_SG	3
#define REG_START	0x38
#define REG_START_ALL	0x3
#define REG_STATUS	0x3C

#define REG_STATUS_FETCHER_BUSY_MASK	    0x01
#define REG_STATUS_PUSHER_BUSY_MASK	    0x02
#define REG_STATUS_PUSHER_WAITING_FIFO_MASK 0x20
#define REG_STATUS_BUSY_MASK                                                                       \
	(REG_STATUS_FETCHER_BUSY_MASK | REG_STATUS_PUSHER_BUSY_MASK |                              \
	 REG_STATUS_PUSHER_WAITING_FIFO_MASK)

#define REG_SOFT_RESET_ENABLE 0x10
#define REG_SOFT_RESET_BUSY   0x40

#define REG_HW_PRESENCE	     0x400
#define REG_HW_PRESENT_BA411 (1u << 0)
#define REG_HW_PRESENT_BA415 (1u << 1)
#define REG_HW_PRESENT_BA416 (1u << 2)
#define REG_HW_PRESENT_BA412 (1u << 3)
#define REG_HW_PRESENT_BA413 (1u << 4)
#define REG_HW_PRESENT_BA417 (1u << 5)
#define REG_HW_PRESENT_BA418 (1u << 6)
#define REG_HW_PRESENT_BA421 (1u << 7)
#define REG_HW_PRESENT_BA419 (1u << 8)
#define REG_HW_PRESENT_BA431 (1u << 10)
#define REG_HW_PRESENT_BA420 (1u << 11)
#define REG_HW_PRESENT_BA423 (1u << 12)
#define REG_HW_PRESENT_BA422 (1u << 13)

#define REG_BA411_CAPS	 0x404
#define REG_BA411_CTR_SZ 0x408
#define REG_BA413_CAPS	 0x40C
#define REG_BA418_CAPS	 0x410
#define REG_BA419_CAPS	 0x414
#define REG_BA424_CAPS	 0x418

/* BA413 */
/* Masks possible algorithms from capabilities register */
#define REG_BA413_CAPS_ALGO_MASK       (0x7F)
/* Masks HW padding from capabilities register */
#define REG_BA413_CAPS_HW_PADDING_MASK (1 << 16)
/* Masks HMAC from capabilities register */
#define REG_BA413_CAPS_HMAC_MASK       (1 << 17)
#define REG_BA413_CAPS_ALGO_SM3	       (1 << 6)

#endif

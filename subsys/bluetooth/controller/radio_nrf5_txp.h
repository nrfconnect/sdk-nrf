/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_TX_PWR_0)
#define RADIO_TXP_DEFAULT  0
#elif defined(CONFIG_BT_CTLR_TX_PWR_PLUS_8)
#define RADIO_TXP_DEFAULT  8
#elif defined(CONFIG_BT_CTLR_TX_PWR_PLUS_7)
#define RADIO_TXP_DEFAULT  7
#elif defined(CONFIG_BT_CTLR_TX_PWR_PLUS_6)
#define RADIO_TXP_DEFAULT  6
#elif defined(CONFIG_BT_CTLR_TX_PWR_PLUS_5)
#define RADIO_TXP_DEFAULT  5
#elif defined(CONFIG_BT_CTLR_TX_PWR_PLUS_4)
#define RADIO_TXP_DEFAULT  4
#elif defined(CONFIG_BT_CTLR_TX_PWR_PLUS_3)
#define RADIO_TXP_DEFAULT  3
#elif defined(CONFIG_BT_CTLR_TX_PWR_PLUS_2)
#define RADIO_TXP_DEFAULT  2
#elif defined(CONFIG_BT_CTLR_TX_PWR_MINUS_4)
#define RADIO_TXP_DEFAULT -4
#elif defined(CONFIG_BT_CTLR_TX_PWR_MINUS_8)
#define RADIO_TXP_DEFAULT -8
#elif defined(CONFIG_BT_CTLR_TX_PWR_MINUS_12)
#define RADIO_TXP_DEFAULT -12
#elif defined(CONFIG_BT_CTLR_TX_PWR_MINUS_16)
#define RADIO_TXP_DEFAULT -16
#elif defined(CONFIG_BT_CTLR_TX_PWR_MINUS_20)
#define RADIO_TXP_DEFAULT -20
#elif defined(CONFIG_BT_CTLR_TX_PWR_MINUS_30)
#define RADIO_TXP_DEFAULT -30
#elif defined(CONFIG_BT_CTLR_TX_PWR_MINUS_40)
#define RADIO_TXP_DEFAULT -40
#else
#error Unknown output power
#endif

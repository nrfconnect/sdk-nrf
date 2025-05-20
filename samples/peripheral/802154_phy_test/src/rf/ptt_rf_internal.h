/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: internal RF defines */

#ifndef PTT_RF_INTERNAL_H__
#define PTT_RF_INTERNAL_H__

#include <stdint.h>

#include "ptt_types.h"

#include "ctrl/ptt_events.h"

#include "rf/ptt_rf.h"

#define PTT_RF_EVT_UNLOCKED (PTT_EVENT_UNASSIGNED)
#define PTT_RF_DEFAULT_CHANNEL (PTT_CHANNEL_MIN)
/* @todo: arbitrary selected, change PTT_RF_DEFAULT_POWER to appropriate value after testing */
#define PTT_RF_DEFAULT_POWER (-20)
#define PTT_RF_DEFAULT_ANTENNA 0u

/** RF module context */
struct ptt_rf_ctx_s {
	uint8_t channel; /**< configured RF channel */
	int8_t power; /**< configured RF power */
	uint8_t antenna; /**< configured RF antenna */
	ptt_evt_id_t evt_lock; /**< current event processing by RF module */
	bool cca_on_tx; /**< perform CCA before transmission or not */
	bool stat_enabled; /**< statistic gathering enabled */
	struct ptt_rf_stat_s stat; /**< received packets statistic */
	struct ptt_ltx_payload_s ltx_payload; /**< raw payload for 'custom ltx' command */
};

#endif /* PTT_RF_INTERNAL_H__ */

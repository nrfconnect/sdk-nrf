/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nfc/tnep/tag_signalling.h>

LOG_MODULE_DECLARE(nfc_tnep_tag, CONFIG_NFC_TNEP_TAG_LOG_LEVEL);

#define TNEP_EVENT_RX_IDX 0
#define TNEP_EVENT_TX_IDX 1

struct tnep_control {
	struct k_poll_event *events;
	struct k_poll_signal msg_rx;
	struct k_poll_signal msg_tx;
};

static struct tnep_control tnep_ctrl;

int nfc_tnep_tag_signalling_init(struct k_poll_event *events, uint8_t event_cnt)
{
	if (!events) {
		return -EINVAL;
	}

	if (event_cnt != NFC_TNEP_EVENTS_NUMBER) {
		LOG_ERR("Invalid k_pool events count. Got %d events, required %d",
			event_cnt, NFC_TNEP_EVENTS_NUMBER);
		return -EINVAL;
	}

	tnep_ctrl.events = events;

	k_poll_signal_init(&tnep_ctrl.msg_rx);
	k_poll_signal_init(&tnep_ctrl.msg_tx);

	k_poll_event_init(&tnep_ctrl.events[TNEP_EVENT_RX_IDX],
			  K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &tnep_ctrl.msg_rx);

	k_poll_event_init(&tnep_ctrl.events[TNEP_EVENT_TX_IDX],
			  K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &tnep_ctrl.msg_tx);

	return 0;
}

void nfc_tnep_tag_signalling_rx_event_raise(enum tnep_event event)
{
	k_poll_signal_raise(&tnep_ctrl.msg_rx, event);
}

void nfc_tnep_tag_signalling_tx_event_raise(enum tnep_event event)
{
	k_poll_signal_raise(&tnep_ctrl.msg_tx, event);
}

static bool event_check_and_clear(int event_idx, enum tnep_event *event)
{
	if (tnep_ctrl.events[event_idx].state == K_POLL_STATE_SIGNALED) {
		*event = tnep_ctrl.events[event_idx].signal->result;

		k_poll_signal_reset(tnep_ctrl.events[event_idx].signal);
		tnep_ctrl.events[event_idx].state = K_POLL_STATE_NOT_READY;

		return true;
	}

	return false;
}

bool nfc_tnep_tag_signalling_rx_event_check_and_clear(enum tnep_event *event)
{
	return event_check_and_clear(TNEP_EVENT_RX_IDX, event);
}

bool nfc_tnep_tag_signalling_tx_event_check_and_clear(enum tnep_event *event)
{
	return event_check_and_clear(TNEP_EVENT_TX_IDX, event);
}

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_timer.h>
#include <nrfx_timer.h>
#include <esb.h>
#include "esb_common.h"
#include "esb_graph.h"
#include "esb_queues.h"

LOG_MODULE_DECLARE(esb, CONFIG_ESB_LOG_LEVEL);


#define PID_UNKNOWN 0xFF


struct prx_pipe_state {

#if CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0

	/* Queues holding next ACK payload. One queue for each pipe. */
	struct esb_fifo_list ack_payload_queue;

	/* Keeps the last ACK payload. Needed to resend the same ACK if the same
	 * PID is received again. NULL if last ACK has no payload.
	 */
	struct ack_payload_item *last_ack_payload;

#endif /* CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0 */

	/* PID of the last received packet over this pipe. If we get the same PID again,
	 * we need to resend the same ACK (without sending event to the application).
	 */
	uint8_t last_pid;
};

/** State for each ESB pipe. */
static struct prx_pipe_state pipes[CONFIG_ESB_PIPE_COUNT];

/** FIFO of received packets. */
static struct esb_fifo_ring rx_fifo;

/** Storage of packets for rx_fifo. */
static struct esb_data_payload rx_fifo_array[CONFIG_ESB_RX_FIFO_SIZE];

/** A buffer used as a destination of DMA transfers from RADIO. */
static uint8_t esb_radio_buffer[ESB_MAX_RADIO_BUFFER_SIZE];

#if CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0

/* Queue of unused ACK payload items. */
static struct esb_lifo_list ack_payload_unused;

/* Storage for ACK payload items. */
static struct ack_payload_item ack_payload_items[CONFIG_ESB_TX_FIFO_SIZE];

#endif /* CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0 */


/** Returns time between end of received packet and start of ACK for current configuration. */
static int get_rx_to_ack_time(void)
{
	if (esb_config.switching_time == ESB_SWITCHING_FAST) {
		return SW_TIME_FAST_RX_TO_ACK;
	} else if (esb_config.switching_time == ESB_SWITCHING_NORMAL) {
		return SW_TIME_NORMAL_RX_TO_ACK;
	} else {
		return SW_TIME_LEGACY_RX_TO_ACK;
	}
}


int esb_prx_write_payload(const struct esb_payload *payload)
{
#if CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0
	unsigned int key;
	struct prx_pipe_state *pipe_state;
	struct ack_payload_item *ack_item;

	if (payload == NULL || payload->pipe >= CONFIG_ESB_PIPE_COUNT
		|| (esb_address.pipes_enabled & BIT(payload->pipe)) == 0) {
		return -EINVAL;
	}

	if (payload->length > CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH) {
		return -EMSGSIZE;
	}

	pipe_state = &pipes[payload->pipe];

	key = irq_lock();
	ack_item = esb_lifo_pop(&ack_payload_unused);
	irq_unlock(key);
	if (ack_item == NULL) {
		return -ENOMEM;
	}

	memcpy(&ack_item->payload, payload, sizeof(struct esb_ack_payload));

	key = irq_lock();
	esb_fifo_push(&pipe_state->ack_payload_queue, &ack_item->queue_item);
	irq_unlock(key);

	return 0;
#else
	return -ENOSYS;
#endif /* CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0 */
}


int esb_prx_read_rx_payload(struct esb_payload *payload)
{
	struct esb_data_payload *input_payload;

	if (ESB_FIFO_COUNT(&rx_fifo, rx_fifo_array) == 0) {
		return -ENODATA;
	}
	input_payload = ESB_FIFO_PREPARE_POP(&rx_fifo, rx_fifo_array);
	memcpy(payload, input_payload, offsetof(struct esb_payload, data) + input_payload->length);
	ESB_FIFO_POP(&rx_fifo, rx_fifo_array);
	return 0;
}


/** Prepare the next ACK packet for transmission and put it into the RADIO buffer.
 *
 *  @param pipe_state State of the pipe on which the ACK is sent.
 *  @param retransmitted True if the last received packet was a retransmission.
 */
static void prepare_next_ack_packet(struct prx_pipe_state *pipe_state, bool retransmitted)
{
	struct esb_radio_dynamic_pdu *packet = (struct esb_radio_dynamic_pdu *)esb_radio_buffer;

	esb_radio_buffer[0] = 0;
	esb_radio_buffer[1] = 0;
	packet->pid = pipe_state->last_pid;
	packet->ack = 1;

#if CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0
	struct ack_payload_item *ack_item = pipe_state->last_ack_payload;

	if (!retransmitted) {
		if (ack_item != NULL) {
			esb_lifo_push(&ack_payload_unused, &ack_item->queue_item);
		}
		ack_item = esb_fifo_pop(&pipe_state->ack_payload_queue);
		pipe_state->last_ack_payload = ack_item;
	}

	if (ack_item != NULL) {
		packet->length = ack_item->payload.length;
		memcpy((uint8_t *)&packet[1], ack_item->payload.data, ack_item->payload.length);
	}
#endif /* CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0 */
}


/** Radio IRQ handler for fast and normal switching that do normal ACK handling where
 *  decision whether to switch to TX or not is done in the IRQ context.
 */
static void normal_ack_radio_irq(void)
{
	struct esb_radio_dynamic_pdu *packet = (struct esb_radio_dynamic_pdu *)esb_radio_buffer;
	struct prx_pipe_state *pipe_state;
	int pipe;
	bool retransmitted;
	bool skip_ack;

	__ASSERT(nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_CRCOK), "Unexpected interrupt.");

	/* Clear IRQ reason */
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_CRCOK);
	pipe = nrf_radio_rxmatch_get(NRF_RADIO);

	struct esb_data_payload *payload = ESB_FIFO_PREPARE_PUSH(&rx_fifo, rx_fifo_array);

	/* Make sure we allowed to receive this packet */
	if (payload == NULL || pipe >= CONFIG_ESB_PIPE_COUNT
		|| !(esb_address.pipes_enabled & BIT(pipe))) {

		nrf_timer_task_trigger(ESB_TIMER, NRF_TIMER_TASK_STOP);
		nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_START);
		return;
	}

	pipe_state = &pipes[pipe];

	/* Trigger user callback only for new PIDs, otherwise just resend ACK */
	retransmitted = (pipe_state->last_pid == packet->pid);
	if (!retransmitted) {
		pipe_state->last_pid = packet->pid;
		/* Copy and push received payload to RX FIFO */
		payload->length = packet->length;
		payload->pipe = pipe;
		payload->rssi = nrf_radio_rssi_sample_get(NRF_RADIO);
		payload->pid = packet->pid;
		payload->noack = !packet->ack;
		memcpy(payload->data, &packet[1], payload->length);
		/* Packet copied, actual pushing will be done later */
	}

	/* Determine if we need to skip ACK */
	if (esb_config.ack_sending_mode == ESB_ACK_NEVER) {
		skip_ack = true;
	} else if (esb_config.ack_sending_mode == ESB_ACK_ALWAYS) {
		skip_ack = false;
	} else {
		skip_ack = !packet->ack;
	}

	if (skip_ack) {
		/* Restart RX immediately if there is no ACK requested */
		nrf_timer_task_trigger(ESB_TIMER, NRF_TIMER_TASK_STOP);
		nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_START);

		/* Skip compare event if happened */
		esb_disable_group(2);
	} else {
		/* Prepare ACK packet */
		nrf_radio_txaddress_set(NRF_RADIO, pipe);
		prepare_next_ack_packet(pipe_state, retransmitted);

		/* Switch groups for TX */
		if (!ESB_FAST_SWITCHING_SUPPORTED) {
			esb_disable_group(0);
		}
		esb_enable_group(1);

		/* Pass compare event further */
		esb_enable_group(3);
	}

	if (!retransmitted) {
		ESB_FIFO_PUSH(&rx_fifo, rx_fifo_array);
		/* Trigger EGU to handle received packet in lower priority context */
		nrf_egu_task_trigger(ESB_EGU, NRF_EGU_TASK_TRIGGER0);
	}
}


/** Graph for fast switching that does normal ACK handling where
 *  decision whether to switch to TX or not is done in the IRQ context.
 */
static void fast_sw_normal_ack_graph(bool enable_graph, uint32_t flags)
{
	GRAPH_BEGIN(fast_sw_normal_ack_graph);

	/* Starting point: RADIO(RXEN) */

	if (enable_graph) {
		int timer_ticks = get_rx_to_ack_time() - ESB_FAST_TXRU_TIME_US;

		nrf_timer_cc_set(ESB_TIMER, 0, MAX(1, timer_ticks));
	}

	/* Simplified normal_ack_radio_irq logic:
	 * if sending ACK: ENABLE_GROUP(1), ENABLE_GROUP(3)
	 * if skipping ACK: TIMER(STOP), RADIO(START), DISABLE_GROUP(2)
	 */
	RADIO_IRQ(normal_ack_radio_irq, CRCOK);
	RADIO_SHORTS(READY, /* --> */ START,
		     ADDRESS, /* --> */ RSSISTART);
	TIMER_SHORTS(COMPARE0, /* --> */ STOP);

	CONNECT_CONTROLLED_ONE_SHOT_1_TO_1(2, 3, true, false,
		TIMER(COMPARE0), /* --> */ RADIO(TXEN));

	CONNECT_1_TO_3(true,
		RADIO(CRCOK), /* --> */ TIMER(START),
					TIMER(CLEAR),
					ENABLE_GROUP(2));

	CONNECT_1_TO_1(true,
		RADIO(CRCERROR), /* --> */ RADIO(START));

	GROUP_BEGIN(1);
	CONNECT_1_TO_2(false,
		RADIO(PHYEND), /* --> */ RADIO(RXEN),
					 DISABLE_GROUP_DECL(1));
	GROUP_END();
	DISABLE_GROUP_IMPL(1);

#if defined(RADIO_SHORTS_DISABLED_RSSISTOP_Msk)
	CONNECT_1_TO_1(true,
		RADIO(END), /* --> */ RADIO(RSSISTOP));
#endif

	GRAPH_END();
}


/** Graph for normal switching that do normal ACK handling where
 *  decision whether to switch to TX or not is done in the IRQ context.
 */
static void normal_sw_normal_ack_graph(bool enable_graph, uint32_t flags)
{
	uint8_t reuse_disabled_event;
	uint8_t reuse_disable_task;

	GRAPH_BEGIN(normal_sw_normal_ack_graph);

	/* Starting point: RADIO(RXEN) */

	if (enable_graph) {
		int timer_ticks = get_rx_to_ack_time() -
				  (ESB_NORMAL_TXRU_TIME_US + ESB_DISABLE_TIME_US);

		nrf_timer_cc_set(ESB_TIMER, 0, MAX(1, timer_ticks));
	}

	/* Simplified normal_ack_radio_irq logic:
	 * if sending ACK: DISABLE_GROUP(0), ENABLE_GROUP(1), ENABLE_GROUP(3)
	 * if skipping ACK: TIMER(STOP), RADIO(START), DISABLE_GROUP(2)
	 */
	RADIO_IRQ(normal_ack_radio_irq, CRCOK);
	RADIO_SHORTS(READY, /* --> */ START,
		     ADDRESS, /* --> */ RSSISTART);
	TIMER_SHORTS(COMPARE0, /* --> */ STOP);

	CONNECT_CONTROLLED_ONE_SHOT_1_TO_1(2, 3, true, false,
		TIMER(COMPARE0), /* --> */ REUSABLE(RADIO(DISABLE), reuse_disable_task));

	CONNECT_1_TO_3(true,
		RADIO(CRCOK), /* --> */ TIMER(START),
					TIMER(CLEAR),
					ENABLE_GROUP(2));

	GROUP_BEGIN(0);
	CONNECT_1_TO_1(true,
		REUSABLE(RADIO(DISABLED), reuse_disabled_event), /* --> */ RADIO(RXEN));
	GROUP_END();

	CONNECT_1_TO_1(true,
		RADIO(CRCERROR), /* --> */ RADIO(START));

	GROUP_BEGIN(1);
	CONNECT_1_TO_3(false,
		RADIO(PHYEND), /* --> */ REUSE(RADIO(DISABLE), reuse_disable_task),
					 ENABLE_GROUP(0),
					 DISABLE_GROUP_DECL(1));
	CONNECT_1_TO_1(false,
		REUSE(RADIO(DISABLED), reuse_disabled_event), /* --> */ RADIO(TXEN));
	GROUP_END();
	DISABLE_GROUP_IMPL(1);

#if defined(RADIO_SHORTS_DISABLED_RSSISTOP_Msk)
	CONNECT_1_TO_1(true,
		RADIO(END), /* --> */ RADIO(RSSISTOP));
#endif

	GRAPH_END();
}


/** Radio IRQ handler for fast switching that does fast ACK handling where
 *  DPPI/PPI automatically switches to TX without CPU intervention. IRQ is
 *  responsible for switching back to RX if no ACK is will be sent.
 */
static void fast_sw_fast_ack_radio_irq(void)
{
	struct esb_radio_dynamic_pdu *packet = (struct esb_radio_dynamic_pdu *)esb_radio_buffer;
	struct prx_pipe_state *pipe_state;
	int pipe;
	bool retransmitted;
	bool skip_ack;
	uint32_t initial_state;

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_READY);
	__ASSERT(nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_CRCOK), "Unexpected interrupt.");

	initial_state = nrf_radio_state_get(NRF_RADIO);

	/* Clear IRQ reason */
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_CRCOK);
	pipe = nrf_radio_rxmatch_get(NRF_RADIO);

	struct esb_data_payload *payload = ESB_FIFO_PREPARE_PUSH(&rx_fifo, rx_fifo_array);

	/* Make sure we allowed to receive this packet */
	if (payload == NULL || pipe >= CONFIG_ESB_PIPE_COUNT
		|| !(esb_address.pipes_enabled & BIT(pipe))) {

		nrf_timer_task_trigger(ESB_TIMER, NRF_TIMER_TASK_STOP);
		skip_ack = true;
		retransmitted = true;
	} else {
		if (esb_config.ack_sending_mode == ESB_ACK_ALWAYS) {
			skip_ack = false;
		} else if (esb_config.ack_sending_mode == ESB_ACK_NEVER || !packet->ack) {
			nrf_timer_task_trigger(ESB_TIMER, NRF_TIMER_TASK_STOP);
			skip_ack = true;
		} else  {
			skip_ack = false;
		}

		pipe_state = &pipes[pipe];

		/* Trigger user callback only for new PIDs, otherwise just resend ACK */
		retransmitted = (pipe_state->last_pid == packet->pid);
		if (!retransmitted) {
			pipe_state->last_pid = packet->pid;
			/* Copy and push received payload to RX FIFO */
			payload->length = packet->length;
			payload->pipe = pipe;
			payload->rssi = nrf_radio_rssi_sample_get(NRF_RADIO);
			payload->pid = packet->pid;
			payload->noack = !packet->ack;
			memcpy(payload->data, &packet[1], payload->length);
			/* Packet copied, actual pushing will be done later */
		}
	}

	if (skip_ack) {
		nrf_timer_task_trigger(ESB_TIMER, NRF_TIMER_TASK_CAPTURE5);
		if (nrf_timer_cc_get(ESB_TIMER, 5) < nrf_timer_cc_get(ESB_TIMER, 0)) {
			/* We disabled timer before triggering TX ramp-up, so we just need to
			 * restart RX.
			 */
			nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_START);
		} else {
			/* Pass READY event to RXEN task to restart RX after TXRU is done. */
			esb_enable_group(3);
		}
	} else {
		/* Prepare ACK packet */
		nrf_radio_txaddress_set(NRF_RADIO, pipe);
		prepare_next_ack_packet(pipe_state, retransmitted);

		/* Enable connections needed for sending */
		esb_enable_group(0);

		/* Pass READY event to START task*/
		esb_enable_group(2);
	}

	if (!retransmitted) {
		ESB_FIFO_PUSH(&rx_fifo, rx_fifo_array);
		/* Trigger EGU to handle received packet in lower priority context */
		nrf_egu_task_trigger(ESB_EGU, NRF_EGU_TASK_TRIGGER0);
	}
}


/** Graph for fast switching that does fast ACK handling where
 *  DPPI/PPI automatically switches to TX without CPU intervention. IRQ is
 *  responsible for switching back to RX if no ACK is will be sent.
 */
static void fast_sw_fast_ack_graph(bool enable_graph, uint32_t flags)
{
	uint8_t reuse_radio_start;
	uint8_t reuse_egu_1;

	GRAPH_BEGIN(fast_sw_fast_ack_graph);

	/* Graph starting point: RADIO(RXEN) */

	if (enable_graph) {
		int timer_ticks = get_rx_to_ack_time() - ESB_FAST_TXRU_TIME_US;

		nrf_timer_cc_set(ESB_TIMER, 0, MAX(1, timer_ticks));
	}

	/* Simplified fast_sw_fast_ack_radio_irq logic:
	 * if sending ACK: ENABLE_GROUP(0), ENABLE_GROUP(2)
	 * if skipping ACK: TIMER(STOP)
	 *     if stopped before COMPARE0: RADIO(START)
	 *     if not: ENABLE_GROUP(3)
	 */
	RADIO_IRQ(fast_sw_fast_ack_radio_irq, CRCOK);
	RADIO_SHORTS(ADDRESS, /* --> */ RSSISTART);
	TIMER_SHORTS(COMPARE0, /* --> */ STOP);

	CONNECT_CONTROLLED_ONE_SHOT_1_TO_1_AND_1(1, 2, 3, true, true, false,
		RADIO(READY), /* --2--> */ REUSABLE(RADIO(START), reuse_radio_start),
			      /* --3--> */ REUSABLE(EGU(1), reuse_egu_1));

	GROUP_BEGIN(0);
	CONNECT_1_TO_1(false,
		RADIO(PHYEND), /* --> */ REUSE(EGU(1), reuse_egu_1));
	GROUP_END();

	CONNECT_1_TO_3(true,
		EGU(1), /* --> */ ENABLE_GROUP(2),
				  RADIO(RXEN),
				  DISABLE_GROUP(0));

	CONNECT_1_TO_3(true,
		RADIO(CRCOK), /* --> */ TIMER(START),
					TIMER(CLEAR),
					ENABLE_GROUP(1));

	CONNECT_1_TO_1(true,
		TIMER(COMPARE0), /* --> */ RADIO(TXEN));

	CONNECT_1_TO_1(true,
		RADIO(CRCERROR), /* --> */ REUSE(RADIO(START), reuse_radio_start));

#if defined(RADIO_SHORTS_DISABLED_RSSISTOP_Msk)
	CONNECT_1_TO_1(true,
		RADIO(END), /* --> */ RADIO(RSSISTOP));
#endif

	GRAPH_END();
}


int esb_prx_start_rx(void)
{
	nrf_radio_state_t radio_state;

	if (esb_running) {
		return -EBUSY;
	}

	radio_state = nrf_radio_state_get(NRF_RADIO);

	if (radio_state == NRF_RADIO_STATE_RXDISABLE || radio_state == NRF_RADIO_STATE_TXDISABLE) {
		/* If user starts RX too fast, the radio may be still disabling. */
		return -EAGAIN;
	}

	esb_configure_peripherals(false);

	nrf_radio_packetptr_set(NRF_RADIO, esb_radio_buffer);
	nrf_radio_rxaddresses_set(NRF_RADIO, esb_address.pipes_enabled);

	if (esb_config.switching_time == ESB_SWITCHING_FAST) {
		if (ESB_FAST_SWITCHING_SUPPORTED) {
			if (esb_config.ack_sending_mode == ESB_ACK_NEVER) {
				fast_sw_normal_ack_graph(true, 0);
			} else {
				fast_sw_fast_ack_graph(true, 0);
			}
		} else {
			return -ENOSYS;
		}
	} else {
		if (ESB_FAST_SWITCHING_SUPPORTED) {
			fast_sw_normal_ack_graph(true, 0);
		} else {
			normal_sw_normal_ack_graph(true, 0);
		}
	}

	esb_running = true;

	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);

	return 0;
}


int esb_prx_stop_rx(void)
{
	if (!esb_running) {
		return -EINVAL;
	}

	esb_clear_graph();
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_DISABLE);
	nrf_timer_task_trigger(ESB_TIMER, NRF_TIMER_TASK_STOP);
	esb_running = false;

	return 0;
}


int esb_prx_flush_tx(void)
{
#if CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0
	int i;
	unsigned int key = irq_lock();

	for (i = 0; i < CONFIG_ESB_PIPE_COUNT; i++) {
		esb_fifo_clear(&pipes[i].ack_payload_queue);
	}
	esb_lifo_clear(&ack_payload_unused);
	for (i = 0; i < CONFIG_ESB_TX_FIFO_SIZE; i++) {
		esb_lifo_push(&ack_payload_unused, &ack_payload_items[i].queue_item);
	}

	irq_unlock(key);

#endif /* CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0 */
	return 0;
}


bool esb_prx_tx_full(void)
{
	bool result;

#if CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0
	unsigned int key = irq_lock();

	result = esb_lifo_is_empty(&ack_payload_unused);
	irq_unlock(key);
#else
	result = true;
#endif /* CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0 */

	return result;
}


int esb_prx_flush_rx(void)
{
	unsigned int key = irq_lock();

	ESB_FIFO_CLEAR(&rx_fifo);
	irq_unlock(key);
	return 0;
}


void esb_prx_egu_irq(void)
{
	struct esb_evt event;

	while (nrf_egu_event_check(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0) && esb_running) {
		nrf_egu_event_clear(ESB_EGU, NRF_EGU_EVENT_TRIGGERED0);
		if (ESB_FIFO_COUNT(&rx_fifo, rx_fifo_array) == 0) {
			/* No more packets to process */
			break;
		}
		event.evt_id = ESB_EVENT_RX_RECEIVED;
		event.tx_attempts = 0;
		esb_config.event_handler(&event);
	}
}


int esb_prx_init(const struct esb_config *config)
{
	int err;

	/* Initialize pipes. */
	memset(pipes, 0, sizeof(pipes));

#if CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0
	uint32_t i;

	for (i = 0; i < CONFIG_ESB_PIPE_COUNT; i++) {
		pipes[i].last_pid = PID_UNKNOWN;
	}

	/* Initialize the ACK queue. */
	esb_lifo_clear(&ack_payload_unused);
	for (i = 0; i < CONFIG_ESB_TX_FIFO_SIZE; i++) {
		esb_lifo_push(&ack_payload_unused, &ack_payload_items[i].queue_item);
	}

#endif /* CONFIG_ESB_MAX_ACK_PAYLOAD_LENGTH > 0 */

	/* Initialize RX FIFO. */
	ESB_FIFO_INIT(&rx_fifo);

	err = esb_alloc_channels_and_groups(ESB_MAX_PRX_CHANNELS, ESB_MAX_PRX_GROUPS);
	if (err != 0) {
		return err;
	}

	return 0;
}


void esb_prx_disable(void)
{
	esb_prx_stop_rx();
	esb_free_channels_and_groups();
}

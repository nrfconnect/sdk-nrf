/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_events.h>
#include <zephyr/kernel.h>

/* Helper mask, with 0th bits for all processors set. */
#define ALL_CPUS_MASK 0x11111111

/* Helper mask, with all event bits for a given processor set. */
#define ALL_EVENT_MASK 0x0F

/* The maximum CPU ID handled by this module. */
#define CPU_N 8

/* The number of bits per CPU inside the event mask. */
#define BITS_N 4

/* Internal event structure, common for all API calls. */
static K_EVENT_DEFINE(suit_events);

/* Convert (cpu_id, event mask) pair into global (common for all CPUs) event mask. */
static uint32_t cpu_events_to_suit_events_mask(uint8_t cpu_id, uint8_t events)
{
	/* Check for valid CPU ID values. */
	if ((cpu_id >= CPU_N) && (cpu_id != SUIT_EVENT_CPU_ID_ANY)) {
		return 0;
	}

	/* Mask all unsupported events. */
	events &= ALL_EVENT_MASK;

	if (cpu_id == SUIT_EVENT_CPU_ID_ANY) {
		return events * ALL_CPUS_MASK;
	}

	return (uint32_t)events << (cpu_id * BITS_N);
}

/* Convert global (common for all CPUs) event mask into an event mask for a given CPU. */
static uint8_t suit_events_mask_to_cpu_events(uint8_t cpu_id, uint32_t events)
{
	uint32_t cpu_shift = 0;
	uint32_t cpu_events = 0;

	/* Check for valid CPU ID values. */
	if ((cpu_id >= CPU_N) && (cpu_id != SUIT_EVENT_CPU_ID_ANY)) {
		return 0;
	}

	if (cpu_id == SUIT_EVENT_CPU_ID_ANY) {
		uint8_t common_mask = 0;

		for (size_t id = 0; id < CPU_N; id++) {
			cpu_shift = (id * BITS_N);
			cpu_events =
				(events & ((uint32_t)ALL_EVENT_MASK << cpu_shift)) >> cpu_shift;
			common_mask |= (cpu_events & ALL_EVENT_MASK);
		}

		return common_mask;
	}

	cpu_shift = (cpu_id * BITS_N);
	cpu_events = (events & ((uint32_t)ALL_EVENT_MASK << cpu_shift)) >> cpu_shift;

	return (cpu_events & ALL_EVENT_MASK);
}

uint32_t suit_event_post(uint8_t cpu_id, uint8_t events)
{
	uint32_t main_mask = cpu_events_to_suit_events_mask(cpu_id, events);

	if (main_mask == 0) {
		/* Unsupported CPU ID. */
		return 0;
	}

	return suit_events_mask_to_cpu_events(cpu_id, k_event_post(&suit_events, main_mask));
}

uint32_t suit_event_clear(uint8_t cpu_id, uint8_t events)
{
	uint32_t main_mask = cpu_events_to_suit_events_mask(cpu_id, events);

	if (main_mask == 0) {
		/* Unsupported CPU ID. */
		return 0;
	}

	return suit_events_mask_to_cpu_events(cpu_id, k_event_clear(&suit_events, main_mask));
}

uint32_t suit_event_wait(uint8_t cpu_id, uint8_t events, bool reset, uint32_t timeout_ms)
{
	k_timeout_t timeout = K_NO_WAIT;
	uint32_t main_mask = cpu_events_to_suit_events_mask(cpu_id, events);

	if (main_mask == 0) {
		/* Unsupported CPU ID. */
		return 0;
	}

	if (reset) {
		(void)suit_event_clear(cpu_id, main_mask);
	}

	if (timeout_ms == SUIT_WAIT_FOREVER) {
		timeout = K_FOREVER;
	} else if (timeout_ms != 0) {
		timeout = K_MSEC(timeout_ms);
	}

	return suit_events_mask_to_cpu_events(
		cpu_id, k_event_wait(&suit_events, main_mask, false, timeout));
}

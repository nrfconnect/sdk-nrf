/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_egu.h>
#include <nrfx_dppi.h>
#include <esb.h>
#include "esb_common.h"
#include "esb_graph.h"


uint32_t esb_channels_mask;
uint32_t esb_groups_mask;
uint8_t esb_channels[ESB_MAX_CHANNELS];
uint8_t esb_groups[ESB_MAX_GROUPS];
void (*volatile esb_active_graph)(bool enable_graph, uint32_t flags);
volatile uint32_t esb_active_graph_flags;

static nrfx_dppi_t dppi = ESB_DPPI_INSTANCE;


int esb_alloc_channels_and_groups(int max_channels, int max_groups)
{
	int err;

	esb_free_channels_and_groups();

	if (IS_ENABLED(CONFIG_ASSERT)) {
		memset(esb_channels, 0xFF, sizeof(esb_channels));
		memset(esb_groups, 0xFF, sizeof(esb_groups));
	}

	for (uint8_t i = 0; i < max_channels; i++) {
		err = nrfx_dppi_channel_alloc(&dppi, &esb_channels[i]);
		if (err != NRFX_SUCCESS) {
			esb_free_channels_and_groups();
			return -ENOSPC;
		}
		esb_channels_mask |= BIT(esb_channels[i]);
	}
	for (uint8_t i = 0; i < max_groups; i++) {
		err = nrfx_dppi_group_alloc(&dppi, &esb_groups[i]);
		if (err != NRFX_SUCCESS) {
			esb_free_channels_and_groups();
			return -ENOSPC;
		}
		esb_groups_mask |= BIT(esb_groups[i]);
	}

	return 0;
}


void esb_free_channels_and_groups(void)
{
	for (uint8_t i = 0; i < 32; i++) {
		if (esb_channels_mask & BIT(i)) {
			nrfx_dppi_channel_free(&dppi, i);
		}
	}
	esb_channels_mask = 0;
	for (uint8_t i = 0; i < 32; i++) {
		if (esb_groups_mask & BIT(i)) {
			nrfx_dppi_group_free(&dppi, i);
		}
	}
	esb_groups_mask = 0;
}


void esb_clear_graph(void)
{
	/* Disable all interrupts that may affect the graph. */
	irq_disable(ESB_RADIO_IRQ_NUMBER);
	irq_disable(ESB_TIMER_IRQ_NUMBER);
	irq_disable(ESB_EGU_IRQ_NUMBER);
	if (esb_active_graph != NULL) {
		/* If there is an active graph, clear it. */
		esb_active_graph(false, esb_active_graph_flags);
		esb_active_graph = NULL;
		/* Graph handling function will enable EGU interrupt back, so not needed here. */
	} else {
		/* Switch back EGU interrupts, but other are not needed since there is no graph. */
		irq_enable(ESB_EGU_IRQ_NUMBER);
	}
}

void esb_disable_group(int group)
{
	nrf_dppi_group_disable(ESB_DPPIC, esb_groups[group]);
}

void esb_enable_group(int group)
{
	nrf_dppi_group_enable(ESB_DPPIC, esb_groups[group]);
}

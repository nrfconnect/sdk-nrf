/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <tfm_ns_interface.h>

#include "secure_peripheral_partition.h"

#include <hal/nrf_egu.h>

#define EGU_INT_PRIO 4
static K_SEM_DEFINE(spp_process_sem, 0, 1);

static struct k_work process_work;
static struct k_work_delayable send_work;

static void egu0_handler(const void *context)
{
	nrf_egu_event_clear(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED0);

	k_work_submit(&process_work);
}

static void process(struct k_work *work)
{
	psa_status_t status;

	printk("SPP: processing signals: ");
	status = spp_process();
	if (status == PSA_SUCCESS) {
		printk("Success\n");
	} else {
		printk("ERROR: spp_process status: %d\n", status);
	}
}

static void send(struct k_work *work)
{
	psa_status_t status;

	printk("SPP: sending message: ");
	status = spp_send();
	if (status == PSA_SUCCESS) {
		printk("Success\n");
	} else {
		printk("ERROR: spp_send status: %d\n", status);
	}

	k_work_schedule(&send_work, K_SECONDS(20));
}

int main(void)
{
	IRQ_CONNECT(EGU0_IRQn, EGU_INT_PRIO, egu0_handler, NULL, 0);
	nrf_egu_int_enable(NRF_EGU0, NRF_EGU_INT_TRIGGERED0);
	NVIC_EnableIRQ(EGU0_IRQn);

	k_work_init(&process_work, process);
	k_work_init_delayable(&send_work, send);

	k_work_schedule(&send_work, K_NO_WAIT);

	return 0;
}

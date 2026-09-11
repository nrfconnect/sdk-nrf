/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <hal/nrf_egu.h>

#include "rtfw_internal.h"

#define RTFW_EGU_NODE DT_ALIAS(rt_egu)

BUILD_ASSERT(DT_NODE_EXISTS(RTFW_EGU_NODE),
	     "RTFW requires the rt-egu devicetree alias");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(RTFW_EGU_NODE, nordic_nrf_egu),
	     "rt-egu must reference a Nordic EGU instance");

#define RTFW_EGU      ((NRF_EGU_Type *)DT_REG_ADDR(RTFW_EGU_NODE))
#define RTFW_EGU_IRQN DT_IRQN(RTFW_EGU_NODE)

static void egu_isr(const void *argument)
{
	ARG_UNUSED(argument);

	nrf_egu_event_clear(RTFW_EGU, NRF_EGU_EVENT_TRIGGERED0);
	rtfw_delivery_signal();
}

void rtfw_doorbell_init(void)
{
	nrf_egu_event_clear(RTFW_EGU, NRF_EGU_EVENT_TRIGGERED0);
	nrf_egu_int_enable(RTFW_EGU, NRF_EGU_INT_TRIGGERED0);
	IRQ_CONNECT(RTFW_EGU_IRQN, CONFIG_RTFW_EGU_IRQ_PRIORITY, egu_isr, NULL, 0);
	irq_enable(RTFW_EGU_IRQN);
}

void rtfw_doorbell_notify(void)
{
	nrf_egu_task_trigger(RTFW_EGU, NRF_EGU_TASK_TRIGGER0);
}

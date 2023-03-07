/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_PERIPHERALS_H_
#define ESB_PERIPHERALS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <nrfx.h>
#include <nrfx_timer.h>

#include <hal/nrf_egu.h>

/** The ESB event IRQ number when running on an nRF5 device. */
#define ESB_EVT_IRQ        SWI0_IRQn
/** The handler for @ref ESB_EVT_IRQ when running on an nRF5 device. */
#define ESB_EVT_IRQHandler SWI0_IRQHandler

/** ESB timer instance number. */
#define ESB_TIMER_INSTANCE_NO CONFIG_ESB_SYS_TIMER_INSTANCE

#define ESB_TIMER_IRQ          NRFX_CONCAT_3(TIMER, ESB_TIMER_INSTANCE_NO, _IRQn)
#define ESB_TIMER_IRQ_HANDLER  NRFX_CONCAT_3(nrfx_timer_,		    \
					     ESB_TIMER_INSTANCE_NO, \
					     _irq_handler)

/** ESB nRF Timer instance */
#define ESB_NRF_TIMER_INSTANCE \
	NRFX_CONCAT_2(NRF_TIMER, ESB_TIMER_INSTANCE_NO)

/** ESB nrfx timer instance. */
#define ESB_TIMER_INSTANCE NRFX_TIMER_INSTANCE(ESB_TIMER_INSTANCE_NO)

/** ESB EGU instance, events and tasks configuration. */
#define ESB_EGU       NRF_EGU0
#define ESB_EGU_EVENT NRF_EGU_EVENT_TRIGGERED6
#define ESB_EGU_TASK  NRF_EGU_TASK_TRIGGER6

/** ESB additional EGU event and task for devices with DPPI. */
#define ESB_EGU_DPPI_EVENT NRF_EGU_EVENT_TRIGGERED7
#define ESB_EGU_DPPI_TASK  NRF_EGU_TASK_TRIGGER7

#ifdef __cplusplus
}
#endif

#endif /* ESB_PERIPHERALS_H_ */

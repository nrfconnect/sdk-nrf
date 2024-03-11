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


#if defined(CONFIG_SOC_SERIES_NRF54HX)

	/** The ESB EVT IRQ is using EGU on nRF54 devices. */
	#define ESB_EVT_IRQ_NUMBER EGU020_IRQn
	#define ESB_EVT_USING_EGU 1

	/** The ESB Radio interrupt number. */
	#define ESB_RADIO_IRQ_NUMBER RADIO_0_IRQn

	/** DPPIC instance used by ESB. */
	#define ESB_DPPIC NRF_DPPIC020

	/** ESB EGU instance configuration. */
	#define ESB_EGU NRF_EGU020

	/** Use end of packet send/received over the air for nRF54 devices. */
	#define ESB_RADIO_EVENT_END NRF_RADIO_EVENT_PHYEND
	#define ESB_SHORT_DISABLE_MASK NRF_RADIO_SHORT_PHYEND_DISABLE_MASK

	#define ESB_RADIO_INT_END_MASK NRF_RADIO_INT_PHYEND_MASK

#elif defined(CONFIG_SOC_SERIES_NRF54LX)

	/** The ESB EVT IRQ is using EGU on nRF54 devices. */
	#define ESB_EVT_IRQ_NUMBER EGU10_IRQn
	#define ESB_EVT_USING_EGU 1

	/** The ESB Radio interrupt number. */
	#define ESB_RADIO_IRQ_NUMBER RADIO_0_IRQn

	/** DPPIC instance used by ESB. */
	#define ESB_DPPIC NRF_DPPIC10

	/** ESB EGU instance configuration. */
	#define ESB_EGU NRF_EGU10

	/** Use end of packet send/received over the air for nRF54 devices. */
	#define ESB_RADIO_EVENT_END NRF_RADIO_EVENT_PHYEND
	#define ESB_SHORT_DISABLE_MASK NRF_RADIO_SHORT_PHYEND_DISABLE_MASK

	#define ESB_RADIO_INT_END_MASK NRF_RADIO_INT_PHYEND_MASK
#else

	/** The ESB event IRQ number when running on the nRF52 and nRF53 device. */
	#define ESB_EVT_IRQ_NUMBER SWI0_IRQn

	/** The ESB Radio interrupt number. */
	#define ESB_RADIO_IRQ_NUMBER RADIO_IRQn

	/** DPPIC instance used by ESB. */
	#define ESB_DPPIC NRF_DPPIC

	/** ESB EGU instance configuration. */
	#define ESB_EGU NRF_EGU0

	/** nRF52 and nRF53 device has just one kind of end event. */
	#define ESB_RADIO_EVENT_END NRF_RADIO_EVENT_END
	#define ESB_SHORT_DISABLE_MASK NRF_RADIO_SHORT_END_DISABLE_MASK

	#define ESB_RADIO_INT_END_MASK NRF_RADIO_INT_END_MASK

#endif

/** ESB timer instance number. */
#if defined(CONFIG_ESB_SYS_TIMER_INSTANCE_LEADING_ZERO)
#define ESB_TIMER_INSTANCE_NO NRFX_CONCAT_2(0, CONFIG_ESB_SYS_TIMER_INSTANCE)
#else
#define ESB_TIMER_INSTANCE_NO CONFIG_ESB_SYS_TIMER_INSTANCE
#endif

#define ESB_TIMER_IRQ          NRFX_CONCAT_3(TIMER, ESB_TIMER_INSTANCE_NO, _IRQn)
#define ESB_TIMER_IRQ_HANDLER  NRFX_CONCAT_3(nrfx_timer_,		    \
					     ESB_TIMER_INSTANCE_NO, \
					     _irq_handler)

/** ESB nRF Timer instance */
#define ESB_NRF_TIMER_INSTANCE \
	NRFX_CONCAT_2(NRF_TIMER, ESB_TIMER_INSTANCE_NO)

/** ESB nrfx timer instance. */
#define ESB_NRFX_TIMER_INSTANCE NRFX_TIMER_INSTANCE(ESB_TIMER_INSTANCE_NO)

#if !defined(CONFIG_NRFX_DPPI)
/** Use fixed DPPI channels and groups if nrfx_dppi is not available. */
#define ESB_DPPI_FIXED
/** First fixed DPPI channel, total used channels: 7. */
#define ESB_DPPI_FIRST_FIXED_CHANNEL 0
/** First fixed DPPI group, total used groups: 1. */
#define ESB_DPPI_FIRST_FIXED_GROUP 0
#endif

/** ESB EGU events and tasks configuration. */
#define ESB_EGU_EVENT NRF_EGU_EVENT_TRIGGERED6
#define ESB_EGU_TASK  NRF_EGU_TASK_TRIGGER6

/** ESB additional EGU event and task for devices with DPPI. */
#define ESB_EGU_DPPI_EVENT NRF_EGU_EVENT_TRIGGERED7
#define ESB_EGU_DPPI_TASK  NRF_EGU_TASK_TRIGGER7

/** Use additional EGU channel if SWI0 is unavailable. */
#define ESB_EGU_EVT_EVENT NRF_EGU_EVENT_TRIGGERED8
#define ESB_EGU_EVT_TASK NRF_EGU_TASK_TRIGGER8
#define ESB_EGU_EVT_INT NRF_EGU_INT_TRIGGERED8

#ifdef __cplusplus
}
#endif

#endif /* ESB_PERIPHERALS_H_ */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_HW_H_
#define ESB_HW_H_

#include <zephyr/kernel.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_egu.h>
#include <hal/nrf_timer.h>
#include <nrfx_dppi.h>


#if defined(CONFIG_SOC_SERIES_NRF54HX)

#define ESB_RADIO_IRQ_NUMBER RADIO_0_IRQn
#define ESB_DPPIC NRF_DPPIC020
#define ESB_DPPI_INSTANCE NRFX_DPPI_INSTANCE(020)
#define ESB_EGU NRF_EGU020
#define ESB_EGU_IRQ_NUMBER EGU020_IRQn

#define ESB_NORMAL_RXRU_TIME_US 40
#define ESB_NORMAL_TXRU_TIME_US 40
#define ESB_FAST_RXRU_TIME_US 20
#define ESB_FAST_TXRU_TIME_US 20
#define ESB_DISABLE_TIME_US 1

#elif defined(CONFIG_SOC_SERIES_NRF54LX)

#define ESB_RADIO_IRQ_NUMBER RADIO_0_IRQn
#define ESB_DPPIC NRF_DPPIC10
#define ESB_DPPI_INSTANCE NRFX_DPPI_INSTANCE(10)
#define ESB_EGU NRF_EGU10
#define ESB_EGU_IRQ_NUMBER EGU10_IRQn

#define ESB_NORMAL_RXRU_TIME_US 40
#define ESB_NORMAL_TXRU_TIME_US 40
#define ESB_FAST_RXRU_TIME_US 20
#define ESB_FAST_TXRU_TIME_US 20
#define ESB_DISABLE_TIME_US 1

#elif defined(CONFIG_SOC_SERIES_NRF53X)

#define ESB_RADIO_IRQ_NUMBER RADIO_IRQn
#define ESB_DPPIC NRF_DPPIC0
#define ESB_DPPI_INSTANCE NRFX_DPPI_INSTANCE(0)
#define ESB_EGU NRF_EGU0
#define ESB_EGU_IRQ_NUMBER EGU0_IRQn

#define ESB_NORMAL_RXRU_TIME_US 40
#define ESB_NORMAL_TXRU_TIME_US 40
#define ESB_FAST_RXRU_TIME_US 40
#define ESB_FAST_TXRU_TIME_US 40
#define ESB_DISABLE_TIME_US 1


#elif defined(CONFIG_SOC_SERIES_NRF52X)

#error "Not implemented for PPI (nRF52 series devices)."

#else

#error "Unsupported SoC series"

#endif


#if defined(CONFIG_ESB_SYS_TIMER_INSTANCE_LEADING_ZERO)
#define _ESB_TIMER_INSTANCE_NO NRFX_CONCAT_2(0, CONFIG_ESB_SYS_TIMER_INSTANCE)
#else
#define _ESB_TIMER_INSTANCE_NO CONFIG_ESB_SYS_TIMER_INSTANCE
#endif
#define ESB_TIMER_IRQ_NUMBER NRFX_CONCAT_3(TIMER, _ESB_TIMER_INSTANCE_NO, _IRQn)
#define ESB_TIMER NRFX_CONCAT_2(NRF_TIMER, _ESB_TIMER_INSTANCE_NO)


#endif /* ESB_HW_H_ */

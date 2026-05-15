/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_init_soc.h
 *
 * @brief SoC-specific parameters for MPSL init.
 *
 */

#ifndef MPSL_INIT_SOC_H__
#define MPSL_INIT_SOC_H__

#if defined(CONFIG_SOC_COMPATIBLE_NRF52X) || defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define MPSL_INIT_SOC_COUNTER_RESERVED_NODES rtc0, timer0
#define MPSL_TIMER_IRQn                      TIMER0_IRQn
#define MPSL_RTC_IRQn                        RTC0_IRQn
#define MPSL_RADIO_IRQn                      RADIO_IRQn

#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX) || defined(CONFIG_SOC_SERIES_NRF71)
#define MPSL_INIT_SOC_COUNTER_RESERVED_NODES timer10
#define MPSL_TIMER_IRQn                      TIMER10_IRQn
#define MPSL_RTC_IRQn                        GRTC_3_IRQn
#define MPSL_RADIO_IRQn                      RADIO_0_IRQn

#define MPSL_RESERVED_GRTC_CHANNELS ((1U << 7) | (1U << 8) | (1U << 9) | (1U << 10) | (1U << 11))

#elif defined(CONFIG_SOC_SERIES_NRF54H)
#define MPSL_INIT_SOC_COUNTER_RESERVED_NODES timer020
#define MPSL_TIMER_IRQn                      TIMER020_IRQn
#define MPSL_RTC_IRQn                        GRTC_2_IRQn
#define MPSL_RADIO_IRQn                      RADIO_0_IRQn
#define MPSL_RESERVED_IPCT_SOURCE_CHANNELS   (1U << 0)
#define MPSL_RESERVED_DPPI_SOURCE_CHANNELS   (1U << 0)
#define MPSL_RESERVED_DPPI_SINK_CHANNELS     (1U << 0)

#define MPSL_RESERVED_GRTC_CHANNELS ((1U << 8) | (1U << 9) | (1U << 10) | (1U << 11) | (1U << 12))
#endif /* CONFIG_SOC_COMPATIBLE_NRF52X || CONFIG_SOC_NRF5340_CPUNET */

#if defined(CONFIG_SOC_SERIES_NRF54L)
#define MPSL_INIT_SOC_CPU_FREQ_MHZ 128
#endif

#endif /* MPSL_INIT_SOC_H__ */

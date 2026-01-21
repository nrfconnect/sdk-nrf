/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf_security_core.h
 *
 * This interface is used for core functionality e.g. delays.
 */

#ifndef NRF_SECURITY_CORE_H
#define NRF_SECURITY_CORE_H

#include <stdint.h>

/**
 * @brief Delay execution for the number of microseconds.
 *
 * @param[in] time_us Number of microseconds to wait.
 */
void nrf_security_core_delay_us(uint32_t time_us);

#endif /* NRF_SECURITY_CORE_H */

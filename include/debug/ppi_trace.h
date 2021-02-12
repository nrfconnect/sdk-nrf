/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef __PPI_TRACE_H
#define __PPI_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ppi_trace PPI trace
 * @brief Module for tracing hardware events on GPIO.
 *
 * @{
 */

/** @brief Configure a PPI trace pin for tracing a single event.
 *
 * @note If a hardware event is used by DPPI in the application, the PPI trace
 *	 configuration must occur after the DPPI connection for the given event.
 *	 The order is important because DPPI allows assigning events to only
 *	 one channel. Therefore, PPI trace subscribes to the same
 *	 channel.
 *
 * @param pin		Pin to use for tracing.
 * @param evt		Hardware event to be traced on the pin.
 *
 * @return Handle, or NULL if the configuration failed.
 */
void *ppi_trace_config(uint32_t pin, uint32_t evt);

/** @brief Configure a PPI trace pin for tracing complementary events.
 *
 * @note Not supported on nRF51 Series. Requires presence of GPIOTE SET and CLR
 *	 tasks.
 *
 * @param pin		Pin to use for tracing.
 * @param start_evt	Hardware event that sets the pin.
 * @param stop_evt	Hardware event that clears the pin.
 *
 * @return Handle, or NULL if the configuration failed.
 */
void *ppi_trace_pair_config(uint32_t pin, uint32_t start_evt, uint32_t stop_evt);

/** @brief Enable PPI trace pin.
 *
 * @param handle	Handle.
 */
void ppi_trace_enable(void *handle);

/** @brief Disable PPI trace pin.
 *
 * @param handle	Handle.
 */
void ppi_trace_disable(void *handle);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __PPI_TRACE_H */

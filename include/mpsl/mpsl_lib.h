/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_lib.h
 *
 * @defgroup mpsl_lib Multiprotocol Service Layer library control.
 *
 * @brief Methods for initializing MPSL and required interrupt handlers.
 * @{
 */

#ifndef MPSL_LIB__
#define MPSL_LIB__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @brief Initialize MPSL and attach interrupt handlers.
 *
 * This routine initializes MPSL (via `mpsl_init`) after
 * it has been uninitialized via @ref mpsl_lib_uninit, and attaches
 * all required interrupt handlers.
 *
 * @pre This method requires CONFIG_MPSL_DYNAMIC_INTERRUPTS to be enabled and
 *      MPSL to have been previously uninitialized via @ref mpsl_lib_uninit.
 *
 * @note
 * After successful execution of this method, any existing interrupt handlers
 * will be detached from RADIO_IRQn, RTC0_IRQn, and TIMER0_IRQn.
 *
 * @retval   0             MPSL enabled successfully.
 * @retval   -NRF_EPERM    Operation is not supported or
 *                         MPSL is already initialized.
 * @retval   -NRF_EINVAL   Invalid parameters supplied to MPSL.
 */
int32_t mpsl_lib_init(void);

/** @brief Uninitialize MPSL and disable interrupt handlers.
 *
 * This routine uninitializes MPSL (via `mpsl_uninit`) and
 * disables MPSL interrupts. Uninitializing MPSL stops clocks and scheduler.
 * This will release all peripherals and reduce power usage, allowing the
 * user to override any interrupt handlers used by MPSL.
 *
 * @pre This method requires CONFIG_MPSL_DYNAMIC_INTERRUPTS to be enabled.
 *
 * @note
 * After successful execution of this method, user-supplied interrupt
 * handlers can be attached to RADIO_IRQn, RTC0_IRQn, and TIMER0_IRQn
 * using `irq_connect_dynamic`.
 * Care must be taken when developing these handlers, as they will be
 * executed as direct dynamic interrupts.
 * See `ARM_IRQ_DIRECT_DYNAMIC_CONNECT` for additional documentation.
 * These interrupts will trigger thread re-scheduling upon return.
 *
 * @retval   0             MPSL disabled successfully.
 * @retval   -NRF_EPERM    Operation is not supported.
 */
int32_t mpsl_lib_uninit(void);

#ifdef __cplusplus
}
#endif

#endif /* MPSL_LIB__ */

/**@} */

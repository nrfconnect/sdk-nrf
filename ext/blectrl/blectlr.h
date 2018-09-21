/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BLECTLR_H__
#define BLECTLR_H__


/** @brief Host signal handler function pointer type. */
typedef void (*host_signal_t)(void);

/**
 * @brief     Function prototype for the blectlr assertion handler. To be implemented
 *            by the application.
 *
 * @note      The implementation of this function should not return.
 *            This function may be called from different interrupt priorities
 *            and should be preemptive or disable interrupts.
 *
 * @param[in] file                  The filename where the assertion occurred
 * @param[in] line                  The line number where the assertion occurred
 */
void blectlr_assertion_handler(const char * const file, const uint32_t line);

/**
 * @brief      Initialize the BLE Controller
 *
 * @param[in]  host_signal  The host signal handler, this will be called by the BLE controller
 *                          when data or event is available.
 *
 * @retval ::NRF_SUCCESS             Initialized successfully
 * @retval ::NRF_ERROR_NO_MEM        Not enough memory to support the current configuration.
 */
uint32_t blectlr_init(host_signal_t host_signal);

/**
 * @brief      Process all pending signals in the BLE Controller
 *
 * @note       The higher priority interrupts will notify that the signals
 *             are ready to be processed by setting the SWI5 IRQ pending.
 */
void blectlr_signal(void);

/**
 * @brief      BLE Controller RADIO interrupt handler
 *
 * @note       This handler should be placed in the interrupt vector table.
 *             The interrupt priority level should be priority 0.
 */
void C_RADIO_Handler(void);

/**
 * @brief      BLE Controller RTC0 interrupt handler
 *
 * @note       This handler should be placed in the interrupt vector table.
 *             The interrupt priority level should be priority 0
 */
void C_RTC0_Handler(void);

/**
 * @brief      BLE Controller TIMER0 interrupt handler.
 *
 * @note       This handler should be placed in the interrupt vector table.
 *             The interrupt priority level should be priority 0
 */
void C_TIMER0_Handler(void);

/**
 * @brief      BLE Controller RNG interrupt handler
 *
 * @note       This handler should be placed in the interrupt vector table.
 *             The interrupt priority level should be lower than priority 0.
 */
void C_RNG_Handler(void);

/**
 * @brief      BLE Controller POWER_CLOCK interrupt handler
 *
 * @note       This handler should be placed in the interrupt vector table.
 *             The interrupt priority level should be lower than priority 0.
 */
void C_POWER_CLOCK_Handler(void);

#endif

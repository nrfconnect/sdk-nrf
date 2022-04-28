/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

/* Undef the TF-M defined array size since devicetree will pull in util.h which
 * has this definition.
 */
#undef ARRAY_SIZE

#include <autoconf.h>
#include <devicetree.h>

#define UARTE(idx)			DT_NODELABEL(uart##idx)
#define UARTE_PROP(idx, prop)		DT_PROP(UARTE(idx), prop)

// <i> Configuration settings for Driver_USART0 in component ::Drivers:USART
#define   RTE_USART0                    1
//   <h> Pin Selection (0xFFFFFFFF means Disconnected)
//     <o> TXD
#define   RTE_USART0_TXD_PIN            UARTE_PROP(0, tx_pin)
//     <o> RXD
#define   RTE_USART0_RXD_PIN            UARTE_PROP(0, rx_pin)
//     <o> RTS
#define   RTE_USART0_RTS_PIN            UARTE_PROP(0, rts_pin)
//     <o> CTS
#define   RTE_USART0_CTS_PIN            UARTE_PROP(0, cts_pin)
//   </h> Pin Configuration

// <i> Configuration settings for Driver_USART1 in component ::Drivers:USART
#define   RTE_USART1                    1
//   <h> Pin Selection (0xFFFFFFFF means Disconnected)
#if defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP_NS)
//     <o> TXD
#define   RTE_USART1_TXD_PIN            25 // TODO: Add to devicetree
//     <o> RXD
#define   RTE_USART1_RXD_PIN            26 // TODO: Add to devicetree
#else
//     <o> TXD
#define   RTE_USART1_TXD_PIN            UARTE_PROP(1, tx_pin)
//     <o> RXD
#define   RTE_USART1_RXD_PIN            UARTE_PROP(1, rx_pin)
#endif
//     <o> RTS
#define   RTE_USART1_RTS_PIN            0xFFFFFFFF
//     <o> CTS
#define   RTE_USART1_CTS_PIN            0xFFFFFFFF
//   </h> Pin Configuration

// <e> FLASH (Flash Memory) [Driver_FLASH0]
// <i> Configuration settings for Driver_FLASH0 in component ::Drivers:FLASH
#define   RTE_FLASH0                    1
// </e> FLASH (Flash Memory) [Driver_FLASH0]

#endif  /* __RTE_DEVICE_H */

/*
 * Copyright (c) 2019 Arm Limited. All rights reserved.
 * Copyright (c) 2021 Nordic Semiconductor ASA. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

#include <devicetree.h>
#include <autoconf.h>

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
#if defined(CONFIG_BOARD_NRF5340PDK_NRF5340_CPUAPPNS) \
	|| defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPPNS)
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

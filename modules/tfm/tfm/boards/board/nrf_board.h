/*
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

#ifndef NRF_BOARD_H__
#define NRF_BOARD_H__

#include <hal/nrf_gpio.h>
#include <devicetree.h>

#define BUTTON1_PIN          (DT_GPIO_PIN(DT_NODELABEL(button0), gpios))
#define BUTTON1_ACTIVE_LEVEL (0UL)
#define BUTTON1_PULL         (NRF_GPIO_PIN_PULLUP)
#define LED1_PIN             (DT_GPIO_PIN(DT_NODELABEL(led0), gpios))
#define LED1_ACTIVE_LEVEL    (0UL)

#endif // NRF_BOARD_H__

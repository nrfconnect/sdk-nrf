/*
 * Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
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

#pragma once

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#define RGB_NODE DT_ALIAS(rgb_pwm)
#define LED_COLOR_LIGHT_AVAILABLE DT_NODE_HAS_STATUS(RGB_NODE, okay)

#define BUZZER_NODE DT_ALIAS(buzzer_pwm)
#define BUZZER_AVAILABLE DT_NODE_HAS_STATUS(BUZZER_NODE, okay)

#define PUSH_BUTTON_NODE(idx) DT_ALIAS(push_button_##idx)
#define PUSH_BUTTON_AVAILABLE(idx) DT_NODE_HAS_STATUS(PUSH_BUTTON_NODE(idx), okay)
#define PUSH_BUTTON_AVAILABLE_ANY                                                                  \
	(PUSH_BUTTON_AVAILABLE(0) || PUSH_BUTTON_AVAILABLE(1) || PUSH_BUTTON_AVAILABLE(2) ||       \
	 PUSH_BUTTON_AVAILABLE(3))

#define SWITCH_NODE(idx) DT_ALIAS(switch_##idx)
#define SWITCH_AVAILABLE(idx) DT_NODE_HAS_STATUS(SWITCH_NODE(idx), okay)
#define SWITCH_AVAILABLE_ANY (SWITCH_AVAILABLE(0) || SWITCH_AVAILABLE(1) || SWITCH_AVAILABLE(2))

#define PUSH_BUTTON_GLUE_ITEM(num)                                                                 \
	{                                                                                          \
		.device = DEVICE_DT_GET(DT_GPIO_CTLR(PUSH_BUTTON_NODE(num), gpios)),               \
		.gpio_pin = DT_GPIO_PIN(PUSH_BUTTON_NODE(num), gpios),                             \
		.gpio_flags = (GPIO_INPUT | DT_GPIO_FLAGS(PUSH_BUTTON_NODE(num), gpios))           \
	}

#define SWITCH_BUTTON_GLUE_ITEM(num)                                                               \
	{                                                                                          \
		.device = DEVICE_DT_GET(DT_GPIO_CTLR(SWITCH_NODE(num), gpios)),                    \
		.gpio_pin = DT_GPIO_PIN(SWITCH_NODE(num), gpios),                                  \
		.gpio_flags = (GPIO_INPUT | DT_GPIO_FLAGS(SWITCH_NODE(num), gpios))                \
	}

#define TEMPERATURE_NODE DT_ALIAS(temperature)
#define TEMPERATURE_AVAILABLE DT_NODE_HAS_STATUS(TEMPERATURE_NODE, okay)

#define HUMIDITY_NODE DT_ALIAS(humidity)
#define HUMIDITY_AVAILABLE DT_NODE_HAS_STATUS(HUMIDITY_NODE, okay)

#define BAROMETER_NODE DT_ALIAS(barometer)
#define BAROMETER_AVAILABLE DT_NODE_HAS_STATUS(BAROMETER_NODE, okay)

#define DISTANCE_NODE DT_ALIAS(distance)
#define DISTANCE_AVAILABLE DT_NODE_HAS_STATUS(DISTANCE_NODE, okay)

#define ILLUMINANCE_NODE DT_ALIAS(illuminance)
#define ILLUMINANCE_AVAILABLE DT_NODE_HAS_STATUS(ILLUMINANCE_NODE, okay)

#define ACCELEROMETER_NODE DT_ALIAS(accelerometer)
#define ACCELEROMETER_AVAILABLE DT_NODE_HAS_STATUS(ACCELEROMETER_NODE, okay)

#define GYROMETER_NODE DT_ALIAS(gyrometer)
#define GYROMETER_AVAILABLE DT_NODE_HAS_STATUS(GYROMETER_NODE, okay)

#define MAGNETOMETER_NODE DT_ALIAS(magnetometer)
#define MAGNETOMETER_AVAILABLE DT_NODE_HAS_STATUS(MAGNETOMETER_NODE, okay)

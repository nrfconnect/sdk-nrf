/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <zephyr/devicetree.h>
#include <dk_buttons_and_leds.h>

#define LEDS_NODE_ID DT_PATH(leds)
#define BUTTONS_NODE_ID DT_PATH(buttons)
#define INCREMENT_BY_ONE(button_or_led) +1
#define NUMBER_OF_LEDS (0 DT_FOREACH_CHILD(LEDS_NODE_ID, INCREMENT_BY_ONE))
#define NUMBER_OF_BUTTONS (0 DT_FOREACH_CHILD(BUTTONS_NODE_ID, INCREMENT_BY_ONE))

#define FUNCTION_BUTTON DK_BTN1
#define FUNCTION_BUTTON_MASK DK_BTN1_MSK

#ifndef BLUETOOTH_ADV_BUTTON
#define BLUETOOTH_ADV_BUTTON DK_BTN1
#endif
#define BLUETOOTH_ADV_BUTTON_MASK BIT(BLUETOOTH_ADV_BUTTON)

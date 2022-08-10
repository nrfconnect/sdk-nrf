/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

/* ---- Lighting Example App Config ---- */

#include "board_util.h"

#define FUNCTION_BUTTON DK_BTN1
#define FUNCTION_BUTTON_MASK DK_BTN1_MSK
#if NUMBER_OF_BUTTONS == 2
#define BLE_ADVERTISEMENT_START_AND_SWITCH_BUTTON DK_BTN2
#define BLE_ADVERTISEMENT_START_AND_SWITCH_BUTTON_MASK DK_BTN2_MSK
#else
#define SWITCH_BUTTON DK_BTN2
#define SWITCH_BUTTON_MASK DK_BTN2_MSK
#define BLE_ADVERTISEMENT_START_BUTTON DK_BTN4
#define BLE_ADVERTISEMENT_START_BUTTON_MASK DK_BTN4_MSK
#endif

#define SYSTEM_STATE_LED DK_LED1
#define IDENTIFY_LED DK_LED2

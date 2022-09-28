/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

/* ---- Template App Config ---- */

#include "board_util.h"

#define FUNCTION_BUTTON DK_BTN1
#define FUNCTION_BUTTON_MASK DK_BTN1_MSK

#define SYSTEM_STATE_LED DK_LED1
#define FACTORY_RESET_SIGNAL_LED DK_LED2

#if NUMBER_OF_LEDS == 4
#define FACTORY_RESET_SIGNAL_LED1 DK_LED3
#define FACTORY_RESET_SIGNAL_LED2 DK_LED4
#endif

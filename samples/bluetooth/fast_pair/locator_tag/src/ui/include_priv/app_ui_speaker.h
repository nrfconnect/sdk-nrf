/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_UI_SPEAKER_H_
#define APP_UI_SPEAKER_H_

#include <stdint.h>


/** @brief Initialize the speaker peripheral.
 *
 *  This function initializes speaker power control pin and speaker pwm output.
 *  The pwm is initialized in continuous output mode at 50% duty cycle.
 *
 *  @return Zero on success or negative error code otherwise
 */
int app_ui_speaker_init(void);

/** @brief Turn on speaker.
 *
 *  This function turns on speaker by turning on the speaker power control pin.
 *
 *  @return Zero on success or negative error code otherwise
 */
int app_ui_speaker_on(void);

/** @brief Turn off speaker.
 *
 *  This function turns off speaker by turning off the speaker power control pin.
 *
 *  @return Zero on success or negative error code otherwise
 */
int app_ui_speaker_off(void);


#endif

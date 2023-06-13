/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UI_BUZZER_H__
#define UI_BUZZER_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Turn the buzzer on or off. If false the buzzer
 *        will be off regardless of the frequency and the intensity.
 *
 * @param new_state The buzzer's new state.
 * @return int 0 if successful, negative error code if not.
 */
int ui_buzzer_on_off(bool new_state);

/**
 * @brief Set the frequency of the buzzer.
 *
 * @param freq Frequency in Hz.
 * @return int 0 if successful, negative error code if not.
 */
int ui_buzzer_set_frequency(uint32_t freq);

/**
 * @brief Set the intesity of the buzzer.
 *
 * @param intensity Integer between [0, 100], describing
 *        a percentage of the maximum buzzer volume intensity.
 * @return int 0 if successful, negative error code if not.
 */
int ui_buzzer_set_intensity(uint8_t intensity);

/**
 * @brief Initialize the buzzer.
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_buzzer_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_BUZZER_H__ */

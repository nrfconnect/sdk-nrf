/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: PHY Test Tool library interface for external usage */

#ifndef PTT_H__
#define PTT_H__

#include "ptt_types.h"

/** @brief PTT library initialization
 *
 *  @param call_me_cb - callback to be called from the library to schedule a timer
 *  @param max_time - maximum supported time by a current timer implementation
 *
 *  @return none
 */
void ptt_init(ptt_call_me_cb_t call_me_cb, ptt_time_t max_time);

/** @brief PTT library uninitialization
 *
 *  @param none
 *
 *  @return none
 */
void ptt_uninit(void);

/** @brief Notify the library about expired timeout
 *
 *  @param current_time - current time in milliseconds
 *
 *  @return none
 */
void ptt_process(ptt_time_t current_time);

/* external provided function to get current time */
extern ptt_time_t ptt_get_current_time(void);
/* external provided function to get mode mask from application */
extern bool ptt_get_mode_mask_ext(uint32_t *mode);
/* external provided function to get channel_mask from application */
extern bool ptt_get_channel_mask_ext(uint32_t *channel_mask);
/* external provided function to get power from application */
extern bool ptt_get_power_ext(int8_t *power);
/* external provided function to get antenna from application */
extern bool ptt_get_antenna_ext(uint8_t *antenna);
/* external provided function to get SW version from application */
extern bool ptt_get_sw_version_ext(uint8_t *sw_version);
/* external provided function to get HW version from application */
extern bool ptt_get_hw_version_ext(uint8_t *hw_version);
/* external provided function to get ANT mode from application */
extern bool ptt_get_ant_mode_ext(uint8_t *ant_mode);
/* external provided function to get ANT number from application */
extern bool ptt_get_ant_num_ext(uint8_t *ant_num);
/* external provided function to reset board */
extern void ptt_do_reset_ext(void);
/** @brief external provided function to get random value
 *
 *  must be able to get at least @ref PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE bytes every
 *  (@ref PTT_L_STREAM_INTERVAL_MIN + @ref PTT_L_STREAM_PULSE_MIN) ms
 */
extern uint32_t ptt_random_get_ext(void);
/* external provided function to control HFLCK output */
extern bool ptt_clk_out_ext(uint8_t pin, bool mode);
/* external provided function to set GPIO pin value */
extern bool ptt_set_gpio_ext(uint8_t pin, uint8_t value);
/* external provided function to get GPIO pin value */
extern bool ptt_get_gpio_ext(uint8_t pin, uint8_t *value);
/* external provided function to disable/enable DC/DC mode */
extern void ptt_set_dcdc_ext(bool enable);
/* external provided function to get current DC/DC mode */
extern bool ptt_get_dcdc_ext(void);
/* external provided function to disable/enable ICACHE */
extern void ptt_set_icache_ext(bool enable);
/* external provided function to get current state of ICACHE */
extern bool ptt_get_icache_ext(void);
/* external provided function to get SoC temperature */
extern bool ptt_get_temp_ext(int32_t *temp);
/* external provided function to set LED indicating received packet state to ON */
extern void ptt_ctrl_led_indication_on_ext(void);
/* external provided function to set LED indicating received packet state to OFF*/
extern void ptt_ctrl_led_indication_off_ext(void);

#endif /* PTT_H__ */

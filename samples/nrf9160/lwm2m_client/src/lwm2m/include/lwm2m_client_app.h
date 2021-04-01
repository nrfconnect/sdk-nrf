/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CLIENT_APP_H__
#define LWM2M_CLIENT_APP_H__

#include <zephyr.h>
#include <net/lwm2m.h>

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Device */
int lwm2m_app_init_device(char *serial_num);

#if defined(CONFIG_LWM2M_IPSO_LIGHT_CONTROL)
int lwm2m_init_light_control(void);
#endif

#if defined(CONFIG_LWM2M_IPSO_TEMP_SENSOR)
int lwm2m_init_temp(void);
#endif

#if defined(CONFIG_UI_BUZZER)
int lwm2m_init_buzzer(void);
#endif

#if defined(CONFIG_UI_BUTTON)
int handle_button_events(struct ui_evt *evt);
int lwm2m_init_button(void);
#endif

#if defined(CONFIG_LWM2M_IPSO_ACCELEROMETER)
#if CONFIG_FLIP_INPUT > 0
int handle_accel_events(struct ui_evt *evt);
#endif
int lwm2m_init_accel(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_CLIENT_APP_H__ */

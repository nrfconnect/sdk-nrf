/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CLIENT_APP_H__
#define LWM2M_CLIENT_APP_H__

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>

#ifdef __cplusplus
extern "C" {
#endif

int lwm2m_app_init_device(char *serial_num);

#if defined(CONFIG_LWM2M_APP_LIGHT_CONTROL)
int lwm2m_init_light_control(void);
#endif

#if defined(CONFIG_LWM2M_APP_TEMP_SENSOR)
int lwm2m_init_temp_sensor(void);
#endif

#if defined(CONFIG_LWM2M_APP_PRESS_SENSOR)
int lwm2m_init_press_sensor(void);
#endif

#if defined(CONFIG_LWM2M_APP_HUMID_SENSOR)
int lwm2m_init_humid_sensor(void);
#endif

#if defined(CONFIG_LWM2M_APP_GAS_RES_SENSOR)
int lwm2m_init_gas_res_sensor(void);
#endif

#if defined(CONFIG_LWM2M_APP_LIGHT_SENSOR)
int lwm2m_init_light_sensor(void);
#endif

#if defined(CONFIG_LWM2M_APP_BUZZER)
int lwm2m_init_buzzer(void);
#endif

#if defined(CONFIG_LWM2M_APP_PUSH_BUTTON)
int lwm2m_init_push_button(void);
#endif

#if defined(CONFIG_LWM2M_APP_ONOFF_SWITCH)
int lwm2m_init_onoff_switch(void);
#endif

#if defined(CONFIG_LWM2M_APP_ACCELEROMETER)
int lwm2m_init_accel(void);
#endif

#if defined(CONFIG_LWM2M_PORTFOLIO_OBJ_SUPPORT)
int lwm2m_init_portfolio_object(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_CLIENT_APP_H__ */

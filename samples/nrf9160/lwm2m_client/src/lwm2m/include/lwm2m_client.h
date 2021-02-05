/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CLIENT_H__
#define LWM2M_CLIENT_H__

#include <zephyr.h>
#include <net/lwm2m.h>

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Security */
int lwm2m_init_security(struct lwm2m_ctx *ctx, char *endpoint);

/* Device */
int lwm2m_init_device(char *serial_num);

/* Location */
int lwm2m_init_location(void);

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)
void *firmware_read_cb(uint16_t obj_inst_id, size_t *data_len);
int lwm2m_init_firmware(void);
int lwm2m_init_image(void);
int lwm2m_verify_modem_fw_update(void);
#endif

#if defined(CONFIG_LWM2M_CONN_MON_OBJ_SUPPORT)
int lwm2m_init_connmon(void);
int lwm2m_start_connmon(void);
#endif

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

#endif /* LWM2M_CLIENT_H__ */

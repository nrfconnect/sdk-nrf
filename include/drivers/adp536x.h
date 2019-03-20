/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

int adp536x_init(const char *dev_name);
int adp536x_vbus_current_set(u8_t value);
int adp536x_charger_current_set(u8_t value);
int adp536x_charger_termination_voltage_set(u8_t value);
int adp536x_charging_enable(bool enable);
int adp536x_charger_status_1_read(u8_t *buf);
int adp536x_charger_status_2_read(u8_t *buf);
int adp536x_oc_chg_hiccup_set(bool enable);
int adp536x_oc_dis_hiccup_set(bool enable);
int adp536x_buckbst_enable(bool enable);
int adp536x_buck_1v8_set(void);
int adp536x_buckbst_3v3_set(void);
int adp536x_factory_reset(void);
int adp536x_oc_chg_current_set(u8_t value);

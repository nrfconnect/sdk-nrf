/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief ESL client settings module
 */

#ifndef ESL_CLIENT_SETTINGS_H_
#define ESL_CLIENT_SETTINGS_H_

#include <zephyr/settings/settings.h>

int esl_c_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg);
int esl_c_settings_init(void);
int esl_c_setting_ap_key_save(void);
int esl_c_setting_clear_all(void);

#if defined(CONFIG_BT_ESL_TAG_STORAGE)
int esl_c_setting_tags_per_group_save(void);
int esl_c_setting_group_per_button_save(void);
#endif /* (CONFIG_BT_ESL_TAG_STORAGE) */

#endif /*ESL_CLIENT_SETTINGS_H__*/

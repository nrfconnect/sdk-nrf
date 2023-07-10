/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief ESL settings module
 */

#ifndef ESL_SETTINGS_H_
#define ESL_SETTINGS_H_

int esl_settings_init(void);
int esl_settings_addr_save(void);
int esl_settings_ap_key_save(void);
int esl_settings_rsp_key_save(void);
int esl_settings_abs_save(void);
int esl_settings_delete_provisioned_data(void);

#endif /*ESL_SETTINGS_H__*/

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_CLOUD_LWM2M_H
#define MOSH_CLOUD_LWM2M_H

struct lwm2m_ctx *cloud_lwm2m_client_ctx_get(void);

bool cloud_lwm2m_is_connected(void);

int cloud_lwm2m_connect(void);

int cloud_lwm2m_disconnect(void);

int cloud_lwm2m_suspend(void);

int cloud_lwm2m_resume(void);

void cloud_lwm2m_update(void);

#endif /* MOSH_CLOUD_LWM2M_H */

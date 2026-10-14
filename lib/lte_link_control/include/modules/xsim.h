/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef XSIM_H__
#define XSIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Enable notifications.
 *
 * Not all modem firmware versions support %XSIM, so a failure is only logged and must not
 * prevent the functional mode change.
 */
void xsim_notifications_enable(void);

#ifdef __cplusplus
}
#endif

#endif /* XSIM_H__ */

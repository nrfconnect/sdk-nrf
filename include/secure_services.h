/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SECURE_SERVICES_H__
#define SECURE_SERVICES_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Request a system reboot from the Secure Firmware.
 */
void spm_request_system_reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* SECURE_SERVICES_H__ */

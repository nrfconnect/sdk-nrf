/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TLS_CREDENTIALS_H__
#define TLS_CREDENTIALS_H__

#ifdef __cplusplus
extern "C" {
#endif


#define PSA_PS_CERTIFICATE_UID		(1)
#define PSA_PS_CERTIFICATE_KEY_UID	(2)
#define PSA_PS_CA_CERTIFICATE_UID	(3)

#define APP_SUCCESS			(0)
#define APP_ERROR			(-1)


/** @brief Function for registering certificates and PSKs.
 *
 * @retval 0 TLS credential successfully set.
 */
int tls_set_credentials(void);


#ifdef __cplusplus
}
#endif

#endif /* TLS_CREDENTIALS_H__ */

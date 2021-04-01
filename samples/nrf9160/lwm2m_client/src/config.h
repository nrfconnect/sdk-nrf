/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
static char client_psk[] = CONFIG_APP_LWM2M_PSK;

#define SERVER_TLS_TAG CONFIG_APP_SERVER_TLS_TAG
#define BOOTSTRAP_TLS_TAG CONFIG_APP_BOOTSTRAP_TLS_TAG
#endif /* defined(CONFIG_LWM2M_DTLS_SUPPORT) */

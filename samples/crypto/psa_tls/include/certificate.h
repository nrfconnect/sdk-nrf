/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#define CA_CERTIFICATE_TAG 1
#define SERVER_CERTIFICATE_TAG 2
#define PSK_TAG 3


#if defined(CONFIG_PSA_TLS_CERTIFICATE_TYPE_RSA)

static const unsigned char ca_certificate[] = {
#include "root-cert.der.inc"
};
static const unsigned char server_certificate[] = {
#include "echo-apps-cert.der.inc"
};
/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "echo-apps-key.der.inc"
};

#elif defined(CONFIG_PSA_TLS_CERTIFICATE_TYPE_ECDSA)

static const unsigned char ca_certificate[] = {
#include "ec-root-cert.der.inc"
};
static const unsigned char server_certificate[] = {
#include "ec-echo-apps-cert.der.inc"
};
/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "ec-echo-apps-key.der.inc"
};

#else

#error "No certificate type selected"

#endif


#endif /* __CERTIFICATE_H__ */

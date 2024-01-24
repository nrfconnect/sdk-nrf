/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 *      "-----BEGIN CA CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CA CERTIFICATE-----\n"
 */
#if defined(CONFIG_NRF_CLOUD_PROVISION_CA_CERT)
static const unsigned char ca_certificate[] = {
#include "ca-cert.pem"
};
#endif

/*
 *      "-----BEGIN PRIVATE KEY-----\n"
 *      "-----KEY-----\n"
 *      "-----END PRIVATE KEY-----\n"
 */
#if defined(CONFIG_NRF_CLOUD_PROVISION_PRV_KEY)
static const unsigned char private_key[] = {
#include "private-key.pem"
};
#endif

/*
 *      "-----BEGIN CLIENT CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CLIENT CERTIFICATE-----\n"
 */
#if defined(CONFIG_NRF_CLOUD_PROVISION_CLIENT_CERT)
static const unsigned char device_certificate[] = {
#include "client-cert.pem"
};
#endif

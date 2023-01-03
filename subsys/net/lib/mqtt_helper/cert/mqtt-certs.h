/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#warning "Credentials are exposed in non-secure memory. Do avoid in a production scenario."

/*
 *      "-----BEGIN CA CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CA CERTIFICATE-----\n"
 */
static const unsigned char ca_certificate[] = {
#include "ca-cert.pem"
};

/*
 *      "-----BEGIN PRIVATE KEY-----\n"
 *      "-----KEY-----\n"
 *      "-----END PRIVATE KEY-----\n"
 */
static const unsigned char private_key[] = {
#include "private-key.pem"
};

/*
 *      "-----BEGIN CLIENT CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CLIENT CERTIFICATE-----\n"
 */
static const unsigned char device_certificate[] = {
#include "client-cert.pem"
};

#if CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1

/*
 *      "-----BEGIN CA CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CA CERTIFICATE-----\n"
 */
static const unsigned char ca_certificate_2[] = {
#include "ca-cert-2.pem"
};

/*
 *      "-----BEGIN PRIVATE KEY-----\n"
 *      "-----KEY-----\n"
 *      "-----END PRIVATE KEY-----\n"
 */
static const unsigned char private_key_2[] = {
#include "private-key-2.pem"
};

/*
 *      "-----BEGIN CLIENT CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CLIENT CERTIFICATE-----\n"
 */
static const unsigned char device_certificate_2[] = {
#include "client-cert-2.pem"
};

#endif /* CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1 */

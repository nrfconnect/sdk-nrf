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

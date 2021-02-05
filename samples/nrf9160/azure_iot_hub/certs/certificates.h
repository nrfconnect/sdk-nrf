/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

static const unsigned char ca_certificate[] = {
#include "ca-cert.pem"
};

static const unsigned char private_key[] = {
#include "private-key.pem"
};

static const unsigned char device_certificate[] = {
#include "client-cert.pem"
};

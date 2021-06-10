/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef CARRIER_CERTS_H__
#define CARRIER_CERTS_H__

#ifdef CONFIG_LWM2M_CARRIER
#include <lwm2m_carrier.h>

int carrier_cert_provision(ca_cert_tags_t * const tags);
#endif /* CONFIG_LWM2M_CARRIER */

#endif /* CARRIER_CERTS_H__ */

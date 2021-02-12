/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Verify certificate file defines. */

#if defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)

#include CONFIG_NRF_CLOUD_CERTIFICATES_FILE

#ifndef NRF_CLOUD_CLIENT_ID
#error "Missing NRF_CLOUD_CLIENT_ID in certificate file"
#endif

#ifndef NRF_CLOUD_CA_CERTIFICATE
#error "Missing NRF_CLOUD_CA_CERTIFICATE in certificate file"
#endif

#ifndef NRF_CLOUD_CLIENT_PRIVATE_KEY
#warning "Missing NRF_CLOUD_CLIENT_PRIVATE_KEY in certificate file"
#endif

#ifndef NRF_CLOUD_CLIENT_PUBLIC_CERTIFICATE
#warning "Missing NRF_CLOUD_CLIENT_PUBLIC_CERTIFICATE in certificate file"
#endif

#endif /* CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES */

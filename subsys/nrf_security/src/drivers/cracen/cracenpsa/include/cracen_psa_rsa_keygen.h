/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_RSA_KEYGEN_H
#define CRACEN_PSA_RSA_KEYGEN_H

#include <cracen_psa_primitives.h>
#include <stdint.h>

/*
 * Generate an RSA private key, based on FIPS 186-4.
 */
int cracen_rsa_generate_privkey(uint8_t *pubexp, size_t pubexpsz, size_t keysz,
				struct cracen_rsa_key *privkey);

#endif /* CRACEN_PSA_RSA_KEYGEN_H */

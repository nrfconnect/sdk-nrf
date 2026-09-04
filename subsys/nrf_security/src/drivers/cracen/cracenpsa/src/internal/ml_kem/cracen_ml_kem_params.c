/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_kem_internal.h"

/* Parameter sets from FIPS 203, Table 2 (ML-KEM parameters) and Table 3 (sizes). */

#if defined(PSA_NEED_CRACEN_ML_KEM_512)
static const ml_kem_params_t ml_kem_512_params = {
	.k = 2,
	.eta1 = 3,
	.eta2 = 2,
	.du = 10,
	.dv = 4,
	.key_bits = 512,
	.pk_size = 800,
	.dk_size = 1632,
	.ciphertext_size = 768,
};
#endif /* PSA_NEED_CRACEN_ML_KEM_512 */

#if defined(PSA_NEED_CRACEN_ML_KEM_768)
static const ml_kem_params_t ml_kem_768_params = {
	.k = 3,
	.eta1 = 2,
	.eta2 = 2,
	.du = 10,
	.dv = 4,
	.key_bits = 768,
	.pk_size = 1184,
	.dk_size = 2400,
	.ciphertext_size = 1088,
};
#endif /* PSA_NEED_CRACEN_ML_KEM_768 */

#if defined(PSA_NEED_CRACEN_ML_KEM_1024)
static const ml_kem_params_t ml_kem_1024_params = {
	.k = 4,
	.eta1 = 2,
	.eta2 = 2,
	.du = 11,
	.dv = 5,
	.key_bits = 1024,
	.pk_size = 1568,
	.dk_size = 3168,
	.ciphertext_size = 1568,
};
#endif /* PSA_NEED_CRACEN_ML_KEM_1024 */

const ml_kem_params_t *cracen_ml_kem_params_get(size_t bits)
{
	switch (bits) {
#if defined(PSA_NEED_CRACEN_ML_KEM_512)
	case 512:
		return &ml_kem_512_params;
#endif /* PSA_NEED_CRACEN_ML_KEM_512 */
#if defined(PSA_NEED_CRACEN_ML_KEM_768)
	case 768:
		return &ml_kem_768_params;
#endif /* PSA_NEED_CRACEN_ML_KEM_768 */
#if defined(PSA_NEED_CRACEN_ML_KEM_1024)
	case 1024:
		return &ml_kem_1024_params;
#endif /* PSA_NEED_CRACEN_ML_KEM_1024 */
	default:
		return NULL;
	}
}

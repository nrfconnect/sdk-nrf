/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"

/* gamma2 = (q - 1) / val */
#define GAMMA2(val)	((ML_DSA_PRIME_NUM - 1) / val)
/* w1_max = (q - 1) / (2 * gamma2) - 1 */
#define W1_MAX(gamma2)	((uint8_t)((ML_DSA_PRIME_NUM - 1) / (uint32_t)(2 * gamma2) - 1))

/*
 * Parameter sets from FIPS 204, Table 1 (ML-DSA parameters) and Table 2
 * (sizes). The PSA key-bits value selects the set: 128 -> ML-DSA-44,
 * 192 -> ML-DSA-65, 256 -> ML-DSA-87.
 */

#if defined(PSA_NEED_CRACEN_ML_DSA_44)
static const ml_dsa_params_t ml_dsa_44_params = {
	.rows_k = 4,
	.columns_l = 4,
	.tau = 39,
	.lambda = 128,
	.beta = 78,
	.gamma1 = (1 << 17),
	.gamma2 = GAMMA2(88),
	.omega = 80,
	.w1_max = W1_MAX(GAMMA2(88)),
	.priv_key_size = 2560,
	.pk_size = 1312,
	.sig_size = 2420,
};
#endif /* PSA_NEED_CRACEN_ML_DSA_44 */

#if defined(PSA_NEED_CRACEN_ML_DSA_65)
static const ml_dsa_params_t ml_dsa_65_params = {
	.rows_k = 6,
	.columns_l = 5,
	.tau = 49,
	.lambda = 192,
	.beta = 196,
	.gamma1 = (1 << 19),
	.gamma2 = GAMMA2(32),
	.omega = 55,
	.w1_max = W1_MAX(GAMMA2(32)),
	.priv_key_size = 4032,
	.pk_size = 1952,
	.sig_size = 3309,
};
#endif /* PSA_NEED_CRACEN_ML_DSA_65 */

#if defined(PSA_NEED_CRACEN_ML_DSA_87)
static const ml_dsa_params_t ml_dsa_87_params = {
	.rows_k = 8,
	.columns_l = 7,
	.tau = 60,
	.lambda = 256,
	.beta = 120,
	.gamma1 = (1 << 19),
	.gamma2 = GAMMA2(32),
	.omega = 75,
	.w1_max = W1_MAX(GAMMA2(32)),
	.priv_key_size = 4896,
	.pk_size = 2592,
	.sig_size = 4627,
};
#endif /* PSA_NEED_CRACEN_ML_DSA_87 */

const ml_dsa_params_t *cracen_ml_dsa_params_get(size_t bits)
{
	switch (bits) {
#if defined(PSA_NEED_CRACEN_ML_DSA_44)
	case 128:
		return &ml_dsa_44_params;
#endif /* PSA_NEED_CRACEN_ML_DSA_44 */
#if defined(PSA_NEED_CRACEN_ML_DSA_65)
	case 192:
		return &ml_dsa_65_params;
#endif /* PSA_NEED_CRACEN_ML_DSA_65 */
#if defined(PSA_NEED_CRACEN_ML_DSA_87)
	case 256:
		return &ml_dsa_87_params;
#endif /* PSA_NEED_CRACEN_ML_DSA_87 */
	default:
		return NULL;
	}
}

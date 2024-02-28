/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_psa_primitives.h"
#include "common.h"
#include <cracen_psa.h>
#include <cracen/mem_helpers.h>
#include <psa/crypto.h>
#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/rsa.h>

#define SRP_FIELD_BITS 3072

/* From RFC3526
 * This prime is: 2^3072 - 2^3008 - 1 + 2^64 * { [2^2942 pi] + 1690314 }
 */
const uint8_t cracen_N3072[384] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2,
	0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67,
	0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E,
	0x34, 0x04, 0xDD, 0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
	0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45, 0xE4, 0x85, 0xB5,
	0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF,
	0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED, 0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE,
	0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
	0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05, 0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3,
	0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F, 0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3,
	0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB, 0x9E, 0xD5, 0x29, 0x07, 0x70,
	0x96, 0x96, 0x6D, 0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04, 0xF1, 0x74, 0x6C, 0x08,
	0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B, 0xE3, 0x9E, 0x77,
	0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F, 0xB5, 0xC5,
	0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9, 0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18, 0x39,
	0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
	0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D, 0xAD, 0x33, 0x17, 0x0D, 0x04, 0x50, 0x7A,
	0x33, 0xA8, 0x55, 0x21, 0xAB, 0xDF, 0x1C, 0xBA, 0x64, 0xEC, 0xFB, 0x85, 0x04, 0x58, 0xDB,
	0xEF, 0x0A, 0x8A, 0xEA, 0x71, 0x57, 0x5D, 0x06, 0x0C, 0x7D, 0xB3, 0x97, 0x0F, 0x85, 0xA6,
	0xE1, 0xE4, 0xC7, 0xAB, 0xF5, 0xAE, 0x8C, 0xDB, 0x09, 0x33, 0xD7, 0x1E, 0x8C, 0x94, 0xE0,
	0x4A, 0x25, 0x61, 0x9D, 0xCE, 0xE3, 0xD2, 0x26, 0x1A, 0xD2, 0xEE, 0x6B, 0xF1, 0x2F, 0xFA,
	0x06, 0xD9, 0x8A, 0x08, 0x64, 0xD8, 0x76, 0x02, 0x73, 0x3E, 0xC8, 0x6A, 0x64, 0x52, 0x1F,
	0x2B, 0x18, 0x17, 0x7B, 0x20, 0x0C, 0xBB, 0xE1, 0x17, 0x57, 0x7A, 0x61, 0x5D, 0x6C, 0x77,
	0x09, 0x88, 0xC0, 0xBA, 0xD9, 0x46, 0xE2, 0x08, 0xE2, 0x4F, 0xA0, 0x74, 0xE5, 0xAB, 0x31,
	0x43, 0xDB, 0x5B, 0xFC, 0xE0, 0xFD, 0x10, 0x8E, 0x4B, 0x82, 0xD1, 0x20, 0xA9, 0x3A, 0xD2,
	0xCA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
/* RFC5326 specifies 2 as generator, but Oberon PSA driver uses 5, to align that implementation
 * using same here
 */
static const uint8_t cracen_G3072[] = {5};

static psa_status_t set_password_key(cracen_srp_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *password, const size_t password_length)
{

	if (operation->role == PSA_PAKE_ROLE_CLIENT) {
		/* password = x = SHA512(salt | SHA512(user | ":" | pass)) */
		if (password_length != CRACEN_SRP_HASH_LENGTH) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		memcpy(operation->x, password, CRACEN_SRP_HASH_LENGTH);
	} else { /* PSA_PAKE_ROLE_SERVER */
		/* password = password_verifier = v = g^x mod N */
		if (password_length != CRACEN_SRP_FIELD_SIZE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		memcpy(operation->v, password, CRACEN_SRP_FIELD_SIZE);
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_srp_setup(cracen_srp_operation_t *operation,
			      const psa_key_attributes_t *attributes, const uint8_t *password,
			      size_t password_length, const psa_pake_cipher_suite_t *cipher_suite)
{
	if (psa_pake_cs_get_primitive(cipher_suite) !=
		    PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_DH, PSA_DH_FAMILY_RFC3526,
				       SRP_FIELD_BITS) ||
	    psa_pake_cs_get_key_confirmation(cipher_suite) != PSA_PAKE_CONFIRMED_KEY) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	psa_algorithm_t hash_alg = PSA_ALG_GET_HASH(psa_pake_cs_get_algorithm(cipher_suite));

	if (hash_alg != CRACEN_SRP_HASH_ALG) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return set_password_key(operation, attributes, password, password_length);
}

psa_status_t cracen_srp_set_user(cracen_srp_operation_t *operation, const uint8_t *user_id,
				 const size_t user_id_len)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t hash_length = 0;

	status =
		cracen_hash_compute(CRACEN_SRP_HASH_ALG, user_id, user_id_len, operation->user_hash,
				    sizeof(operation->user_hash), &hash_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	if (hash_length != CRACEN_SRP_HASH_LENGTH) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_srp_set_role(cracen_srp_operation_t *operation, const psa_pake_role_t role)
{
	if (role != PSA_PAKE_ROLE_CLIENT && role != PSA_PAKE_ROLE_SERVER) {
		return PSA_ERROR_NOT_SUPPORTED;
	}
	operation->role = role;

	return PSA_SUCCESS;
}

/* Calculate SRP-6a specified multiplier k = H(N, pad(g)) */
static psa_status_t cracen_srp_get_multiplier(uint8_t *k, size_t k_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_hash_operation_t hash_op = {0};
	size_t out_length = 0;

	/* k has to be of FIELD_SIZE to be used as padding later */
	if (k_length < CRACEN_SRP_FIELD_SIZE) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = cracen_hash_setup(&hash_op, CRACEN_SRP_HASH_ALG);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_hash_update(&hash_op, cracen_N3072, sizeof(cracen_N3072));
	if (status != PSA_SUCCESS) {
		return status;
	}
	/* padding same way Oberon PSA Driver does, assuming k is all zero */
	status = cracen_hash_update(&hash_op, k, CRACEN_SRP_FIELD_SIZE - sizeof(cracen_G3072));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_hash_update(&hash_op, cracen_G3072, sizeof(cracen_G3072));
	if (status != PSA_SUCCESS) {
		return status;
	}
	return cracen_hash_finish(&hash_op, k, k_length, &out_length);
}

/* A = g^a mod N */
static psa_status_t cracen_srp_calculate_client_key_share(cracen_srp_operation_t *operation,
							  uint8_t *output, const size_t output_size,
							  size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	/* a <- random() */
	status = cracen_get_random(NULL, operation->ab, sizeof(operation->ab));
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* g^a mod N */
	sx_op g = {.sz = sizeof(cracen_G3072), .bytes = (char *)cracen_G3072};
	sx_op a = {.sz = sizeof(operation->ab), .bytes = (char *)operation->ab};
	sx_op modulo = {.sz = sizeof(cracen_N3072), .bytes = (char *)cracen_N3072};
	sx_op result = {.sz = sizeof(operation->A), .bytes = operation->A};
	int sx_status = sx_mod_exp(NULL, &g, &a, &modulo, &result);

	status = silex_statuscodes_to_psa(sx_status);
	if (status != PSA_SUCCESS) {
		return status;
	}
	memcpy(output, operation->A, CRACEN_SRP_FIELD_SIZE);
	*output_length = CRACEN_SRP_FIELD_SIZE;

	return PSA_SUCCESS;
}

/* B = (v + g^b) mod N */
static psa_status_t cracen_srp_calculate_server_key_share(cracen_srp_operation_t *operation,
							  uint8_t *output, const size_t output_size,
							  size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t temp_value[CRACEN_SRP_FIELD_SIZE] = {0};
	uint8_t temp_value_2[CRACEN_SRP_FIELD_SIZE] = {0};
	/* k is only a hash but the buffer is used for padding in cracen_srp_get_multiplier */
	uint8_t k_value[CRACEN_SRP_FIELD_SIZE] = {0};

	/* b <- random() */
	status = cracen_get_random(NULL, operation->ab, sizeof(operation->ab));
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* b' = g^b */
	sx_op g = {.sz = sizeof(cracen_G3072), .bytes = (char *)cracen_G3072};
	sx_op b = {.sz = sizeof(operation->ab), .bytes = (char *)operation->ab};
	sx_op modulo = {.sz = sizeof(cracen_N3072), .bytes = (char *)cracen_N3072};
	sx_op temp_b = {.sz = sizeof(temp_value), .bytes = temp_value};
	int sx_status = sx_mod_exp(NULL, &g, &b, &modulo, &temp_b);

	status = silex_statuscodes_to_psa(sx_status);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	/* SRP-6a alternation
	 * k = H(p | pad(g))
	 * B = (kv + b') mod N
	 * While RFC2945 does
	 * B = (v + b') mod N
	 */

	/* k = H(p | pad(g)) */
	status = cracen_srp_get_multiplier(k_value, sizeof(k_value));
	if (status != PSA_SUCCESS) {
		goto error;
	}
	/* kv mod N, using output as temp storage */
	sx_op k = {.sz = CRACEN_SRP_HASH_LENGTH, .bytes = k_value};
	sx_op v = {.sz = sizeof(operation->v), .bytes = operation->v};
	sx_op kv = {.sz = sizeof(temp_value_2), .bytes = temp_value_2};
	const struct sx_pk_cmd_def *cmd_mul = SX_PK_CMD_ODD_MOD_MULT;

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, &modulo, &k, &v, &kv);
	status = silex_statuscodes_to_psa(sx_status);
	if (status != PSA_SUCCESS) {
		goto error;
	}

	/* B = (kv + b') mod N */
	sx_op result = {.sz = sizeof(operation->B), .bytes = operation->B};
	const struct sx_pk_cmd_def *cmd_add = SX_PK_CMD_MOD_ADD;

	sx_status = sx_mod_primitive_cmd(NULL, cmd_add, &modulo, &kv, &temp_b, &result);
	status = silex_statuscodes_to_psa(sx_status);
	if (status != PSA_SUCCESS) {
		goto error;
	}
	memcpy(output, operation->B, CRACEN_SRP_FIELD_SIZE);
	*output_length = CRACEN_SRP_FIELD_SIZE;
	return status;

error:
	safe_memzero(output, output_size);
	return status;
}

static psa_status_t cracen_srp_calculate_u(cracen_srp_operation_t *operation, uint8_t *u,
					   const size_t u_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_hash_operation_t hash_op = {0};
	size_t hash_length = 0;

	if (u_length < CRACEN_SRP_HASH_LENGTH) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* SRP-6a:
	 * u = H(A | B)
	 * RFC2945:
	 * u = first 32 bits of Hash(B)
	 */

	status = cracen_hash_setup(&hash_op, CRACEN_SRP_HASH_ALG);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_hash_update(&hash_op, operation->A, sizeof(operation->A));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_hash_update(&hash_op, operation->B, sizeof(operation->B));
	if (status != PSA_SUCCESS) {
		return status;
	}

	return cracen_hash_finish(&hash_op, u, CRACEN_SRP_HASH_LENGTH, &hash_length);
}

/* S = (B - k*g^x)^(a+ux) mod N  = (B - k*g^x)^a mod N * (B - k*g^x)^ux mod N*/
static psa_status_t cracen_srp_calculate_client_S(cracen_srp_operation_t *operation, uint8_t *S,
						  const size_t S_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t temp_value_0[CRACEN_SRP_FIELD_SIZE] = {0};
	uint8_t temp_value_1[CRACEN_SRP_FIELD_SIZE] = {0};

	if (S_length < CRACEN_SRP_FIELD_SIZE) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* g'= g^x mod N; using S as temp storage */
	sx_op g = {.sz = sizeof(cracen_G3072), .bytes = (char *)cracen_G3072};
	sx_op x = {.sz = sizeof(operation->x), .bytes = (char *)operation->x};
	sx_op modulo = {.sz = sizeof(cracen_N3072), .bytes = (char *)cracen_N3072};
	sx_op temp_g = {.sz = CRACEN_SRP_FIELD_SIZE, .bytes = S};
	int sx_status = sx_mod_exp(NULL, &g, &x, &modulo, &temp_g);

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* kg = kg' mod N*/
	status = cracen_srp_get_multiplier(temp_value_0, sizeof(temp_value_0));
	if (status != PSA_SUCCESS) {
		return status;
	}
	/* k is only a hash there only using the first bytes of the buffer */
	sx_op k = {.sz = CRACEN_SRP_HASH_LENGTH, .bytes = temp_value_0};
	sx_op kg = {.sz = sizeof(temp_value_1), .bytes = temp_value_1};
	const struct sx_pk_cmd_def *cmd_mul = SX_PK_CMD_ODD_MOD_MULT;

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, &modulo, &k, &temp_g, &kg);
	status = silex_statuscodes_to_psa(sx_status);
	if (status != PSA_SUCCESS) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* B' = (B - kg) mod N */
	sx_op B = {.sz = sizeof(operation->B), .bytes = operation->B};
	sx_op temp_b = {.sz = CRACEN_SRP_FIELD_SIZE, .bytes = S};
	const struct sx_pk_cmd_def *cmd_sub = SX_PK_CMD_MOD_SUB;

	sx_status = sx_mod_primitive_cmd(NULL, cmd_sub, &modulo, &B, &kg, &temp_b);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* t_1 = (B')^a mod N */
	sx_op a = {.sz = sizeof(operation->ab), .bytes = (char *)operation->ab};
	sx_op temp_1 = {.sz = sizeof(temp_value_1), .bytes = temp_value_1};

	sx_status = sx_mod_exp(NULL, &temp_b, &a, &modulo, &temp_1);

	/* get u */
	status = cracen_srp_calculate_u(operation, temp_value_0, sizeof(temp_value_0));
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* t_2 = (B')^u mod N */
	sx_op u = {.sz = CRACEN_SRP_HASH_LENGTH, .bytes = (char *)temp_value_0};
	sx_op temp_2 = {.sz = CRACEN_SRP_FIELD_SIZE, .bytes = S};

	sx_status = sx_mod_exp(NULL, &temp_b, &u, &modulo, &temp_2);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* t_3 = (t_2)^x mod N = ((B')^u)^x mod N = (B')^(ux) mod N */
	sx_op temp_3 = {.sz = sizeof(temp_value_0), .bytes = temp_value_0};

	sx_status = sx_mod_exp(NULL, &temp_2, &x, &modulo, &temp_3);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* B = t_1 * t_3 mod N */
	sx_op result = {.sz = CRACEN_SRP_FIELD_SIZE, .bytes = S};

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, &modulo, &temp_1, &temp_3, &result);
	return silex_statuscodes_to_psa(sx_status);
}

/* S = (Av^u)^b mod N */
static psa_status_t cracen_srp_calculate_server_S(cracen_srp_operation_t *operation, uint8_t *S,
						  const size_t S_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t u_buffer[CRACEN_SRP_HASH_LENGTH] = {0};
	uint8_t temp_value[CRACEN_SRP_FIELD_SIZE] = {0};

	if (S_length < CRACEN_SRP_FIELD_SIZE) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* get u */
	status = cracen_srp_calculate_u(operation, u_buffer, sizeof(u_buffer));
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* t_1 = v^u mod N */
	sx_op v = {.sz = sizeof(operation->v), .bytes = operation->v};
	sx_op u = {.sz = sizeof(u_buffer), .bytes = (char *)u_buffer};
	sx_op modulo = {.sz = sizeof(cracen_N3072), .bytes = (char *)cracen_N3072};
	sx_op temp_1 = {.sz = CRACEN_SRP_FIELD_SIZE, .bytes = S};
	int sx_status = sx_mod_exp(NULL, &v, &u, &modulo, &temp_1);

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* t_2 = A * t_1 mod N = (Av^u) mod N */
	sx_op temp_2 = {.sz = sizeof(temp_value), .bytes = temp_value};
	sx_op A = {.sz = sizeof(operation->A), .bytes = operation->A};
	const struct sx_pk_cmd_def *cmd_mul = SX_PK_CMD_ODD_MOD_MULT;

	sx_status = sx_mod_primitive_cmd(NULL, cmd_mul, &modulo, &A, &temp_1, &temp_2);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	/* t_2^b mod N = (Av^u)^b mod N = S */
	sx_op b = {.sz = sizeof(operation->ab), .bytes = (char *)operation->ab};
	sx_op result = {.sz = CRACEN_SRP_FIELD_SIZE, .bytes = S};

	sx_status = sx_mod_exp(NULL, &temp_2, &b, &modulo, &result);
	return silex_statuscodes_to_psa(sx_status);
}

static psa_status_t cracen_srp_SHA_interleaved(cracen_srp_operation_t *operation, const uint8_t *S,
					       size_t S_size)
{
	size_t K_length = 0;

	/* S' = skip leading zero bytes of S*/
	while (S_size > 0 && *S == 0) {
		S++;
		S_size--;
	}

	/* SRP-6a:
	 * k = H(S')
	 * RFC2945 specifies the following:
	 * E = S'[0] | S'[2] | S'[4] | ...
	 * F = S'[1] | S'[3] | S'[5] | ...
	 * G = Hash(E)
	 * H = Hash(F)
	 * K = G[0] | H[0] | G[1] | H[1] | ...
	 * Resulting in a key 2 * HASH_LENGTH
	 *
	 * To align with oberon use SRP-6a k = H(S') here
	 */

	return cracen_hash_compute(CRACEN_SRP_HASH_ALG, S, S_size, operation->K,
				   sizeof(operation->K), &K_length);
}

static psa_status_t cracen_srp_calculate_shared_key(cracen_srp_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t S[CRACEN_SRP_FIELD_SIZE] = {0};

	if (operation->role == PSA_PAKE_ROLE_CLIENT) {
		status = cracen_srp_calculate_client_S(operation, S, sizeof(S));
	} else { /* PSA_PAKE_ROLE_SERVER */
		status = cracen_srp_calculate_server_S(operation, S, sizeof(S));
	}

	if (status != PSA_SUCCESS) {
		return status;
	}

	return cracen_srp_SHA_interleaved(operation, S, CRACEN_SRP_FIELD_SIZE);
}

static psa_status_t cracen_srp_get_client_confirm(cracen_srp_operation_t *operation,
						  uint8_t *output, const size_t output_size,
						  size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_hash_operation_t hash_op = {0};
	size_t out_length = 0;
	uint8_t temp_value[MAX(CRACEN_SRP_HASH_LENGTH, CRACEN_SRP_FIELD_SIZE)];

	/* H(N) */
	status = cracen_hash_compute(CRACEN_SRP_HASH_ALG, cracen_N3072, sizeof(cracen_N3072),
				     temp_value, sizeof(temp_value), &out_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* H(g) */
	status = cracen_hash_compute(CRACEN_SRP_HASH_ALG, cracen_G3072, sizeof(cracen_G3072),
				     output, output_size, &out_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* H(N) xor H(g) */
	cracen_xorbytes(temp_value, output, CRACEN_SRP_HASH_LENGTH);

	/* M1 = H(H(N) xor H(g) | H(U) | salt | A | B | K ) */
	status = cracen_hash_setup(&hash_op, CRACEN_SRP_HASH_ALG);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* H( H(N) xor H(g) | */
	status = cracen_hash_update(&hash_op, temp_value, CRACEN_SRP_HASH_LENGTH);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* H(U) | */
	status = cracen_hash_update(&hash_op, operation->user_hash, sizeof(operation->user_hash));
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* salt | */
	status = cracen_hash_update(&hash_op, operation->salt, operation->salt_len);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* A | */
	status = cracen_hash_update(&hash_op, operation->A, sizeof(operation->A));
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* B | */
	status = cracen_hash_update(&hash_op, operation->B, sizeof(operation->B));
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* K ) */
	status = cracen_hash_update(&hash_op, operation->K, sizeof(operation->K));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_hash_finish(&hash_op, output, output_size, output_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	memcpy(operation->M, output, sizeof(operation->M));

	return status;
}

static psa_status_t cracen_srp_get_server_confirm(cracen_srp_operation_t *operation,
						  uint8_t *output, const size_t output_size,
						  size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_hash_operation_t hash_op = {0};

	/* M' = H( A | M | K ) */
	status = cracen_hash_setup(&hash_op, CRACEN_SRP_HASH_ALG);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_hash_update(&hash_op, operation->A, sizeof(operation->A));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_hash_update(&hash_op, operation->M, sizeof(operation->M));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_hash_update(&hash_op, operation->K, sizeof(operation->K));
	if (status != PSA_SUCCESS) {
		return status;
	}

	return cracen_hash_finish(&hash_op, output, output_size, output_length);
}

psa_status_t cracen_srp_output(cracen_srp_operation_t *operation, const psa_pake_step_t step,
			       uint8_t *output, const size_t output_size, size_t *output_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	switch (step) {
	case PSA_PAKE_STEP_KEY_SHARE:
		if (output_size < CRACEN_SRP_FIELD_SIZE) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}
		if (operation->role == PSA_PAKE_ROLE_CLIENT) {
			return cracen_srp_calculate_client_key_share(operation, output, output_size,
								     output_length);
		} else { /* PSA_PAKE_ROLE_SERVER */
			return cracen_srp_calculate_server_key_share(operation, output, output_size,
								     output_length);
		}
		break;
	case PSA_PAKE_STEP_CONFIRM:
		if (output_size < CRACEN_SRP_HASH_LENGTH) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		/* Calculate shared key K and store it in the operation context */
		status = cracen_srp_calculate_shared_key(operation);
		if (status != PSA_SUCCESS) {
			return status;
		}
		if (operation->role == PSA_PAKE_ROLE_CLIENT) {
			status = cracen_srp_get_client_confirm(operation, output, output_size,
							       output_length);
		} else { /* PSA_PAKE_ROLE_SERVER */
			status = cracen_srp_get_server_confirm(operation, output, output_size,
							       output_length);
		}
		if (status != PSA_SUCCESS) {
			*output_length = 0;
			return status;
		}
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return status;
}

static psa_status_t cracen_srp_store_key_share(cracen_srp_operation_t *operation,
					       const uint8_t *input, const size_t input_length)
{
	if (input_length != CRACEN_SRP_FIELD_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	if (operation->role == PSA_PAKE_ROLE_CLIENT) {
		memcpy(operation->B, input, CRACEN_SRP_FIELD_SIZE);
	} else { /* PSA_PAKE_ROLE_SERVER */
		memcpy(operation->A, input, CRACEN_SRP_FIELD_SIZE);
	}

	return PSA_SUCCESS;
}

static psa_status_t cracen_srp_check_confirm(cracen_srp_operation_t *operation,
					     const uint8_t *input, const size_t input_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t M[CRACEN_SRP_HASH_LENGTH] = {0};
	size_t out_length = 0;

	if (input_length != sizeof(M)) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	if (operation->role == PSA_PAKE_ROLE_CLIENT) {
		status = cracen_srp_get_server_confirm(operation, M, sizeof(M), &out_length);
	} else { /* PSA_PAKE_ROLE_SERVER */
		/* First the Client sends the proof to the server, so to verify the shared key
		 * needs to be calculated
		 */
		status = cracen_srp_calculate_shared_key(operation);
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_srp_get_client_confirm(operation, M, sizeof(M), &out_length);
	}

	if (status != PSA_SUCCESS) {
		return status;
	}

	if (constant_memcmp(input, M, sizeof(M))) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_srp_input(cracen_srp_operation_t *operation, const psa_pake_step_t step,
			      const uint8_t *input, const size_t input_length)
{

	switch (step) {
	case PSA_PAKE_STEP_SALT:
		if (input_length > sizeof(operation->salt)) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		memcpy(operation->salt, input, input_length);
		operation->salt_len = input_length;
		return PSA_SUCCESS;
	case PSA_PAKE_STEP_KEY_SHARE:
		return cracen_srp_store_key_share(operation, input, input_length);
	case PSA_PAKE_STEP_CONFIRM:
		return cracen_srp_check_confirm(operation, input, input_length);
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}
}

psa_status_t cracen_srp_get_shared_key(cracen_srp_operation_t *operation,
				       const psa_key_attributes_t *attributes, uint8_t *output,
				       const size_t output_size, size_t *output_length)
{
	if (output_size < CRACEN_SRP_HASH_LENGTH) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(output, &operation->K, CRACEN_SRP_HASH_LENGTH);
	*output_length = CRACEN_SRP_HASH_LENGTH;

	return PSA_SUCCESS;
}

psa_status_t cracen_srp_abort(cracen_srp_operation_t *operation)
{
	safe_memzero(operation, sizeof(*operation));

	return PSA_SUCCESS;
}

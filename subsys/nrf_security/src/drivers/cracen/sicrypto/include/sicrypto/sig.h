/** Signature generation and verification and related keys.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_SIG_HEADER_FILE
#define SICRYPTO_SIG_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "rsa_keys.h"
#include "ecc.h"

struct sitask;
struct si_sig_def;
struct si_pk_ecurve;

/** An isolated key.
 *
 * This is an opaque type. It should not be used directly.
 */
struct si_ik_key {
	const struct sx_pk_ecurve *curve;
	int index;
};

/** Structure to represent a private key of any type. */
struct si_sig_privkey {
	/* Defines the algorithm that the key can be used with. */
	const struct si_sig_def *def;
	const struct sxhashalg *hashalg; /**< Hash algorithm to use. */
	size_t saltsz;			 /**< Salt size in bytes. */
	union {
		struct si_eccsk eckey;
		struct si_rsa_key rsa;
		struct sx_ed25519_v *ed25519;
		struct sx_x25519_op *x25519;
		struct sx_ed448_v *ed448;
		struct sx_x448_op *x448;
		struct si_ik_key ref;
	} key; /**< Union for algorithm-dependent key data. */
};

/** Structure to represent a public key of any type. */
struct si_sig_pubkey {
	/* Defines the algorithm that the key can be used with. */
	const struct si_sig_def *def;
	const struct sxhashalg *hashalg; /**< Hash algorithm to use. */
	size_t saltsz;			 /**< Salt size in bytes. */
	union {
		struct si_eccpk eckey;
		struct si_rsa_key rsa;
		struct sx_ed25519_pt *ed25519;
		struct sx_x25519_pt *x25519;
		struct sx_ed448_pt *ed448;
		struct sx_x448_pt *x448;
	} key; /**< Union for algorithm-dependent key data. */
};

/** Structure to represent a signature of any type.
 *
 * When this structure is used to represent an ECDSA signature, members `r` and
 * `s` point to the signature elements with the same names. When used to
 * represent EdDSA or RSA signatures, only member `r` is used (these signatures
 * are seen as composed of a single element).
 */
struct si_sig_signature {
	size_t sz; /**< Total signature size, in bytes. */
	char *r;   /**< Signature element "r". */
	char *s;   /**< Signature element "s". */
};

/** Create a task to compute a signature.
 *
 * @param[in,out] t         Task structure to use.
 * @param[in] privkey       Private key.
 * @param[out] signature    Output signature.
 *
 * The \p privkey argument specifies the key and the signature algorithm to use.
 *
 * The signature algorithm defined in `si_sig_privkey::def` imposes some
 * constraints on the task. See the documentation of the value used in
 * `si_sig_privkey::def` for the details.
 */
void si_sig_create_sign(struct sitask *t, const struct si_sig_privkey *privkey,
			struct si_sig_signature *signature);

/** Create a task to compute a signature from a message digest.
 *
 * @param[in,out] t         Task structure to use.
 * @param[in] privkey       Private key.
 * @param[out] signature    Output signature.
 *
 * The \p privkey argument specifies the key and the signature algorithm to use.
 * Not all key types allow this operation.
 *
 * The signature algorithm defined in `si_sig_privkey::def` imposes some
 * constraints on the task. See the documentation of the value used in
 * `si_sig_privkey::def` for the details.
 */
void si_sig_create_sign_digest(struct sitask *t, const struct si_sig_privkey *privkey,
			       struct si_sig_signature *signature);

/** Create a task to verify a signature.
 *
 * @param[in,out] t         Task structure to use.
 * @param[in] pubkey        Public key.
 * @param[in] signature     Signature to be verified.
 *
 * The \p pubkey argument specifies the key and the signature algorithm to use.
 *
 * The signature algorithm defined in `si_sig_privkey::def` imposes some
 * constraints on the task. See the documentation of the value used in
 * `si_sig_privkey::def` for the details.
 */
void si_sig_create_verify(struct sitask *t, const struct si_sig_pubkey *pubkey,
			  const struct si_sig_signature *signature);

/** Create a task to verify a signature from a message digest.
 *
 * @param[in,out] t         Task structure to use.
 * @param[in] pubkey        Public key.
 * @param[in] signature     Signature to be verified.
 *
 * The \p pubkey argument specifies the key and the signature algorithm to use.
 * Not all key types allow this operation.
 *
 * The signature algorithm defined in `si_sig_privkey::def` imposes some
 * constraints on the task. See the documentation of the value used in
 * `si_sig_privkey::def` for the details.
 */
void si_sig_create_verify_digest(struct sitask *t, const struct si_sig_pubkey *pubkey,
				 const struct si_sig_signature *signature);

/** Create a task to compute the public key from a given private key.
 *
 * @param[in,out] t         Task structure to use.
 * @param[in] privkey       Private key.
 * @param[out] pubkey       Public key.
 *
 * The \p privkey argument specifies the key and the signature algorithm to use.
 * Not all key types allow this operation.
 *
 * The signature algorithm defined in `si_sig_privkey::def` imposes some
 * constraints on the task. See the documentation of the value used in
 * `si_sig_privkey::def` for the details.
 */
void si_sig_create_pubkey(struct sitask *t, const struct si_sig_privkey *privkey,
			  struct si_sig_pubkey *pubkey);

/** Return the total memory size needed to store the components of a signature
 */
size_t si_sig_get_total_sig_size(const struct si_sig_privkey *privkey);

#ifdef __cplusplus
}
#endif

#endif

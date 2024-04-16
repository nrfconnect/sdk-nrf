/** RSA OAEP encryption and decryption.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_RSAOAEP_HEADER_FILE
#define SICRYPTO_RSAOAEP_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <silexpk/sxbuf/sxbufop.h>
#include "rsa_keys.h"

struct sitask;

/** Create a task to perform RSAES-OAEP message encryption.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] hashalg      Hash algorithm to use.
 * @param[in] pubkey       Public key.
 * @param[in,out] text     Input plaintext and, on task completion, the
 *                         ciphertext.
 * @param[in]     label    Label to be used by the operation.
 *
 * When calling this function, the \p text argument must provide the plaintext
 * to encrypt. On successful task completion, \p text points to the task's
 * result, the ciphertext, which is inside the workmem memory area.
 *
 * The buffers holding the public key do not need to be in DMA memory.
 *
 * The PRNG in the sicrypto environment must have been set up prior to calling
 * this function.
 *
 * This task needs a workmem buffer with size:
 *      rsa_modulus_size + hash_digest_size + 4
 * where all sizes are expressed in bytes.
 *
 * The hash algorithm selected via \p hashalg is used in the EME-OAEP encoding
 * and in the MGF1 function.
 *
 * This task assumes that the provided RSA key is valid. If the user provides an
 * invalid RSA key (e.g. one where the exponent is not less than the modulus),
 * this is not detected by sicrypto.
 */
void si_ase_create_rsa_oaep_encrypt(struct sitask *t, const struct sxhashalg *hashalg,
				    struct si_rsa_key *pubkey, struct si_ase_text *text,
				    struct sx_buf *label);

/** Create a task to perform RSAES-OAEP message decryption.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] hashalg      Hash algorithm to use.
 * @param[in] privkey      Private key.
 * @param[in,out] text     Input ciphertext and, on task completion, the
 *                         decrypted text.
 * @param[in]     label    Label to be used by the operation.
 *
 * When calling this function, the \p text argument must provide the ciphertext
 * to decrypt. On successful task completion, \p text points to the task's
 * result, the decrypted text, which is inside the workmem memory area.
 *
 * The buffers holding the private key do not need to be in DMA memory.
 *
 * This task needs a workmem buffer with size:
 *      rsa_modulus_size + hash_digest_size + 4
 * where all sizes are expressed in bytes.
 *
 * The hash algorithm selected via \p hashalg is used in the EME-OAEP decoding
 * and in the MGF1 function.
 *
 * This task assumes that the provided RSA key is valid. If the user provides an
 * invalid RSA key (e.g. one where the exponent is not less than the modulus),
 * this is not detected by sicrypto.
 */
void si_ase_create_rsa_oaep_decrypt(struct sitask *t, const struct sxhashalg *hashalg,
				    struct si_rsa_key *privkey, struct si_ase_text *text,
				    struct sx_buf *label);

#ifdef __cplusplus
}
#endif

#endif

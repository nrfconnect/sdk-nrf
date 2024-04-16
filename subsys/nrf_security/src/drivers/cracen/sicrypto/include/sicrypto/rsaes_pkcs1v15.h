/** RSA PKCS1 v1.5 encryption and decryption.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_RSAES_PKCS1V15_HEADER_FILE
#define SICRYPTO_RSAES_PKCS1V15_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "rsa_keys.h"
#include <silexpk/sxbuf/sxbufop.h>

struct sitask;

/** Create a task to perform RSAES-PKCS1-v1_5 message encryption.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] pubkey       Public key.
 * @param[in,out] text     Input plaintext and, on task completion, the
 *                         ciphertext.
 *
 * When calling this function, the \p text argument must provide the plaintext
 * to encrypt. On successful task completion, \p text has been changed to point
 * to the task's result, the ciphertext, inside the workmem memory area.
 *
 * The buffers holding the public key do not need to be in DMA memory.
 *
 * The PRNG in the sicrypto environment must have been set up prior to calling
 * this function.
 *
 * This task needs a workmem buffer with size at least equal to size of the RSA
 * modulus.
 *
 * This task assumes that the provided RSA key is valid. If the user provides an
 * invalid RSA key (e.g. one where the exponent is not less than the modulus),
 * this is not detected by sicrypto.
 */
void si_ase_create_rsa_pkcs1v15_encrypt(struct sitask *t, struct si_rsa_key *pubkey,
					struct si_ase_text *text);

/** Create a task to perform RSAES-PKCS1-v1_5 message decryption.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] privkey      Private key.
 * @param[in,out] text     Input ciphertext and, on task completion, the
 *                         decrypted text.
 *
 * When calling this function, the \p text argument must provide the ciphertext
 * to decrypt. On successful task completion, \p text has been changed to point
 * to the task's result, the decrypted text, inside the workmem memory area.
 *
 * The buffers holding the private key do not need to be in DMA memory.
 *
 * This task needs a workmem buffer with size at least equal to size of the RSA
 * modulus.
 *
 * This task assumes that the provided RSA key is valid. If the user provides an
 * invalid RSA key (e.g. one where the exponent is not less than the modulus),
 * this is not detected by sicrypto.
 */
void si_ase_create_rsa_pkcs1v15_decrypt(struct sitask *t, struct si_rsa_key *privkey,
					struct si_ase_text *text);

#ifdef __cplusplus
}
#endif

#endif

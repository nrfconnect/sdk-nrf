/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_STATUSCODES_HEADER_FILE
#define CRACEN_STATUSCODES_HEADER_FILE

/**
 * @addtogroup
 *
 * @{
 */

/** The function or operation succeeded */
#define SX_OK 0

/** The function or operation was given an invalid parameter */
#define SX_ERR_INVALID_PARAM 1

/** Unknown error */
#define SX_ERR_UNKNOWN_ERROR 2

/** The operation is still executing */
#define SX_ERR_BUSY 3

/** The input operand is not a quadratic residue */
#define SX_ERR_NOT_QUADRATIC_RESIDUE 4

/** The input value for Rabin-Miller test is a composite value */
#define SX_ERR_COMPOSITE_VALUE 5

/** Inversion of non-invertible value */
#define SX_ERR_NOT_INVERTIBLE 6

/** The signature is not valid
 *
 * This error can happen during signature generation
 * and signature verification
 */
#define SX_ERR_INVALID_SIGNATURE 7

/** The functionality or operation is not supported */
#define SX_ERR_NOT_IMPLEMENTED 8

/** The output operand is a point at infinity */
#define SX_ERR_POINT_AT_INFINITY 9

/** The input value is outside the expected range */
#define SX_ERR_OUT_OF_RANGE 10

/** The modulus has an unexpected value
 *
 * This error happens when the modulus is zero or
 * even when odd modulus is expected
 */
#define SX_ERR_INVALID_MODULUS 11

/** The input point is not on the defined elliptic curve */
#define SX_ERR_POINT_NOT_ON_CURVE 12

/** The input operand is too large
 *
 * Happens when an operand is bigger than one of the following:
 * - The hardware operand size: Check ::sx_pk_capabilities
 * to see the hardware operand size limit.
 * - The operation size: Check sx_pk_get_opsize() to get
 * the operation size limit.
 */
#define SX_ERR_OPERAND_TOO_LARGE 13

/** A platform specific error */
#define SX_ERR_PLATFORM_ERROR 14

/** The evaluation period for the product expired */
#define SX_ERR_EXPIRED 15

/** The hardware is still in IK mode
 *
 * This error happens when a normal operation
 * is started and the hardware is still in IK mode.
 * Run command ::SX_PK_CMD_IK_EXIT to exit the IK
 * mode and to run normal operations again
 */
#define SX_ERR_IK_MODE 16

/** The parameters of the elliptic curve are not valid. */
#define SX_ERR_INVALID_CURVE_PARAM 17

/** The IK module is not ready
 *
 * This error happens when a IK operation is started
 * but the IK module has not yet generated the internal keys.
 * Call sx_pk_ik_derive_keys() before starting IK operations.
 */
#define SX_ERR_IK_NOT_READY 18

/** Resources not available for a new operation. Retry later */
#define SX_ERR_PK_RETRY 19

/** The size passed as part of a request is not valid */
#define SX_ERR_INVALID_REQ_SZ 100

/** A PRNG reseed is required */
#define SX_ERR_RESEED_NEEDED 101

/** An invalid parameter was passed to a function */
#define SX_ERR_INVALID_ARG 102

/** The task is ready for consume, produce, run or partial run */
#define SX_ERR_READY 103

/** Hash algorithm not supported */
#define SX_ERR_UNSUPPORTED_HASH_ALG 104

/** The ciphertext is not valid */
#define SX_ERR_INVALID_CIPHERTEXT 105

/** The number of attempts in an algorithm has exceeded a given threshold */
#define SX_ERR_TOO_MANY_ATTEMPTS 106

/** Range checks, performed on p and/or q during RSA key generation, failed */
#define SX_ERR_RSA_PQ_RANGE_CHECK_FAIL 107

/** Task needs bigger workmem buffer than what provided with si_task_init() */
#define SX_ERR_WORKMEM_BUFFER_TOO_SMALL 109

/** Waiting on the hardware to process this operation */
#define SX_ERR_HW_PROCESSING (-1)

/** No hardware available for a new operation. Retry later. */
#define SX_ERR_RETRY (-2)

/** No compatible hardware for this operation.
 *
 * This error occurs if the dedicated hardware to execute the operation is not
 * present, or hardware is present and operation not supported by it.
 */
#define SX_ERR_INCOMPATIBLE_HW (-3)

/** Invalid authentication tag in authenticated decryption */
#define SX_ERR_INVALID_TAG (-16)

/** Hardware DMA error
 *
 * Fatal error that should never happen. Can be caused by invalid or
 * wrong addresses, RAM corruption, a hardware or software bug or system
 * corruption.
 */
#define SX_ERR_DMA_FAILED (-32)

/** Fatal error, trying to call a function with an uninitialized object
 *
 * For example calling sx_aead_decrypt() with an sxaead object which
 * has not been created yet with sx_aead_create_*() function.
 */
#define SX_ERR_UNINITIALIZED_OBJ (-33)

/** Fatal error, trying to call an AEAD or block cipher create function with an
 * uninitialized or invalid key reference.
 *
 * Examples: calling sx_blkcipher_create_aesecb() with a key reference which
 * has not been initialized yet with sx_keyref_load_material() or sx_keyref_load_by_id()
 * function, sx_keyref_load_material() was called with key NULL or size 0, or
 * sx_keyref_load_by_id() was called with an invalid index ID.
 */
#define SX_ERR_INVALID_KEYREF (-34)

/** Fatal error, trying to create instance with not enough memory */
#define SX_ERR_ALLOCATION_TOO_SMALL (-35)

/** Input or output buffer size too large */
#define SX_ERR_TOO_BIG (-64)

/** Output buffer size too small */
#define SX_ERR_OUTPUT_BUFFER_TOO_SMALL (-65)

/** The given key size is not supported by the algorithm or the hardware */
#define SX_ERR_INVALID_KEY_SZ (-66)

/** Input tag size is invalid */
#define SX_ERR_INVALID_TAG_SIZE (-67)

/** Input nonce size is invalid */
#define SX_ERR_INVALID_NONCE_SIZE (-68)

/** Too many feeds were inputed */
#define SX_ERR_FEED_COUNT_EXCEEDED (-69)

/** Input data size granularity is incorrect */
#define SX_ERR_WRONG_SIZE_GRANULARITY (-70)

/** Attempt to use HW keys with a mode that does not support HW keys */
#define SX_ERR_HW_KEY_NOT_SUPPORTED (-71)

/** Attempt to use a mode or engine that does not support context saving */
#define SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED (-72)

/** Attempt to feed AAD after input data was fed */
#define SX_ERR_FEED_AFTER_DATA (-73)

/** Hardware cannot work anymore.
 *
 * To recover, reset the hardware or call *_init() function.
 * For example, if this error is received when sx_trng_get()
 * is called, call again sx_trng_open() to reset the hardware.
 */
#define SX_ERR_RESET_NEEDED (-82)

/** Input buffer size too small. */
#define SX_ERR_INPUT_BUFFER_TOO_SMALL (-83)

/** Size of entropy buffer is too small. */
#define SX_ERR_INSUFFICIENT_ENTROPY (-84)

/** The system appears to have been tampered with. */
#define SX_ERR_CORRUPTION_DETECTED (-85)

/** @} */

#endif

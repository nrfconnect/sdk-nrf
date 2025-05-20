/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <errno.h>
#include <cracen/statuscodes.h>

/**
 * @brief Function to convert CRACEN SW library error codes to errno
 *
 * @param[in] sx_err	Error code from sxsymcrypt, silexpk or sicrypto.
 *
 * @return 0 or non-zero return codes from ernno.
 */
inline __attribute__((always_inline)) int sx_err2errno(int sx_err)
{
	switch (sx_err) {
	case SX_OK:
		/* The function or operation succeeded */
		return 0;
	/* Error codes from sxsymcrypt */
	case SX_ERR_HW_PROCESSING:
		/* Waiting on the hardware to process this operation */
		return -EBUSY;
	case SX_ERR_RETRY:
		/* Waiting on the hardware to process this operation */
		return -EAGAIN;
	case SX_ERR_INCOMPATIBLE_HW:
		/**
		 * No compatible hardware for this operation.
		 *
		 * This error occurs if the dedicated hardware to execute the
		 * operation is not present, or hardware is present and
		 * operation not supported by it.
		 */
		return -ENOTSUP;
	case SX_ERR_INVALID_TAG:
		/* Invalid authentication tag in authenticated decryption */
		return -EBADMSG;
	case SX_ERR_DMA_FAILED:
		/**
		 * Hardware DMA error
		 *
		 * Fatal error that should never happen. Can be caused by
		 * invalid or wrong addresses, RAM corruption, a hardware or
		 * software bug or system corruption.
		 */
		return -EFAULT;
	case SX_ERR_UNINITIALIZED_OBJ:
		/**
		 * Fatal error, trying to call a function with an uninitialized
		 * object
		 *
		 * For example calling sx_aead_decrypt() with an sxaead object
		 * which has not been created yet with sx_aead_create_*()
		 * function.
		 */
		return -EINVAL;
	case SX_ERR_INVALID_KEYREF:
		/**
		 * Fatal error, trying to call an AEAD or block cipher create
		 * function with an uninitialized or invalid key reference.
		 *
		 * Examples: calling sx_blkcipher_create_aesecb() with a key
		 * reference which has not been initialized yet with
		 * sx_keyref_load_material() or sx_keyref_load_by_id() function,
		 * sx_keyref_load_material() was called with key NULL or size 0,
		 * or sx_keyref_load_by_id() was called with an invalid index
		 * ID.
		 */
		return -EINVAL;
	case SX_ERR_ALLOCATION_TOO_SMALL:
		/* Fatal error, trying to create instance with not enough memory
		 */
		return -ENOMEM;
	case SX_ERR_TOO_BIG:
		/* Input or output buffer size too large */
		return -EINVAL;
	case SX_ERR_INPUT_BUFFER_TOO_SMALL:
	case SX_ERR_OUTPUT_BUFFER_TOO_SMALL:
		/* Input or output buffer size too small */
		return -EINVAL;
	case SX_ERR_INVALID_KEY_SZ:
		/* The given key size is not supported by the algorithm or the
		 * hardware
		 */
		return -EINVAL;
	case SX_ERR_INVALID_TAG_SIZE:
		/* Input tag size is invalid */
		return -EINVAL;
	case SX_ERR_INVALID_NONCE_SIZE:
		/* Input nonce size is invalid */
		return -EINVAL;
	case SX_ERR_FEED_COUNT_EXCEEDED:
		/* Too many feeds were inputed */
		return -ENOMSG;
	case SX_ERR_WRONG_SIZE_GRANULARITY:
		/* Input data size granularity is incorrect */
		return -EINVAL;
	case SX_ERR_HW_KEY_NOT_SUPPORTED:
		/* Attempt to use HW keys with a mode that does not support HW
		 * keys
		 */
		return -ENOTSUP;
	case SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED:
		/* Attempt to use a mode or engine that does not support context
		 * saving
		 */
		return -ENOTSUP;
	case SX_ERR_FEED_AFTER_DATA:
		/* Attempt to feed AAD after input data was fed */
		return -ENOMSG;
	case SX_ERR_RESET_NEEDED:
		/**
		 * Hardware cannot work anymore.
		 *
		 * To recover, reset the hardware or call *_init() function.
		 * For example, if this error is received when sx_trng_get()
		 * is called, call again sx_trng_open() to reset the hardware.
		 */
		return -EHOSTDOWN;
	/* End of error codes from sxsymcrypt */
	/* Error codes from silexpk */
	case SX_ERR_INVALID_PARAM:
		/* The function or operation was given an invalid parameter */
		return -EINVAL;
	case SX_ERR_UNKNOWN_ERROR:
		/* Unknown error */
		return -EFAULT;
	case SX_ERR_BUSY:
		/* The operation is still executing */
		return -EBUSY;
	case SX_ERR_NOT_QUADRATIC_RESIDUE:
		/* The input operand is not a quadratic residue */
		return -EINVAL;
	case SX_ERR_COMPOSITE_VALUE:
		/* The input value for Rabin-Miller test is a composite value */
		return -EINVAL;
	case SX_ERR_NOT_INVERTIBLE:
		/* Inversion of non-invertible value */
		return -EINVAL;
	case SX_ERR_INVALID_SIGNATURE:
		/**
		 * The signature is not valid
		 *
		 * This error can happen during signature generation
		 * and signature verification
		 */
		return -EBADMSG;
	case SX_ERR_NOT_IMPLEMENTED:
		/* The functionality or operation is not supported */
		return -ENOTSUP;
	case SX_ERR_POINT_AT_INFINITY:
		/* The output operand is a point at infinity */
		return -EDOM;
	case SX_ERR_OUT_OF_RANGE:
		/* The input value is outside the expected range */
		return -EDOM;
	case SX_ERR_INVALID_MODULUS:
		/**
		 * The modulus has an unexpected value
		 *
		 * This error happens when the modulus is zero or
		 * even when odd modulus is expected
		 */
		return -EDOM;
	case SX_ERR_POINT_NOT_ON_CURVE:
		/* The input point is not on the defined elliptic curve */
		return -EDOM;
	case SX_ERR_OPERAND_TOO_LARGE:
		/**
		 * The input operand is too large
		 *
		 * Happens when an operand is bigger than one of the following:
		 * - The hardware operand size: Check ::sx_pk_capabilities
		 * to see the hardware operand size limit.
		 * - The operation size: Check sx_pk_get_opsize() to get
		 * the operation size limit.
		 */
		return -EDOM;
	case SX_ERR_PLATFORM_ERROR:
		/* A platform specific error */
		return -EFAULT;
	case SX_ERR_EXPIRED:
		/* The evaluation period for the product expired */
		return -ETIME;
	case SX_ERR_IK_MODE:
		/**
		 * The hardware is still in IK mode
		 *
		 * This error happens when a normal operation
		 * is started and the hardware is still in IK mode.
		 * Run command ::SX_PK_CMD_IK_EXIT to exit the IK
		 * mode and to run normal operations again
		 */
		return -ENOMSG;
	case SX_ERR_INVALID_CURVE_PARAM:
		/* The parameters of the elliptic curve are not valid. */
		return -EDOM;
	case SX_ERR_IK_NOT_READY:
		/**
		 * The IK module is not ready
		 *
		 * This error happens when a IK operation is started
		 * but the IK module has not yet generated the internal keys.
		 * Call sx_pk_ik_derive_keys() before starting IK operations.
		 */
		return -EBUSY;
	case SX_ERR_PK_RETRY:
		/* Resources not available for a new operation. Retry later */
		return -EBUSY;
	/* End of codes from silexpk */
	/* Error codes from sicrypto */
	case SX_ERR_INVALID_REQ_SZ:
		/** The size passed as part of a request is not valid */
		return -EINVAL;
	case SX_ERR_RESEED_NEEDED:
		/** A PRNG reseed is required */
		return -EHOSTDOWN;
	case SX_ERR_INVALID_ARG:
		/** An invalid parameter was passed to a function */
		return -EINVAL;
	case SX_ERR_READY:
		/** The task is ready for consume, produce, run or partial run
		 */
		return 0;
	case SX_ERR_UNSUPPORTED_HASH_ALG:
		/** Hash algorithm not supported */
		return -EINVAL;
	case SX_ERR_INVALID_CIPHERTEXT:
		/** The ciphertext is not valid */
		return -EBADMSG;
	case SX_ERR_TOO_MANY_ATTEMPTS:
		/** The number of attempts in an algorithm has exceeded a given
		 * threshold
		 */
		return -ECONNREFUSED;
	case SX_ERR_RSA_PQ_RANGE_CHECK_FAIL:
		/** Range checks, performed on p and/or q during RSA key
		 * generation, failed
		 */
		return -EDOM;
	case SX_ERR_WORKMEM_BUFFER_TOO_SMALL:
		/** Task needs bigger workmem buffer than what provided with
		 * si_task_init()
		 */
		return -ENOMEM;
	case SX_ERR_INSUFFICIENT_ENTROPY:
		/** Size of entropy buffer is too small. */
		return -EINVAL;
	/* End of error codes from sicrypto */
	default:
		/**
		 * Return "not supported" for any error-codes that are not
		 * handled.
		 */
		return -ENOTSUP;
	}
}

/**
 * @brief Function to convert CRACEN SW library error codes to strings for
 * logging
 *
 * @param[in] sx_err	Error code from sxsymcrypt, silexpk or sicrypto.
 *
 * @return String literal representing the error code.
 */
inline __attribute__((always_inline)) const char *sx_err2str(int sx_err)
{
	switch (sx_err) {
	case SX_OK:
		/* The function or operation succeeded */
		return "SX_OK";
	/* Error codes from sxsymcrypt */
	case SX_ERR_HW_PROCESSING:
		/* Waiting on the hardware to process this operation */
		return "SX_ERR_HW_PROCESSING";
	case SX_ERR_RETRY:
		/** Waiting on the hardware to process this operation */
		return "SX_ERR_RETRY";
	case SX_ERR_INCOMPATIBLE_HW:
		/**
		 * No compatible hardware for this operation.
		 *
		 * This error occurs if the dedicated hardware to execute the
		 * operation is not present, or hardware is present and
		 * operation not supported by it.
		 */
		return "SX_ERR_INCOMPATIBLE_HW";
	case SX_ERR_INVALID_TAG:
		/* Invalid authentication tag in authenticated decryption */
		return "SX_ERR_INVALID_TAG";
	case SX_ERR_DMA_FAILED:
		/**
		 * Hardware DMA error
		 *
		 * Fatal error that should never happen. Can be caused by
		 * invalid or wrong addresses, RAM corruption, a hardware or
		 * software bug or system corruption.
		 */
		return "SX_ERR_DMA_FAILED";
	case SX_ERR_UNINITIALIZED_OBJ:
		/**
		 * Fatal error, trying to call a function with an uninitialized
		 * object
		 *
		 * For example calling sx_aead_decrypt() with an sxaead object
		 * which has not been created yet with sx_aead_create_*()
		 * function.
		 */
		return "SX_ERR_UNINITIALIZED_OBJ";
	case SX_ERR_INVALID_KEYREF:
		/**
		 * Fatal error, trying to call an AEAD or block cipher create
		 * function with an uninitialized or invalid key reference.
		 *
		 * Examples: calling sx_blkcipher_create_aesecb() with a key
		 * reference which has not been initialized yet with
		 * sx_keyref_load_material() or sx_keyref_load_by_id() function,
		 * sx_keyref_load_material() was called with key NULL or size 0,
		 * or sx_keyref_load_by_id() was called with an invalid index
		 * ID.
		 */
		return "SX_ERR_INVALID_KEYREF";
	case SX_ERR_ALLOCATION_TOO_SMALL:
		/* Fatal error, trying to create instance with not enough memory
		 */
		return "SX_ERR_ALLOCATION_TOO_SMALL";
	case SX_ERR_TOO_BIG:
		/* Input or output buffer size too large */
		return "SX_ERR_TOO_BIG";
	case SX_ERR_INPUT_BUFFER_TOO_SMALL:
		/* Input buffer size too small */
		return "SX_ERR_INPUT_BUFFER_TOO_SMALL";
	case SX_ERR_OUTPUT_BUFFER_TOO_SMALL:
		/* Output buffer size too small */
		return "SX_ERR_OUTPUT_BUFFER_TOO_SMALL";
	case SX_ERR_INVALID_KEY_SZ:
		/* The given key size is not supported by the algorithm or the
		 * hardware
		 */
		return "SX_ERR_INVALID_KEY_SZ";
	case SX_ERR_INVALID_TAG_SIZE:
		/* Input tag size is invalid */
		return "SX_ERR_INVALID_TAG_SIZE";
	case SX_ERR_INVALID_NONCE_SIZE:
		/* Input nonce size is invalid */
		return "SX_ERR_INVALID_NONCE_SIZE";
	case SX_ERR_FEED_COUNT_EXCEEDED:
		/* Too many feeds were inputed */
		return "SX_ERR_FEED_COUNT_EXCEEDED";
	case SX_ERR_WRONG_SIZE_GRANULARITY:
		/* Input data size granularity is incorrect */
		return "SX_ERR_WRONG_SIZE_GRANULARITY";
	case SX_ERR_HW_KEY_NOT_SUPPORTED:
		/* Attempt to use HW keys with a mode that does not support HW
		 * keys
		 */
		return "SX_ERR_HW_KEY_NOT_SUPPORTED";
	case SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED:
		/* Attempt to use a mode or engine that does not support context
		 * saving
		 */
		return "SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED";
	case SX_ERR_FEED_AFTER_DATA:
		/* Attempt to feed AAD after input data was fed */
		return "SX_ERR_FEED_AFTER_DATA";
	case SX_ERR_RESET_NEEDED:
		/**
		 * Hardware cannot work anymore.
		 *
		 * To recover, reset the hardware or call *_init() function.
		 * For example, if this error is received when sx_trng_get()
		 * is called, call again sx_trng_open() to reset the hardware.
		 */
		return "SX_ERR_RESET_NEEDED";
	/* End of error codes from sxsymcrypt */
	/* Error codes from silexpk */
	case SX_ERR_INVALID_PARAM:
		/* The function or operation was given an invalid parameter */
		return "SX_ERR_INVALID_PARAM";
	case SX_ERR_UNKNOWN_ERROR:
		/* Unknown error */
		return "SX_ERR_UNKNOWN_ERROR";
	case SX_ERR_BUSY:
		/* The operation is still executing */
		return "SX_ERR_BUSY";
	case SX_ERR_NOT_QUADRATIC_RESIDUE:
		/* The input operand is not a quadratic residue */
		return "SX_ERR_NOT_QUADRATIC_RESIDUE";
	case SX_ERR_COMPOSITE_VALUE:
		/* The input value for Rabin-Miller test is a composite value */
		return "SX_ERR_COMPOSITE_VALUE";
	case SX_ERR_NOT_INVERTIBLE:
		/* Inversion of non-invertible value */
		return "SX_ERR_NOT_INVERTIBLE";
	case SX_ERR_INVALID_SIGNATURE:
		/**
		 * The signature is not valid
		 *
		 * This error can happen during signature generation
		 * and signature verification
		 */
		return "SX_ERR_INVALID_SIGNATURE";
	case SX_ERR_NOT_IMPLEMENTED:
		/* The functionality or operation is not supported */
		return "SX_ERR_NOT_IMPLEMENTED";
	case SX_ERR_POINT_AT_INFINITY:
		/* The output operand is a point at infinity */
		return "SX_ERR_POINT_AT_INFINITY";
	case SX_ERR_OUT_OF_RANGE:
		/* The input value is outside the expected range */
		return "SX_ERR_OUT_OF_RANGE";
	case SX_ERR_INVALID_MODULUS:
		/**
		 * The modulus has an unexpected value
		 *
		 * This error happens when the modulus is zero or
		 * even when odd modulus is expected
		 */
		return "SX_ERR_INVALID_MODULUS";
	case SX_ERR_POINT_NOT_ON_CURVE:
		/* The input point is not on the defined elliptic curve */
		return "SX_ERR_POINT_NOT_ON_CURVE";
	case SX_ERR_OPERAND_TOO_LARGE:
		/**
		 * The input operand is too large
		 *
		 * Happens when an operand is bigger than one of the following:
		 * - The hardware operand size: Check ::sx_pk_capabilities
		 * to see the hardware operand size limit.
		 * - The operation size: Check sx_pk_get_opsize() to get
		 * the operation size limit.
		 */
		return "SX_ERR_OPERAND_TOO_LARGE";
	case SX_ERR_PLATFORM_ERROR:
		/* A platform specific error */
		return "SX_ERR_PLATFORM_ERROR";
	case SX_ERR_EXPIRED:
		/* The evaluation period for the product expired */
		return "SX_ERR_EXPIRED";
	case SX_ERR_IK_MODE:
		/**
		 * The hardware is still in IK mode
		 *
		 * This error happens when a normal operation
		 * is started and the hardware is still in IK mode.
		 * Run command ::SX_PK_CMD_IK_EXIT to exit the IK
		 * mode and to run normal operations again
		 */
		return "SX_ERR_IK_MODE";
	case SX_ERR_INVALID_CURVE_PARAM:
		/* The parameters of the elliptic curve are not valid. */
		return "SX_ERR_INVALID_CURVE_PARAM";
	case SX_ERR_IK_NOT_READY:
		/**
		 * The IK module is not ready
		 *
		 * This error happens when a IK operation is started
		 * but the IK module has not yet generated the internal keys.
		 * Call sx_pk_ik_derive_keys() before starting IK operations.
		 */
		return "SX_ERR_IK_NOT_READY";
	case SX_ERR_PK_RETRY:
		/* Resources not available for a new operation. Retry later */
		return "SX_ERR_PK_RETRY";
	/* End of error codes from silexpk */
	/* Error codes from sicrypto */
	case SX_ERR_INVALID_REQ_SZ:
		/** The size passed as part of a request is not valid */
		return "SX_ERR_INVALID_REQ_SZ";
	case SX_ERR_RESEED_NEEDED:
		/** A PRNG reseed is required */
		return "SX_ERR_RESEED_NEEDED";
	case SX_ERR_INVALID_ARG:
		/** An invalid parameter was passed to a function */
		return "SX_ERR_INVALID_ARG";
	case SX_ERR_READY:
		/** The task is ready for consume, produce, run or partial run
		 */
		return "SX_ERR_READY";
	case SX_ERR_UNSUPPORTED_HASH_ALG:
		/** Hash algorithm not supported */
		return "SX_ERR_UNSUPPORTED_HASH_ALG";
	case SX_ERR_INVALID_CIPHERTEXT:
		/** The ciphertext is not valid */
		return "SX_ERR_INVALID_CIPHERTEXT";
	case SX_ERR_TOO_MANY_ATTEMPTS:
		/** The number of attempts in an algorithm has exceeded a given
		 * threshold
		 */
		return "SX_ERR_TOO_MANY_ATTEMPTS";
	case SX_ERR_RSA_PQ_RANGE_CHECK_FAIL:
		/** Range checks, performed on p and/or q during RSA key
		 * generation, failed
		 */
		return "SX_ERR_RSA_PQ_RANGE_CHECK_FAIL";
	case SX_ERR_WORKMEM_BUFFER_TOO_SMALL:
		/** Task needs bigger workmem buffer than what provided with
		 * si_task_init()
		 */
		return "SX_ERR_WORKMEM_BUFFER_TOO_SMALL";
	/* End of error codes from sicrypto */
	default:
		return "(UNKNONN)";
	}
}

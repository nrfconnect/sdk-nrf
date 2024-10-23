/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_ERR_H__
#define SUIT_PLAT_ERR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* This file contains the definitions needed for error codes handling
 * by the SUIT platform.
 *
 * Rules for error codes in the SUIT platform:
 *
 * The rules are split into two categories: Rules for the SUIT orchestrator and
 * rules for the SUIT platform.
 *
 * Rules for the SUIT orchestrator:
 * 1) The orchestrator functions return an int error code, taken from the error code
 *    pool defined by zephyr (in errno.h) as negative error codes.
 * 2) The orchestrator converts any error code returned by a function from a SUIT
 *    platform/SUIT processor to a Zephyr error code.
 * 3) Before the conversion of a SUIT error code to a Zephyr error code, the
 *    SUIT error code must be logged so that the information about the error is not
 *    lost.
 *
 * Rules for the SUIT platform:
 * 1) The platform uses a set of common error codes as well as module error codes.
 *    The common error codes are integer lesser or equal to 0, the module error codes
 *    are positive integers. Additionally, if a common error has an equivalent
 *    in the zephyr errno.h then if possible the value of the error should be equal to the negative
 *    value of the corresponding error in errno.h if possible to make integration easier.
 *    For example SUIT_PLAT_ERR_INVAL should have the value of -EINVAL.
 * 2) Values of error codes which do not have their equivalent in errno.h should start with -2000
 *    (__ELASTERROR)
 * 3) The suit_plat_err.h file defines the pool of common error codes, which must be understood
 *    by all modules in the SUIT platform.
 * 4) Any function that is called by the SUIT processor MUST convert the platform error codes
 *    to error codes which are defined by the SUIT processor in suit_types.h. All other public
 *    functions defined in the platform directory should also do this. They can use
 *    the suit_plat_err_to_processor_err_convert function to do so, however they can also choose
 *    to convert the error differently.
 * 5) Specific modules can extend the error codes pool. Error codes for a module must be positive
 *    integers to avoid collisions with common error code pool values.
 *    Note that error codes for modules might overlap.
 * 6) If a module extends the error code pool it should define a type <module_name>_err_t which
 *    is resolved to int and should use it as a return type. This does not have a functional
 *    value, however it indicates the meaning of the error codes.
 * 7) A module dependent on a module which extends the common error code pool must understand
 *    the error codes of all its direct dependencies.
 * 8) The public functions of a module must convert all of the error codes coming from its
 *    dependencies either to errors from its own error code pool or to errors from the common
 *    platform error pool.
 *    In other words, there is no "error code inheritance" from lower level modues.
 * 9) If a dependent does not use the error codes of its dependency as its return values
 *    it must convert them. There is no single conversion function, as the resulting error
 *    code of the dependent might differ depending on the context. However helper macros
 *    such as SUIT_RETURN_IF_SUIT_PLAT_ERR_CODE which have usage for many cases can be used.
 * 10) Modules might define their own, internal error code pools not exported outside of the
 *     module. These error codes must be positive integers to avoid collisions with common error
 *     code pool values.
 * 11) It is recommended to use the error codes from the common error code pool unless it is
 *     really necessary - the main reason for adding module specific error codes should be
 *     the need of a higher layer module to control its flow based on lower level module
 *     error codes, NOT the need of precise logging/debugging.
 */

typedef int suit_plat_err_t;

/** Common error codes for the SUIT platform.
 *
 * This error code list should contain generic errors.
 * It is recommended to avoid extending it unless really necessary.
 */
#define SUIT_PLAT_SUCCESS    0
#define SUIT_PLAT_ERR_IO     -5	 /**< I/O error */
#define SUIT_PLAT_ERR_NOMEM  -12 /**< Not enough space */
#define SUIT_PLAT_ERR_ACCESS -13 /**< Permission denied */
#define SUIT_PLAT_ERR_BUSY   -16 /**< Resource is busy */
#define SUIT_PLAT_ERR_EXISTS -17 /**< Element already exists */
#define SUIT_PLAT_ERR_INVAL  -22 /**< Invalid parameter value */
#define SUIT_PLAT_ERR_TIME   -62 /**< Timeout */

#define SUIT_PLAT_ERR_CRASH	       -2001 /**< Execution crashed */
#define SUIT_PLAT_ERR_SIZE	       -2002 /**< Invalid parameter size */
#define SUIT_PLAT_ERR_OUT_OF_BOUNDS    -2003 /**< Out of bounds */
#define SUIT_PLAT_ERR_NOT_FOUND	       -2004 /**< Entity not found */
#define SUIT_PLAT_ERR_INCORRECT_STATE  -2005 /**< Incorrect state to perform the operation */
#define SUIT_PLAT_ERR_HW_NOT_READY     -2006 /**< Hardware is not ready */
#define SUIT_PLAT_ERR_AUTHENTICATION   -2007 /**< Authentication failed */
#define SUIT_PLAT_ERR_UNREACHABLE_PATH -2008 /**< Firmware executed an unreachable path */
#define SUIT_PLAT_ERR_CBOR_DECODING    -2009 /**< CBOR string decoding error */
#define SUIT_PLAT_ERR_UNSUPPORTED      -2010 /**< Attempt to perform an unsupported operation */
#define SUIT_PLAT_ERR_IPC	       -2011 /**< IPC error */
#define SUIT_PLAT_ERR_NO_RESOURCES     -2012 /**< Not enough resources */

/**
 * If the error code is a common platform error code return it.
 */
#define SUIT_RETURN_IF_SUIT_PLAT_COMMON_ERR_CODE(err)                                              \
	if ((err) < 0) {                                                                           \
		return err;                                                                        \
	}

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_ERR_H__ */

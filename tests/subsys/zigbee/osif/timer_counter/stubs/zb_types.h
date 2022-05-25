/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZB_TYPES_H
#define ZB_TYPES_H 1


/** @brief General purpose boolean type. */
enum zb_bool_e {
	ZB_FALSE = 0, /**< False value literal. */
	ZB_TRUE = 1   /**<  True value literal. */
};

typedef char               zb_char_t;

typedef unsigned char      zb_uchar_t;

typedef unsigned char      zb_uint8_t;

typedef signed char        zb_int8_t;

typedef unsigned short     zb_uint16_t;

typedef signed short       zb_int16_t;


/** @brief General purpose boolean type. */
typedef enum zb_bool_e zb_bool_t;
typedef unsigned int       zb_uint32_t;
typedef int                zb_int32_t;
typedef unsigned long long zb_uint64_t;

typedef zb_uint32_t zb_time_t;


/*
 * 8-bytes address (xpanid or long device address) base type
 */
typedef zb_uint8_t zb_64bit_addr_t[8];

/*
 * Long (64-bit) device address
 */
typedef zb_64bit_addr_t zb_ieee_addr_t;

/** @brief Return type for ZB functions returning execution status. @see ::RET_OK. */
typedef zb_int32_t zb_ret_t;

#endif /* ! defined ZB_TYPES_H */

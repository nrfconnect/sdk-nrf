/*
 * Copyright (c) 2015 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file coap_block.h
 *
 * @defgroup iot_sdk_coap_block CoAP Block transfer
 * @ingroup iot_sdk_coap
 * @{
 * @brief CoAP block transfer options encoding and decoding interface and
 *        definitions.
 */

#ifndef COAP_BLOCK_H__
#define COAP_BLOCK_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Value when more flag is set. */
#define COAP_BLOCK_OPT_BLOCK_MORE_BIT_UNSET 0

/** Value when more flag is not set. */
#define COAP_BLOCK_OPT_BLOCK_MORE_BIT_SET 1

typedef struct {
	u8_t more;    /**< More bit value. */
	u16_t size;   /**< Size of the block in bytes. */
	u32_t number; /**< Block number. */
} coap_block_opt_block1_t;

typedef coap_block_opt_block1_t coap_block_opt_block2_t;

/**@brief Encode block1 option into its uint binary counterpart.
 *
 * @param[out] encoded Encoded version of the CoAP block1 option value.
 *                     Must not be NULL.
 * @param[in]  opt     Pointer to block1 option structure to be decoded into
 *                     uint format. Must not be NULL.
 *
 * @retval 0      If encoding of option was successful.
 * @retval EINVAL If one of the parameters supplied is a null pointer or one of
 *                the fields in the option structure has an illegal value.
 */
u32_t coap_block_opt_block1_encode(u32_t *encoded,
				   coap_block_opt_block1_t *opt);

/**@brief Decode block1 option from a uint to its structure counterpart.
 *
 * @param[out] opt     Pointer to block1 option structure to be filled by the
 *                     function. Must not be NULL.
 * @param[in]  encoded Encoded version of the CoAP block1 option value.
 *
 * @retval 0      If decoding of the option was successful.
 * @retval EINVAL If opt parameter is NULL or the block number is higher then
 *                allowed by spec (more than 20 bits) or the size has the value
 *                of the reserved 2048 value (7).
 */
u32_t coap_block_opt_block1_decode(coap_block_opt_block1_t *opt, u32_t encoded);

/**@brief Encode block2 option into its uint binary counterpart.
 *
 * @param[out] encoded Encoded version of the CoAP block2 option value.
 *                     Must not be NULL.
 * @param[in]  opt     Pointer to block2 option structure to be decoded into
 *                     uint format. Must not be NULL.
 *
 * @retval 0      If encoding of option was successful.
 * @retval EINVAL If one of the parameters supplied is a null pointer or one of
 *                the fields in the option structure has an illegal value.
 */
u32_t coap_block_opt_block2_encode(u32_t *encoded,
				   coap_block_opt_block2_t *opt);

/**@brief Decode block2 option from a uint to its structure counterpart.
 *
 * @param[out] opt     Pointer to block2 option structure to be filled by the
 *                     function. Must not be NULL.
 * @param[in]  encoded Encoded version of the CoAP block2 option value.
 *
 * @retval 0      If decoding of the option was successful.
 * @retval EINVAL If opt parameter is NULL or the block number is higher then
 *                allowed by spec (more than 20 bits) or the size has the value
 *                of the reserved 2048 value (7).
 */
u32_t coap_block_opt_block2_decode(coap_block_opt_block2_t *opt, u32_t encoded);

#endif /* COAP_BLOCK_H__ */

/** @} */

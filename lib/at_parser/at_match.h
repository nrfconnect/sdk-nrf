/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_MATCH_H__
#define AT_MATCH_H__

#include "at_token.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file at_match.h
 *
 * @defgroup at_match Internal AT parser matching functions.
 * @ingroup at_parser
 * @{
 * @brief Internal AT parser functions for matching AT tokens in a given AT command string.
 */

/**
 * @brief Match an AT command prefix.
 *
 * This function returns an AT token representing an AT command prefix for AT command set, read,
 * test, events, or notifications.
 * In case of success, this function updates @p remainder to point to the remaining string after
 * trimming the matched AT token.
 *
 * @param[in]  at        AT command string to match.
 * @param[out] remainder Remainder of the AT command string after a successful match.
 *
 * @return Matched AT token in case of success.
 *         AT token of type @ref AT_TOKEN_TYPE_INVALID in case of failure,
 */
struct at_token at_match_cmd(const char *at, const char **remainder);

/**
 * @brief Match a subparameter.
 *
 * This function returns an AT token representing a subparameter.
 * In case of success, this function updates @p remainder to point to the remaining string after
 * trimming the matched AT token.
 *
 * @param[in]  at        AT command string to match.
 * @param[out] remainder Remainder of the AT command string after a successful match.
 *
 * @return Matched AT token in case of success.
 *         AT token of type @ref AT_TOKEN_TYPE_INVALID in case of failure,
 */
struct at_token at_match_subparam(const char *at, const char **remainder);

/**
 * @brief Match a singleline string subparameter.
 *
 * This function returns an AT token representing a singleline string subparameter.
 * In case of success, this function updates @p remainder to point to the remaining string after
 * trimming the matched AT token.
 *
 * @param[in]  at        AT command string to match.
 * @param[out] remainder Remainder of the AT command string after a successful match.
 *
 * @return Matched AT token in case of success.
 *         AT token of type @ref AT_TOKEN_TYPE_INVALID in case of failure,
 */
struct at_token at_match_str(const char *at, const char **remainder);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_MATCH_H__ */

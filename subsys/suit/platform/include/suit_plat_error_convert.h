/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_ERROR_CONVERT_H__
#define SUIT_PLAT_ERROR_CONVERT_H__

#include <stdint.h>
#include <suit_types.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Convert an error code used by the SUIT platform to an error used by
 *  the SUIT processor.
 *
 *  @param  plat_err A SUIT platform error code. This may be also an error
 *                   code from a SUIT module extending the common platform error
 *                   code pool. In this case it will be converted to SUIT_ERR_CRASH.
 */
int suit_plat_err_to_processor_err_convert(suit_plat_err_t plat_err);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_ERROR_CONVERT_H__ */

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_H__
#define LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_H__

typedef void (*ground_fix_get_result_code_cb_t)(int32_t result_code);
void ground_fix_set_result_code_cb(ground_fix_get_result_code_cb_t cb);



/* DOXYGEN_TEST_INJECT - fake content issues for reviewer testing, do not merge */
#define DOXY_PR_TEST_GROUND_FIX_OBJ_B52875 1
#endif

/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UTIL_HEADER_FILE
#define UTIL_HEADER_FILE

#include <sicrypto/util.h>

int si_status_on_prng(struct sitask *t);

int si_wait_on_prng(struct sitask *t);

int si_silexpk_status(struct sitask *t);

int si_silexpk_wait(struct sitask *t);

#endif

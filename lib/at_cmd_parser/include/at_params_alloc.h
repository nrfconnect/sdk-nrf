/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef AT_PARAMS_ALLOC_H_
#define AT_PARAMS_ALLOC_H_

#include <zephyr.h>

/*
 * FIXME: we need a new header file with the function we want to mock.
 * This function cannot be part of the at_params.h header file because
 * we do not want to mock all at_params APIs, but only this function.
 */

/* Proxy function available only in this module that we can mock. */
static ALWAYS_INLINE void *at_params_calloc(size_t nmemb, size_t size)
{
	return k_calloc(nmemb, size);
}

#endif /* AT_PARAMS_ALLOC_H_ */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cJSON_os.h"
#include "cJSON.h"
#include <stdint.h>
#include <zephyr/kernel.h>

static cJSON_Hooks _cjson_hooks;

/**@brief malloc() function definition. */
static void *malloc_fn_hook(size_t sz) { return k_malloc(sz); }

/**@brief free() function definition. */
static void free_fn_hook(void *p_ptr) { k_free(p_ptr); }

/**@brief Initialize cJSON by assigning function hooks. */
void cJSON_Init(void)
{
	_cjson_hooks.malloc_fn = malloc_fn_hook;
	_cjson_hooks.free_fn = free_fn_hook;

	cJSON_InitHooks(&_cjson_hooks);
}

void cJSON_FreeString(char *ptr)
{
	free_fn_hook(ptr);
}

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/app_memory/app_memdomain.h>

#include "mbedtls/memory_buffer_alloc.h"

#if !defined(CONFIG_MBEDTLS_HEAP_SIZE) || CONFIG_MBEDTLS_HEAP_SIZE == 0
#error "CONFIG_MBEDTLS_HEAP_SIZE must be specified and greater than 0"
#endif

static unsigned char mbedtls_heap[CONFIG_MBEDTLS_HEAP_SIZE];

/*
 * Initializes the heap with the compile-time configured size.
 *
 * Not static in order to allow extern use.
 */
void _heap_init(void)
{
	mbedtls_memory_buffer_alloc_init(mbedtls_heap, sizeof(mbedtls_heap));
}

/*
 * Internal use. Calling this followed by _heap_init allows re-allocating the
 * heap, which is useful when testing small devices with small heaps,
 * where fragmentation might render the heap unusable.
 *
 * Not static in order to allow extern use.
 */
void _heap_free(void)
{
	mbedtls_memory_buffer_alloc_free();
}

static int mbedtls_heap_init(void)
{
	_heap_init();

	return 0;
}

/* Hw cc310 is initialized with CONFIG_KERNEL_INIT_PRIORITY_DEFAULT and the
 * heap must be initialized afterwards.
 */
SYS_INIT(mbedtls_heap_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

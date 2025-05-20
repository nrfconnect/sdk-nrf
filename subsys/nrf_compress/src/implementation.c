/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <nrf_compress/implementation.h>

#if !defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_STATIC) && \
	!defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC)
#error "Missing compression static buffer configuration, please select " \
	"CONFIG_NRF_COMPRESS_MEMORY_TYPE_STATIC or CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC"
#endif

#if !defined(CONFIG_NRF_COMPRESS_COMPRESSION) && !defined(CONFIG_NRF_COMPRESS_DECOMPRESSION)
#error "Compression library enabled but without compression or decompression support enabled, " \
	"please select CONFIG_NRF_COMPRESS_COMPRESSION and/or CONFIG_NRF_COMPRESS_DECOMPRESSION"
#endif

#if !defined(CONFIG_NRF_COMPRESS_TYPE_SELECTED)
#error "An implmentation must be selected to use the nRF compression library"
#endif

struct nrf_compress_implementation *nrf_compress_implementation_find(uint16_t id)
{
	STRUCT_SECTION_FOREACH(nrf_compress_implementation, implementation) {
		if (implementation->id == id) {
			return implementation;
		}
	}

	return NULL;
}

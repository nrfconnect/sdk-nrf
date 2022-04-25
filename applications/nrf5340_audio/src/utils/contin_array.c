/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "contin_array.h"

#include <zephyr.h>
#include <stdio.h>
#include <string.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(contin_array, LOG_LEVEL_WRN);

int contin_array_create(void *const pcm_cont, uint32_t pcm_cont_size, void const *const pcm_finite,
			uint32_t pcm_finite_size, uint32_t *const finite_pos)
{
	LOG_DBG("pcm_cont_size: %d pcm_finite_size %d", pcm_cont_size, pcm_finite_size);

	if (pcm_cont == NULL || pcm_finite == NULL) {
		return -ENXIO;
	}

	if (!pcm_cont_size || !pcm_finite_size) {
		LOG_ERR("size cannot be zero");
		return -EPERM;
	}

	for (uint32_t i = 0; i < pcm_cont_size; i++) {
		if (*finite_pos > (pcm_finite_size - 1)) {
			*finite_pos = 0;
		}
		((char *)pcm_cont)[i] = ((char *)pcm_finite)[*finite_pos];
		(*finite_pos)++;
	}

	return 0;
}

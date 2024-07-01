/*
 * Copyright(c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _AUDIO_MODULE_TEMPLATE_H_
#define _AUDIO_MODULE_TEMPLATE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "audio_defines.h"
#include "audio_module.h"

/* @brief Size of the data block within the template context, for testing. */
#define AUDIO_MODULE_TEMPLATE_DATA_BYTES (10)

/**
 * @brief Private pointer to the module's parameters.
 *
 */
extern struct audio_module_description *audio_module_template_description;

/**
 * @brief The module configuration structure.
 *
 */
struct audio_module_template_configuration {
	/* The sample rate, for testing. */
	uint32_t sample_rate_hz;

	/* The bit depth, for testing. */
	uint8_t bit_depth;

	/* A string for testing.*/
	char *module_description;
};

/**
 * @brief  Private module context.
 *
 */
struct audio_module_template_context {
	/* Array of data to illustrate the data process function. */
	uint8_t audio_module_template_data[AUDIO_MODULE_TEMPLATE_DATA_BYTES];

	/* The template configuration. */
	struct audio_module_template_configuration config;
};

#endif /* _AUDIO_MODULE_TEMPLATE_H_ */

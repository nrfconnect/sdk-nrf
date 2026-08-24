/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "rng_test_logic.h"

#define RANDOM_SIZE 100

static uint8_t random_data[RANDOM_SIZE];

static psa_status_t generate(const void *context)
{
	return psa_generate_random(random_data, sizeof(random_data));
}

int rng_check(void)
{
        for (size_t i = 0; i < sizeof(random_data); i++) {
                if (random_data[i] != 0) {
                        return APP_SUCCESS;
                }
        }

        return APP_ERROR;
}

const struct op rng_operations[] = {{"generate", generate, NULL}};

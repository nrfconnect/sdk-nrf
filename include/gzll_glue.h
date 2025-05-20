/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __GZLL_GLUE_H
#define __GZLL_GLUE_H

/**
 * @file
 * @brief Gazell Link Layer glue
 *
 * File defines a set of functions and variables called by Gazell Link Layer.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup gzll_glue Gazell Link Layer glue
 *
 * File defines a set of functions and variables called by Gazell Link Layer.
 *
 * @{
 */

/**
 * @brief Gazell Link Layer glue initialization.
 *
 * @retval true  if initialization is successful.
 * @retval false if initialization is unsuccessful.
 */
bool gzll_glue_init(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __GZLL_GLUE_H */

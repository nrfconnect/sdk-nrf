/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_DRIVERS_NPMX_NPMX_GLUE_H__
#define ZEPHYR_DRIVERS_NPMX_NPMX_GLUE_H__

#include <npmx.h>
#include <zephyr/sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup npmx_glue npmx_glue.h
 * @{
 * @ingroup npmx
 *
 * @brief This file contains functions declarations and macros that should be implemented
 *        according to the needs of the host environment into which @em npmx is integrated.
 */

/**
 * @brief Macro for placing a runtime assertion.
 *
 * @param expression Expression to be evaluated.
 */
#define NPMX_ASSERT(expression) __ASSERT_NO_MSG(expression)

/**
 * @brief Macro for placing a compile time assertion.
 *
 * @param expression Expression to be evaluated.
 */
#define NPMX_STATIC_ASSERT(expression) BUILD_ASSERT(expression)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_NPMX_NPMX_GLUE_H__ */

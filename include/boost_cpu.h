/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief CPU boost clock header.
 */

#ifndef CPU_BOOST_H__
#define CPU_BOOST_H__

/**
 * @defgroup boost_cpu CPU clock boost
 * @{
 * @brief Module for boosting up CPU clock frequency.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Request change of CPU clock frequency.
 *
 * @param enable flag enabling/disabling CPU clock frequency boost.
 */
void cpu_boost(bool enable);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* CPU_BOOST_H__ */

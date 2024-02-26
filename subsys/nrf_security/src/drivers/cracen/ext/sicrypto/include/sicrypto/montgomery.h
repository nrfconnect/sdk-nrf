/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SI_MONTGOMERY_H
#define SI_MONTGOMERY_H

/**
 * @brief Definitions for X25519. Only public key generation available.
 */
extern const struct si_sig_def *const si_sig_def_x25519;

/**
 * @brief Definitions for X448. Only public key generation available.
 */
extern const struct si_sig_def *const si_sig_def_x448;

#endif

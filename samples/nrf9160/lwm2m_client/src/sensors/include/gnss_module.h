/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GNSS_MODULE_H__
#define GNSS_MODULE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise GNSS interface.
 *
 * @return int 0 if successful, negative error code if not.
 */
int initialise_gnss(void);

/**
 * @brief Start GNSS.
 *
 * @return int 0 if successful, negative error code if not.
 */
int start_gnss(void);

#ifdef __cplusplus
}
#endif

#endif /* GNSS_MODULE_H__ */

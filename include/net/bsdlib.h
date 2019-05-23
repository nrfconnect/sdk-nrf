/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @brief Initialize bsdlib.
 *
 * This function is synchrounous and it could block
 * for a few minutes when the modem firmware is being updated.
 *
 * If you application supports modem firmware updates, consider
 * initializing the library manually to have control of what
 * the application should do while initialization is ongoing.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int bsdlib_init(void);

/**
 * @brief Shutdown bsdlib, releasing its resources.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int bsdlib_shutdown(void);

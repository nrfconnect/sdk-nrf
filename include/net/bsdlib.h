/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief Get the last return value of bsdlib_init.
 *
 * This function can be used to access the last return value of
 * bsdlib_init. This can be used to check the state of a modem
 * firmware exchange when bsdlib was initialized at boot-time.
 *
 * @return int The last return value of bsdlib_init.
 */
int bsdlib_get_init_ret(void);

/**
 * @brief Shutdown bsdlib, releasing its resources.
 *
 * @return int Zero on success, non-zero otherwise.
 */
int bsdlib_shutdown(void);

#ifdef __cplusplus
}
#endif

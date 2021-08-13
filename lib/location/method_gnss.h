/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_GNSS_H
#define METHOD_GNSS_H

int method_gnss_init(void);
void method_gnss_event_handler(int event);
void method_gnss_fix_work_fn(struct k_work *item);
void method_gnss_timeout_work_fn(struct k_work *item);

/** @brief Sets the GNSS configuration and starts GNSS.
 *
 * @details Currently supported configurations are fix interval, fix timeout and fix accuracy.
 *
 * @param[in] config   Requested gnss configuration.
 * @param[in] interval Requested fix interval.
 *
 * @retval 0 on success.
 * @retval NRF_EPERM if GNSS is running or not initialized.
 * @retval NRF_EINVAL if GNSS returned an error.
 * @retval NRF_EAGAIN if out of memory.
 */
int method_gnss_configure_and_start(const struct loc_method_config *config, uint16_t interval);

int method_gnss_cancel(void);

#endif /* METHOD_GNSS_H */

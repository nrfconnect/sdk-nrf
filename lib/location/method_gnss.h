/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_GNSS_H
#define METHOD_GNSS_H

void gnss_event_handler(int event);
void gnss_fix_work_fn(struct k_work *item);
void gnss_timeout_work_fn(struct k_work *item);

/** @brief Sets the GNSS configuration and starts GNSS.
 *
 * @details Single fix navigation mode is engaged by setting the fix interval to 0.
 *
 * Continuous navigation mode is not supported at the moment.
 *
 * Periodic navigation mode is engaged by setting the fix interval to value 10...65535. The unit is
 * seconds.
 *
 * @param[in] gnss_config pointer to requested gnss configuration.
 * @param[in] interval    requested fix interval.
 *
 * @retval 0 on success.
 * @retval NRF_EPERM if GNSS is running or not initialized.
 * @retval NRF_EINVAL if GNSS returned an error.
 * @retval NRF_EAGAIN if out of memory.
 */
int gnss_configure_and_start(struct loc_gnss_config *gnss_config, uint16_t interval);

#endif /* METHOD_GNSS_H */

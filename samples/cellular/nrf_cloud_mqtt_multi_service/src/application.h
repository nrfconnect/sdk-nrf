/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

/**
 * @brief Main application -- Wait for valid connection, start location tracking, and then
 * periodically sample sensors and send them to nRF Cloud.
 */
void main_application_thread_fn(void);

#endif /* _APPLICATION_H_ */

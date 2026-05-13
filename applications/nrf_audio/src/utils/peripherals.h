/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Board initialization function
 *
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

/**
 * @brief	Initialize the hardware related modules on the nRF board.
 *
 * @return	0 if successful, error otherwise.
 */
int peripherals_init(void);

#endif /* _PERIPHERALS_H_ */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

<<<<<<< HEAD
/** @file
 *  @brief nRF5340 Audio DK specific functions
 *
 * Used to access and validate the hardware being used.
 */

#ifndef _NRF5340_BOARD_VERSION_H_
#define _NRF5340_BOARD_VERSION_H_

=======
#ifndef _NRF5340_BOARD_VERSION_H_
#define _NRF5340_BOARD_VERSION_H_

>>>>>>> 1fbc5f5ef0 (applications: nrf5340_audio: LED module for kits with other LED setup)
#include "nrf5340_dk.h"

/**@brief Get the board/HW version
 *
 * @note  This function will init the ADC, perform a reading, and
 *	  return the HW version.
 *
 * @param board_rev	Pointer to container for board version
 *
 * @return 0 on success.
 * Error code on fault or -ESPIPE if no valid version found
 */
int nrf5340_board_version_get(struct board_version *board_rev);

/**@brief Check that the FW is compatible with the HW version
 *
 * @note  This function will init the ADC, perform a reading, and
 * check for valid version match.
 *
 * @note The board file must define a BOARD_VERSION_ARR array of
 * possible valid ADC register values (voltages) for the divider.
 * A BOARD_VERSION_VALID_MSK with valid version bits must also be defined.
 *
 * @return 0 on success. Error code on fault or -EPERM if incompatible board version.
 */
int nrf5340_board_version_valid_check(void);

#endif /* _NRF5340_BOARD_VERSION_H_ */

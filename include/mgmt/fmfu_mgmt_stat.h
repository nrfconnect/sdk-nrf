/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * The RST file for this library can be found in doc/nrf/libraries/dfu/fmfu_mgmt.rst.
 * Rendered documentation is available at
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/dfu/fmfu_mgmt.html.
 */

/** @file fmfu_mgmt_stat.h
 * @defgroup fmfu_mgmt_stat MCUMgr hooks for Full Modem Firmware Upgrade stats
 * @{
 * @brief Full Modem Firmware Update(FMFU) statistics for MCUMgr over SMP.
 *
 * This registers a handler for the MCUMgr stat command to report back the
 * SMP protocol MTU and frame size.
 *
 */

#ifndef MGMT_FMFU_STAT_H__
#define MGMT_FMFU_STAT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Setup and register the command handlers responsible for communicating
 *	   SMP buffer sizes.
 *
 *  This functions setups the stat group for mcumgr so that the serial modem
 *  module update script can negotiate the SMP protocol frame size.
 *
 *  @retval 0 on success, negative integer on failure.
 */
int fmfu_mgmt_stat_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* MGMT_FMFU_STAT_H__ */

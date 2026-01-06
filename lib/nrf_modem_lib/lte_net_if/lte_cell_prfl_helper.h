/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */



/* @brief Configure cellular profiles for Terrestrial Network and Non-Terrestrial Network.
 *
 *  @details Sets up two cellular profiles:
 *           - TN profile (id=0) for LTE-M/NB-IoT
 *           - NTN profile (id=1) for satellite connectivity
 *	     Note: modes +CFUN=45 and +CFUN=46 allowed only when two cellular profiles exists.
 *
 *  @retval 0 on success.
 *  @retval Negative errno on failure.
 */
int cell_prfl_set_profiles(void);

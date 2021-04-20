/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PELION_FCC_ERR_
#define PELION_FCC_ERR_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert an error value returned by fcc module to string
 *
 * @param fcc_status Error code
 * @return String describing the error
 */
const char *fcc_status_to_string(int fcc_status);

#ifdef __cplusplus
}
#endif

#endif /* PELION_FCC_ERR */

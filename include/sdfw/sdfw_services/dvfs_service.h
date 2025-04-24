/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 #ifndef DVFS_SERVICE_H__
 #define DVFS_SERVICE_H__

 #include <stddef.h>
 #include <stdint.h>

 #include <sdfw/sdfw_services/ssf_errno.h>

 int ssf_dvfs_set_oppoint(uint8_t opp);

 #endif /* DVFS_SERVICE_H__ */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#if defined(TFM_PARTITION_INTERNAL_TRUSTED_STORAGE)
/* The ITS partition is enabled so
 * TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_SID will be defined by
 * psa_manifest/sid.h and we don't need to define it here.
 */
#else
/* The ITS partition is not enabled, but the crypto partition is
 * definitely enabled as it is mandatory.
 *
 * The manifest states that the crypto partition depends on the ITS
 * partition, but this is not true for all configurations of the
 * crypto partition.
 *
 * The crypto partition unfortunately does not build without
 * TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_SID defined, so we define it
 * here to pretend like it's dependency is satisfied and support this
 * configuration.
 */

#define TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_SID 0x00000070

#endif

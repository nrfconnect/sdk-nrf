/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Provisioning handler
 */

#ifndef PROV_HANDLER_H__
#define PROV_HANDLER_H__

#include <bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

const struct bt_mesh_prov *prov_handler_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PROV_HANDLER_H__ */

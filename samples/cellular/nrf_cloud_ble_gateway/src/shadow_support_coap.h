/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SHADOW_SUPPORT_COAP_H
#define SHADOW_SUPPORT_COAP_H

int shadow_support_coap_obj_send(struct nrf_cloud_obj *const shadow_obj, const bool desired);

int coap_shadow_thread_fn(void);

#endif /* SHADOW_SUPPORT_COAP_H */

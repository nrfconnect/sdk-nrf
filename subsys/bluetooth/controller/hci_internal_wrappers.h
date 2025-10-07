/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Internal HCI interface Wrappers
 */

#include <stdint.h>
#include <stdbool.h>
#include <sdc_hci_cmd_controller_baseband.h>

#ifndef HCI_INTERNAL_WRAPPERS_H__
#define HCI_INTERNAL_WRAPPERS_H__

#if defined(__DOXYGEN__) || defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
int sdc_hci_cmd_cb_host_buffer_size_wrapper(const sdc_hci_cmd_cb_host_buffer_size_t *cmd_params);
#endif

#endif

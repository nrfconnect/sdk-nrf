/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_common.h>
#include <ot_rpc_ids.h>

#include <openthread/thread.h>

size_t ot_rpc_get_string(enum ot_rpc_cmd_server cmd, char *buffer, size_t buffer_size);
otError ot_rpc_set_string(enum ot_rpc_cmd_server cmd, const char *data);

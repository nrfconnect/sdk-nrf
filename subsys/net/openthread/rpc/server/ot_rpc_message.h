/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_OT_MESSAGE_
#define OT_RPC_OT_MESSAGE_

#include <openthread/message.h>
#include <ot_rpc_types.h>

ot_msg_key ot_reg_msg_alloc(otMessage *msg);
void ot_msg_free(ot_msg_key key);
otMessage *ot_msg_get(ot_msg_key key);

#endif

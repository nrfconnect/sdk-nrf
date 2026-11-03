/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Internal declarations shared between server RPC source files.
 */

#ifndef DECT_RPC_SERVER_INTERNAL_H_
#define DECT_RPC_SERVER_INTERNAL_H_

void dect_rpc_server_note_client_activity(void);

#if defined(CONFIG_DECT_NR_RPC_SHELL)
void dect_rpc_server_shell_line_l2_init(void);
int dect_rpc_shell_session_try_lock(void);
void dect_rpc_shell_session_unlock(void);
#endif

#endif /* DECT_RPC_SERVER_INTERNAL_H_ */

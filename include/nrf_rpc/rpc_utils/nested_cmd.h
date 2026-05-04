/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_NESTED_CMD_H_
#define NRF_RPC_NESTED_CMD_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup nrf_rpc_utils nRF RPC utility commands
 * @{
 * @defgroup nrf_rpc_nested_cmd nRF RPC nested command test
 * @{
 */

/** @brief Run the nested command context test on the remote server.
 *
 * This sends a command to the server. The server sends a nested command back
 * to this client before sending the response. The function returns true if the
 * nested command was handled while waiting for the server response.
 *
 * @retval true  Nested command was handled.
 * @retval false Nested command was not handled or the test failed.
 */
bool nrf_rpc_utils_nested_cmd_test(void);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_NESTED_CMD_H_ */

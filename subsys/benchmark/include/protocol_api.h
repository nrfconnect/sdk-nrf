/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#ifndef PROTOCOL_API_H__
#define PROTOCOL_API_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <shell/shell.h>

#include "benchmark_api.h"

/**@brief Function for determining if a protocol-specific function returned an error.
 *
 * @param[in]   error_code   Return value from a function to check.
 *
 * @retval      true         If @p error_code is not a success.
 * @retval      false        If @p error_code indicates a success.
 */
bool protocol_is_error(uint32_t error_code);

/**@brief Function for protocol initialization.
 *
 * @note  This function should initialize protocol-specific Board Support Package.
*/
void protocol_init(void);

/**@brief Function for protocol processing. */
void protocol_process(void);

/**@brief Function for protocol sleep. */
void protocol_sleep(void);

/**@brief Function for handling protocol-specific SoC events.
 *
 * @param[in]   soc_evt     SoC stack event.
 */
void protocol_soc_evt_handler(uint32_t soc_evt);

/**@brief   Function that prints protocol-specific information about known peers.
 *
 * @param[in]  shell    Pointer to a shell instance.
 * @param[in]  p_peers  Pointer to a structure describing all known peers.
 */
void protocol_cmd_peer_db_get(const struct shell *shell, const benchmark_peer_db_t * p_peers);

/**@brief   Function that prints protocol-specific information about a peer.
 *
 * @param[in]  shell   Pointer to a shell instance.
 * @param[in]  p_peer  Pointer to a peer information structure.
 */
void protocol_cmd_peer_get(const struct shell *shell, const benchmark_peer_entry_t * p_peer);

/**@brief   Function that prints protocol-specific information about the local node.
 *
 * @param[in]  shell    Pointer to a shell instance.
 */
void protocol_cmd_config_get(const struct shell *shell);

/**@brief   Function that sends a command to a remote peer.
 *
 * @note    This function receives the whole shell input as an argument. The command that ought to be sent
 *          to the specified peer should be parsed from a full shell command.
 *
 * @param[in]  shell       Pointer to a shell instance.
 * @param[in]  p_peer_cli  Pointer to a peer shell instance.
 * @param[in]  argc        Number of words the command contains.
 * @param[in]  argv        Array of pointers to the words the command contains.
 */
void protocol_cmd_remote_send(const struct shell *shell, const nrf_cli_t * p_peer_cli, size_t argc, char ** argv);

#endif // PROTOCOL_API_H__

/**
 * @}
 */

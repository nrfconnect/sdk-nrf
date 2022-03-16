/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#ifndef CLI_H__
#define CLI_H__

#include "benchmark_api.h"

#include <stddef.h>

#define CLI_REMOTE_INSTANCES 5       /**< Maximal number of remote CLI connections. */


/**@brief Function for initializing the CLI.
 *
 * Serial connection and RTT console are supported.
 */
void cli_init(void);

/**@brief Function for initializing the remote CLI.
 */
void cli_remote_init(void);

/**@brief Function for starting the CLI.
 */
void cli_start(void);

/**@brief Function for starting the remote CLI.
 */
void cli_remote_start(void);

/**@brief Function for processing CLI operations.
 */
void cli_process(void);

/**@brief Get remote CLI instance from index.
 */
nrf_cli_t const * cli_remote_get(size_t idx);

/**@brief Set address of a remote peer.
 *
 * @param[in] p_peer_cli      Pointer to a remote peer CLI instance.
 * @param[in] p_peer_address  Pointer to protocol-specific address structure.
 */
void cli_remote_peer_set(nrf_cli_t const * p_peer_cli, benchmark_address_context_t * p_peer_address);

#endif /* CLI_H__ */

/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#ifndef COMMANDS_H__
#define COMMANDS_H__

#include <stdint.h>
#include <zephyr.h>
#include <shell/shell.h>

#define DEVICE_ID_HI(device_id) ((uint32_t)((device_id >> 32) & 0xFFFFFFFF))  /** Get more significat 32-bits out of 64-bit device ID value. */
#define DEVICE_ID_LO(device_id) ((uint32_t)(device_id & 0xFFFFFFFF))          /** Get less significat 32-bits out of 64-bit device ID value. */

/**@brief  Default command handler, prints error message and returns.
 *
 * @param[in]  p_cli  Pointer to a CLI instance.
 * @param[in]  argc   Number of words the unknown subcommand contains.
 * @param[in]  argv   Array of pointers to the words the unknown subcommand gets as arguments.
 */
void cmd_default(const struct shell *shell, size_t argc, char ** argv);

/**@brief  Prints the unified message to indicate that command was successfully executed.
 *
 * @param[in]  p_cli    Pointer to a CLI instance.
 */
void print_done(const struct shell *shell);

/**@brief  Prints the unified message to indicate that command execution has failed.
 *
 * @param[in]  p_cli     Pointer to a CLI instance.
 * @param[in]  p_reason  Pointer to an error message.
 */
void print_error(const struct shell *shell, char * p_reason);

#endif // COMMANDS_H__

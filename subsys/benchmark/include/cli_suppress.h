/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#ifndef _CLI_SUPPRESS_H__
#define _CLI_SUPPRESS_H__

#include <stdbool.h>

/** @brief Enable processing of cli and pending logs. */
void cli_suppress_enable(void);

/** @brief Disable processing of cli and pending logs. */
void cli_suppress_disable(void);

/** 
 * @brief Check if cli and logs should be suppressed.
 * 
 * @return If cli and logs should be suppressed. 
 */
bool cli_suppress_is_enabled(void);

#endif // _CLI_SUPPRESS_H__

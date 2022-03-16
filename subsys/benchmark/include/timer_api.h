/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#ifndef TIMER_API_H
#define TIMER_API_H

#include <stdint.h>

/**@brief Timer timeout callback definition.
 */
typedef void (*timer_cb_t)(void);

/**@brief Function for capturing current time.
 *
 * @returns   Current time.
 */
uint32_t timer_capture(void);

/**@brief Function for initializing the timer.
 */
void timer_init(void);

/**@brief Function for setting the timer.
 *
 * @param[in] timeout_ms  Timeout in milliseconds.
 * @param[in] p_callback  Callback to be called once, after timeout_ms.
 */
void timer_set(uint32_t timeout_ms, timer_cb_t p_callback);

/**@brief Function for starting the timer.
 */
void timer_start(void);

/**@brief Function for stopping the timer.
 */
void timer_stop(void);

/**@brief Function for converting ticks to milliseconds.
 *
 * @param[in] ticks   Number of ticks to convert.
 *
 * @returns   Calculated number of milliseconds.
 */
uint32_t timer_ticks_to_ms(uint32_t ticks);

#endif // TIMER_API_H

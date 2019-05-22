/*$$$LICENCE_NORDIC_STANDARD<2018>$$$*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <misc/printk.h>
// #include "nrf_gpio.h"

#include "common_test.h"


uint32_t unhexify(uint8_t * p_output, char const * p_input)
{
    unsigned char c, c2;

    if (p_input == NULL)
    {
        return 0;
    }

    if (p_output == NULL)
    {
        return 0;
    }

    int len = strlen( p_input ) / 2;

    while( *p_input != 0 )
    {
        c = *p_input++;
        if( c >= '0' && c <= '9' )
        {
            c -= '0';
        }
        else if( c >= 'a' && c <= 'f' )
        {
            c -= 'a' - 10;
        }
        else if( c >= 'A' && c <= 'F' )
        {
            c -= 'A' - 10;
        }
        else
        {
            printk("unhexify ERROR");
        }

        c2 = *p_input++;
        if( c2 >= '0' && c2 <= '9' )
        {
            c2 -= '0';
        }
        else if( c2 >= 'a' && c2 <= 'f' )
        {
            c2 -= 'a' - 10;
        }
        else if( c2 >= 'A' && c2 <= 'F' )
        {
            c2 -= 'A' - 10;
        }
        else
        {
            printk("unhexify ERROR");
        }

        *p_output++ = ( c << 4 ) | c2;
    }

    return len;
}

/**@brief Function for running starting time measurements.
 */
void start_time_measurement()
{
    // nrf_gpio_pin_clear(LED_1);
}

/**@brief Function for running stopping time measurements.
 */
void stop_time_measurement()
{
    // nrf_gpio_pin_set(LED_1);
}

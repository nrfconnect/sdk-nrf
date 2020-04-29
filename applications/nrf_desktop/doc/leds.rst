.. _nrf_desktop_leds:

leds module
###########

You can control LED diodes with the ``leds`` module, which sets the LED effect in response to a ``led_event``.

LED effect
**********

The LED effect defines the LED behavior over time for the LED diodes by setting their brightness level periodically. This allows for different RGB or monochromatic colors.
An example of a LED effect may be the LED that is blinking or breathing with a given color.

The LED effect (:c:type:`struct led_effect`) is described by the following characteristics:
- pointer to array of LED steps (:cpp:member:`steps`),
- size of the array (:cpp:member:`step_count`),
- flag indicating if the sequence should start over after it finishes (:cpp:member:`loop_forever`).

To achieve the desired LED effect, the LED color is updated periodically based on LED steps defined for the given LED effect, which in turn are divided in multiple smaller updates called substeps.
During every substep, the next LED color is calculated using a linear approximation between the current LED color and the color described in the next LED step (:cpp:member:`color`).
A single LED step also defines the number of substeps for color change between the given LED step and the previous one  (:cpp:member:`substep_count`), as well as the period of time between color updates (:cpp:member:`substep_time`).
After achieving the color described in the next step, the index of the next step is updated.
After the last step, the sequence restarts if the :cpp:member:`loop_forever` flag is set for the given LED effect.
If the flag is not set, the sequence stops and the given LED effect ends.

Module Events
*************

+------------------+---------------+-------------+------------------+-------------+
| Source Module    | Input Event   | This Module | Output Event     | Sink Module |
+==================+===============+=============+==================+=============+
| ``led_state``    | ``led_event`` | ``leds``    |                  |             |
+------------------+---------------+-------------+------------------+-------------+

Configuration
*************

The module uses the Zephyr :ref:`zephyr:pwm_api` driver for setting the LED color (that is, brightness of the diodes).
To use the module, set the following configuration options:
- :option:`CONFIG_PWM`
- :option:`CONFIG_PWM_0`

In :option:`CONFIG_PWM_0`, 0 corresponds to the PWM port number. Make sure to configure all PWM ports used by application.

The module is enabled using ``CONFIG_DESKTOP_LED_ENABLE``.

You can use the following additional configuration options:
* number of LEDs in use (``CONFIG_DESKTOP_LED_COUNT``),
* number of color channels per diode (``CONFIG_DESKTOP_LED_COLOR_COUNT``),
* maximum value of LED brightness (``CONFIG_DESKTOP_LED_BRIGHTNESS_MAX``).

Symbols from :ref:`zephyr:dt-guide` are used to define the mapping of the PWM channels to pin numbers.
By default, the symbols are defined in the board configuration, but you can redefine them in the ``dts.overlay`` file.
The configuration for the board is written in the ``leds_def.h`` file.
Both files are located in the board-specific directory in the application configuration folder.

Implementation details
**********************

The LED color is achieved by setting the proper pulse widths for the PWM signals.
To achieve the desired LED effect, colors for the given LED are periodically updated using work (:c:type:`struct k_delayed_work`).
One work automatically updates the color of a single LED.

This module turns off all LEDs when the application goes to the power down state.
In such case, the PWM drivers are set to the suspended state to reduce the power consumption.
If the application goes to the error state, the LEDs are used to indicate error.

.. _nrf_desktop_leds:

LEDs module
###########

You can control LED diodes with the LEDs module, which sets the LED effect in response to a ``led_event``.

LED effect
**********

The LED effect defines the LED behavior over time for the LED diodes by setting their brightness level periodically.
This allows for different RGB or monochromatic colors.
An example may be an LED that is blinking or breathing with a given color.
Such LED behavior is referred to as *LED effect*.

The LED effect (:c:type:`struct led_effect`) is described by the following characteristics:

* Pointer to array of LED steps (:cpp:member:`steps`).
* Size of the array (:cpp:member:`step_count`).
* Flag indicating if the sequence should start over after it finishes (:cpp:member:`loop_forever`).

To achieve the desired LED effect, the LED color is updated periodically based on LED steps defined for the given LED effect, which in turn are divided in multiple smaller updates called *substeps*.

During every substep, the next LED color is calculated using a linear approximation between the current LED color and the :cpp:member:`color` described in the next LED step.
A single LED step also defines the number of substeps for color change between the given LED step and the previous one (:cpp:member:`substep_count`), as well as the period of time between color updates (:cpp:member:`substep_time`).
After achieving the color described in the next step, the index of the next step is updated.

After the last step, the sequence restarts if the :cpp:member:`loop_forever` flag is set for the given LED effect.
If the flag is not set, the sequence stops and the given LED effect ends.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_leds_start
    :end-before: table_leds_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module uses Zephyr's :ref:`zephyr:pwm_api` driver for setting the LED color (that is, the brightness of the diodes).
To use the module, set the following configuration options:

* :option:`CONFIG_PWM`

The PWM ports needed by the application are enabled in the :file:`dts.overlay` file by setting the status to "okay".
Make sure to configure all PWM ports that are used by the application.

The module is enabled using ``CONFIG_DESKTOP_LED_ENABLE``.

You can use the following additional configuration options:

* ``CONFIG_DESKTOP_LED_COUNT`` - Number of LEDs in use.
* ``CONFIG_DESKTOP_LED_COLOR_COUNT`` - Number of color channels per diode.
* ``CONFIG_DESKTOP_LED_BRIGHTNESS_MAX`` - Maximum value of LED brightness.

Symbols from Zephyr's :ref:`zephyr:dt-guide` are used to define the mapping of the PWM channels to pin numbers.
By default, the symbols are defined in the board configuration, but you can redefine them in the :file:`dts.overlay` file.
The configuration for the board is written in the :file:`leds_def.h` file.
Both files are located in the board-specific directory in the application configuration directory.

Implementation details
**********************

The LED color is achieved by setting the proper pulse widths for the PWM signals.
To achieve the desired LED effect, colors for the given LED are periodically updated using work (:c:type:`struct k_delayed_work`).
One work automatically updates the color of a single LED.

This module turns off all LEDs when the application goes to the power down state.
In such case, the PWM drivers are set to the suspended state to reduce the power consumption.
If the application goes to the error state, the LEDs are used to indicate error.

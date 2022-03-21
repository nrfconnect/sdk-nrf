.. _dk_buttons_and_leds_readme:

DK Buttons and LEDs
###################

.. contents::
   :local:
   :depth: 2

The DK Buttons and LEDs library is a simple module to interface with the buttons and LEDs on a Nordic Semiconductor development kit.
It supports reading the state of up to four buttons or switches and controlling up to four LEDs.

Configuration
*************

To enable the library in your project, set the :kconfig:option:`CONFIG_DK_LIBRARY` Kconfig option to ``y``.

If you want to retrieve information about the button state, initialize the library with :c:func:`dk_buttons_init`.
You can pass a callback function during initialization.
This function is then called every time the button state changes.

If you want to control the LEDs on the development kit, initialize the library with :c:func:`dk_leds_init`.
You can then set the value of a single LED, or set all of them to a state specified through bitmasks.

To use this library on your own hardware, see :ref:`dk_buttons_and_leds_other_boards` below.

API documentation
*****************

| Header file: :file:`include/dk_buttons_and_leds.h`
| Source files: :file:`lib/dk_buttons_and_leds/`

.. doxygengroup:: dk_buttons_and_leds
   :project: nrf
   :members:

.. _dk_buttons_and_leds_other_boards:

Use on non-DK boards
********************

You can define buttons and LEDs in your own board's :ref:`devicetree <dt-guide>` to use this library with custom hardware.

To define your own buttons, adapt this example to your board's hardware in your board's devicetree file.
The example shows different ways to define buttons to show how to handle different hardware.

.. code-block:: DTS

   / {
           buttons {
                   compatible = "gpio-keys";
                   /*
                    * Add up to 4 total buttons in child nodes as shown here.
                    */
                   button0: button_0 {
                           /* Button 0 on P0.11. Enable internal SoC pull-up
                            * resistor and treat low level as pressed button. */
                           gpios = <&gpio0 11 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
                           label = "Button 0";
                   };
                   button1: button_1 {
                           /* Button 1 on P0.12. Enable internal pull-down resistor.
                            * Treat high level as pressed button. */
                           gpios = <&gpio0 12 (GPIO_PULL_DOWN | GPIO_ACTIVE_HIGH)>;
                           label = "Button 1";
                   };
                   button2: button_2 {
                           /* Button 2 on P1.12, enable internal pull-up,
                            * low level is pressed. */
                           gpios = <&gpio1 12 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
                           label = "Button 2";
                   };
                   button3: button_3 {
                           /* Button 3 on P1.15, no internal pull resistor,
                            * low is pressed. */
                           gpios = <&gpio1 15 GPIO_ACTIVE_LOW>;
                           label = "Button 3";
                   };
           };
   };

To define your own LEDs, adapt this example:

.. code-block:: DTS

   / {
           leds {
                   compatible = "gpio-leds";
                   led_0 {
                           /* LED 0 on P0.13, LED on when pin is high */
                           gpios = < &gpio0 13 GPIO_ACTIVE_HIGH >;
                           label = "LED 0";
                   };
                   led_1 {
                           /* LED 1 on P0.14, LED on when pin is low */
                           gpios = < &gpio0 14 GPIO_ACTIVE_LOW >;
                           label = "LED 1";
                   };
                   led_2 {
                           /* LED 2 on P1.0, on when low */
                           gpios = < &gpio1 0 GPIO_ACTIVE_LOW >;
                           label = "LED 2";
                   };
                   led_3 {
                           /* LED 3 on P1.1, on when high */
                           gpios = < &gpio1 1 GPIO_ACTIVE_HIGH >;
                           label = "LED 3";
                   };
        };
   };

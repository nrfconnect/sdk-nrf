.. _dk_buttons_and_leds_readme:

DK Button and LEDs
##################

.. contents::
   :local:
   :depth: 2

The DK Button and LED library is a simple module to interface with the buttons and LEDs on a Nordic Semiconductor development kit.
It supports reading the state of up to four buttons or switches and controlling up to four LEDs.

If you want to retrieve information about the button state, initialize the library with :c:func:`dk_buttons_init`.
You can pass a callback function during initialization.
This function is then called every time the button state changes.

If you want to control the LEDs on the development kit, initialize the library with :c:func:`dk_leds_init`.
You can then set the value of a single LED, or set all of them to a state specified through bitmasks.

API documentation
*****************

| Header file: :file:`include/dk_buttons_and_leds.h`
| Source files: :file:`lib/dk_buttons_and_leds/`

.. doxygengroup:: dk_buttons_and_leds
   :project: nrf
   :members:

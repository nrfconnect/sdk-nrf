.. _nrf_desktop_leds:

LEDs module
###########

.. contents::
   :local:
   :depth: 2

The LEDs module is responsible for controlling LEDs in response to LED effect set by a ``led_event``.
The source of events could be any other module.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_leds_start
    :end-before: table_leds_end

.. note::
    |nrf_desktop_module_event_note|

Implementation details
**********************

nRF Desktop uses the buttons module from :ref:`lib_caf` (CAF).
See the :ref:`CAF button module <caf_leds>` page for implementation details.

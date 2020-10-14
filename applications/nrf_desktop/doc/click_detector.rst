.. _nrf_desktop_click_detector:

Click detector module
#####################

.. contents::
   :local:
   :depth: 2

The |click_detector| is used to send a ``click_event`` when a known type of click is recorded for the button defined in the module configuration.

Click type
**********

The module records the following click types:

* :c:enumerator:`CLICK_SHORT` - Button pressed and released after short time.
* :c:enumerator:`CLICK_NONE` - Button pressed and held for a period of time that is too long for :c:enumerator:`CLICK_SHORT`, but too short for :c:enumerator:`CLICK_LONG`.
* :c:enumerator:`CLICK_LONG` - Button pressed and held for a long period of time.
* :c:enumerator:`CLICK_DOUBLE` - Two sequences of the button press and release in a short time interval.

The exact values of time intervals for click types are defined in the :file:`click_detector.c` file.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_click_detector_start
    :end-before: table_click_detector_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The |click_detector| detects click types based on ``button_event``.
Make sure you define the :ref:`nrf_desktop_buttons` hardware interface.

Set :option:`CONFIG_DESKTOP_CLICK_DETECTOR_ENABLE` and define the module configuration.
The configuration (array of :c:struct:`click_detector_config`) is written in the :file:`click_detector_def.h`` file located in the board-specific directory in the application configuration directory.

For every click detector, make sure to define the following information:

* :c:member:`click_detector_config.key_id` - ID of the selected key.
* :c:member:`click_detector_config.consume_button_event` - Whether the ``button_event`` with the given :c:member:`click_detector_config.key_id` should be consumed by the module.

Implementation details
**********************

Tracing of key states is implemented using a periodically submitted work (:c:struct:`k_delayed_work`).
The work updates the states of traced keys and sends ``click_event``.
The work is not submitted if there is no key for which the state should be updated.

.. |click_detector| replace:: click detector module

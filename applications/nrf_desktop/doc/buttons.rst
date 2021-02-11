.. _nrf_desktop_buttons:

Buttons module
##############

.. contents::
   :local:
   :depth: 2

The buttons module is responsible for generating events related to key presses.
The source of events are changes to GPIO pins.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_buttons_start
    :end-before: table_buttons_end

.. note::
    |nrf_desktop_module_event_note|

Implementation details
**********************

For implementation details refer to :ref:`caf_buttons`.

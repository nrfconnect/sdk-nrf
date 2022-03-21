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

nRF Desktop uses the buttons module from :ref:`lib_caf` (CAF).
See the :ref:`CAF button module <caf_buttons>` page for implementation details.

Key ID
======

If :ref:`CONFIG_DESKTOP_FN_KEYS_ENABLE <config_desktop_app_options>` is enabled, the :c:member:`button_event.key_id` additionally encodes if a button related to :c:struct:`button_event` is a function key.
See the :ref:`nRF Desktop function key module <nrf_desktop_fn_keys>` page for implementation details.

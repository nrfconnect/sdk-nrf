.. _nrf_desktop_hid_state_pm:

HID state power manager module
##############################

.. contents::
   :local:
   :depth: 2

The |hid_state_pm| is a minor module that prevents suspending the device when HID reports are exchanged with the host.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_state_pm_start
    :end-before: table_hid_state_pm_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module is enabled by :ref:`CONFIG_DESKTOP_HID_STATE_PM_ENABLE <config_desktop_app_options>` Kconfig option.
The option depends on the :kconfig:option:`CONFIG_CAF_POWER_MANAGER` and :ref:`CONFIG_DESKTOP_HID_STATE_ENABLE <config_desktop_app_options>` Kconfig options.
The option is enabled by default.

Implementation details
**********************

The module relies on :c:struct:`hid_report_event` to detect HID report exchange.

.. note::
   In the nRF Desktop application, most of the HID reports are broadcasted as :c:struct:`hid_report_event`, but there are exceptions.
   For example, the :ref:`nrf_desktop_config_channel` uses HID feature reports or HID output reports as transport and the configuration channel data is broadcasted using :c:struct:`config_event` in the application.
   Hence, the |hid_state_pm| does not prevent suspending the device when the configuration channel is in use.

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

The module is enabled by :option:`CONFIG_DESKTOP_HID_STATE_PM_ENABLE` Kconfig option.
The option depends on the :kconfig:option:`CONFIG_CAF_POWER_MANAGER` and :option:`CONFIG_DESKTOP_HID_STATE_ENABLE` Kconfig options.
It selects the :kconfig:option:`CONFIG_CAF_KEEP_ALIVE_EVENTS` Kconfig option to enable support for the :c:struct:`keep_alive_event`.
The option is enabled by default.

Implementation details
**********************

The module relies on :c:struct:`hid_report_event` to detect HID report exchange.
The module submits a :c:struct:`keep_alive_event` on the HID report exchange to prevent application power down.

.. note::
   In the nRF Desktop application, most of the HID reports are broadcasted as :c:struct:`hid_report_event`, but there are exceptions.
   For example, the :ref:`nrf_desktop_config_channel` uses HID feature reports or HID output reports as transport and the configuration channel data is broadcasted using :c:struct:`config_event` in the application.
   Hence, the |hid_state_pm| does not prevent suspending the device when the configuration channel is in use.

Tracking power manager restrictions
===================================

If the power manager events are supported (:kconfig:option:`CONFIG_CAF_POWER_MANAGER_EVENTS`), the HID state power manager module subscribes to the :c:struct:`power_manager_restrict_event` to track the maximum allowed power level.
If the :c:enum:`POWER_MANAGER_LEVEL_ALIVE` power level is enforced by any application module, the module skips submitting the :c:struct:`keep_alive_event` to reduce the number of performed operations and improve performance.

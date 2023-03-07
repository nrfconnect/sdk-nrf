.. _nrf_desktop_watchdog:

Watchdog module
###############

.. contents::
   :local:
   :depth: 2

The watchdog module is responsible for controlling the watchdog timer.

The watchdog timer is used to prevent hangs caused by software or hardware faults.
When the system hangs, it is automatically restarted by the watchdog timer.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_watchdog_start
    :end-before: table_watchdog_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module uses Zephyr's :ref:`zephyr:watchdog_api` driver.
For this reason, it automatically selects the :kconfig:option:`CONFIG_WATCHDOG` option.

The module is enabled by the :ref:`CONFIG_DESKTOP_WATCHDOG_ENABLE <config_desktop_app_options>` option.

You must define :ref:`CONFIG_DESKTOP_WATCHDOG_TIMEOUT <config_desktop_app_options>` option.
After this amount of time (in ms), the device will be restarted if the watchdog timer was not reset.

.. note::
    The module is by default used only in the release configurations that do not enable logs.
    When the :ref:`CONFIG_DESKTOP_LOG <config_desktop_app_options>` Kconfig option is enabled, enabling watchdog timer can cause losing logs, for example, when the logger is in the panic mode.

Implementation details
**********************

The watchdog timer is started when the :ref:`nrf_desktop_main` is ready (which is reported using :c:struct:`module_state_event`).
The module periodically resets the watchdog timer using :c:struct:`k_work_delayable`.
The work resubmits itself with delay equal to ``CONFIG_DESKTOP_WATCHDOG_TIMEOUT / 3``.
In case of the system hang, the work will not be processed, the watchdog timer will not be reset on time, and the system will be restarted.

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
For this reason, set the :option:`CONFIG_WATCHDOG` option.

The module is enabled by the :option:`CONFIG_DESKTOP_WATCHDOG_ENABLE` option.

You must define :option:`CONFIG_DESKTOP_WATCHDOG_TIMEOUT`.
After this amount of time (in ms), the device will be restarted if the watchdog timer was not reset.

.. note::
    The module is used only in the release configurations (``ZRelease``, ``ZReleaseB0``).
    For the :ref:`debug configurations <nrf_desktop_requirements_build_types>` (``ZDebug``, ``ZDebugWithShell``, ``ZDebugB0``), enabling watchdog timer can cause losing logs, for example when the logger is in the panic mode.

Implementation details
**********************

The watchdog timer is started when the :ref:`nrf_desktop_main` is ready (which is reported using ``module_state_event``).
The module periodically resets the watchdog timer using :c:struct:`k_delayed_work`.
The work resubmits itself with delay equal to ``CONFIG_DESKTOP_WATCHDOG_TIMEOUT / 3``.
In case of the system hang, the work will not be processed, the watchdog timer will not be reset on time, and the system will be restarted.

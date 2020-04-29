.. _nrf_desktop_watchdog:

Watchdog module
###############

The ``watchdog`` module is responsible for controlling the watchdog timer.
The watchdog timer is used to prevent hangs caused by software or hardware faults.
When the system hangs, it is automatically restarted by the watchdog timer.

Module Events
*************

+----------------+-------------+--------------+-----------------+------------------+
| Source Module  | Input Event | This Module  | Output Event    | Sink Module      |
+================+=============+==============+=================+==================+
|                |             | ``watchdog`` |                 |                  |
+----------------+-------------+--------------+-----------------+------------------+

Configuration
*************

The module uses the Zephyr :ref:`zephyr:watchdog_api` driver.
For this reason, set the :option:`CONFIG_WATCHDOG` option.

The module is enabled by the ``CONFIG_DESKTOP_WATCHDOG_ENABLE`` option.

You must define ``CONFIG_DESKTOP_WATCHDOG_TIMEOUT``.
After this amount of time (in ms), the device will be restarted if the watchdog timer was not reset.

.. note::
  The module is used only in the release configurations (``ZRelease``, ``ZReleaseB0``).
  For the debug configurations (``ZDebug``, ``ZDebugWithShell``, ``ZDebugB0``), enabling watchdog timer can cause losing logs, for example when the logger is in the panic mode.

Implementation details
**********************

The watchdog timer is started when the ``main`` module is ready (reported using ``module_state_event``).
The module periodically resets the watchdog timer using :c:type:`struct k_delayed_work`.
The work resubmits itself with delay equal to ``CONFIG_DESKTOP_WATCHDOG_TIMEOUT / 3``.
In case of the system hang, the work will not be processed, the watchdog timer will not be reset on time, and the system will be restarted.

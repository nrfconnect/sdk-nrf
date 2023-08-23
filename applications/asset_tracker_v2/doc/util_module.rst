.. _asset_tracker_v2_util_module:

Utility module
##############

.. contents::
   :local:
   :depth: 2

The utility module provides the functionality to administer and monitor mechanisms in the application architecture.

Features
********

This section documents the various features implemented by the module.

Application Shutdown
====================

When an irrecoverable error or a successful FOTA update is reported to the utility module, it initiates a shutdown sequence and ensures a graceful shutdown of the application.

During initialization, each module registers if it supports graceful shutdown.
This is done by calling the :c:func:`module_start` function in the :ref:`api_modules_common` and passing a configuration value in the :c:struct:`module_data` structure for each module that registers for graceful shutdown.
This then expands an internal list in the :ref:`api_modules_common`.

The typical chain of events that completes the shutdown sequence is as follows:

1. A module detects an irrecoverable error and sends an event carrying the associated error code. This event name is typically the module name suffixed with ``EVT_ERROR``. For example, :c:enum:`CLOUD_EVT_ERROR`.
#. The utility module receives the error event and starts a reboot timer that times out after the duration specified by the :ref:`CONFIG_REBOOT_TIMEOUT <CONFIG_REBOOT_TIMEOUT>` option. When this timer expires, the application reboots in any case.
#. The utility module sends out a shutdown request, :c:enum:`UTIL_EVT_SHUTDOWN_REQUEST`, that is handled individually by each module in the system that supports shutdown.
#. When each module has handled the shutdown request, it acknowledges the shutdown by sending an event suffixed with ``EVT_SHUTDOWN_READY``. For example, :c:enum:`CLOUD_EVT_SHUTDOWN_READY`.
#. When the utility module receives a shutdown acknowledgment, for each incoming shutdown acknowledgment, the aforementioned list in the :ref:`api_modules_common` is checked using :c:func:`modules_shutdown_register`. When all the modules in the list have acknowledged the request, the utility module reboots the application at a much earlier point than the time set initially by the reboot timer.

Watchdog
========

The module implements a watchdog library that monitors the system workqueue using the :ref:`Zephyr Watchdog driver <watchdog_api>`.
To configure the watchdog timeout that is used, set the :ref:`CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC <CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC>` Kconfig option.
The header file of the library is located at :file:`asset_tracker_v2/src/watchdog/watchdog_app.h`.

If the watchdog is not fed within the timeout indicated by :ref:`CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC <CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC>`, a watchdog timeout occurs, causing a reboot that is initiated by the watchdog peripheral hardware unit on the nRF91 Series DK.
The watchdog library is set up to feed the :ref:`Zephyr Watchdog driver <watchdog_api>` with the system workqueue constantly at a time interval that equals half of the value specified by :ref:`CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC <CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC>`.
This means that if the watchdog timeout is set to 60 seconds, the system workqueue feeds the watchdog every 30 seconds.
A reboot caused by a watchdog timeout occurs if the system workqueue is blocked and it is unable to feed the watchdog.

Configuration options
*********************

You can set the following options to configure the utility module:

.. _CONFIG_REBOOT_TIMEOUT:

CONFIG_REBOOT_TIMEOUT - Utility module reboot timeout
   This option specifies the timeout within which the utility module initiates a reboot, after an irrecoverable error has been reported to the module.
   However, if all modules acknowledge the utility module's shutdown request before this timeout expires, the reboot occurs earlier.

.. _CONFIG_WATCHDOG_APPLICATION:

CONFIG_WATCHDOG_APPLICATION - Enable the application watchdog
   This option enables the application watchdog timer.

.. _CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC:

CONFIG_WATCHDOG_APPLICATION_TIMEOUT_SEC - Application watchdog timeout
   This option specifies the watchdog timeout.

Module states
*************

This module has no internal states.

Module events
*************

The :file:`asset_tracker_v2/src/events/util_module_event.h` header file contains a list of various events sent by the module.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* :ref:`app_event_manager`
* :file:`asset_tracker_v2/src/watchdog/watchdog_app.h`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/util_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/util_module_event.c`, :file:`asset_tracker_v2/src/modules/util_module.c`

.. doxygengroup:: util_module_event
   :project: nrf
   :members:

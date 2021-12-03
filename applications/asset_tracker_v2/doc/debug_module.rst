.. _asset_tracker_v2_debug_module:

Debug module
############

.. contents::
   :local:
   :depth: 2

The debug module intends to improve the overall debugging experience in the application.
By default, it subscribes to all the events in the system and implements support for `Memfault`_ through the :ref:`mod_memfault` module integrated in |NCS|.

.. note::

   To enable the debug module, you must include the :file:`../overlay-debug.conf` overlay configuration file when building the application.

Features
********

This section documents the various features implemented by the module.

Memfault
========

Memfault SDK provides the functionality to collect information from devices in the form of coredumps, error traces, metrics, logs and more.
Information that is collected from the device can be sent to Memfault's cloud solution for further analysis.

The debug module uses `Memfault SDK`_ to track |NCS| specific metrics such as LTE and stack metrics.
In addition, the following types of custom Memfault metrics are defined and tracked when compiling in the debug module:

 * ``GpsTimeToFix`` - Time duration between the start of a GNSS search and obtaining a fix.
 * ``GpsTimeoutSearchTime`` - Time duration between the start of a GNSS search and a search timeout.
 * ``GpsSatellitesTracked`` - Number of satellites tracked during a GNSS search window.

The debug module also implements `Memfault SDK`_ software watchdog, which is designed to trigger an assert before an actual watchdog timeout.
This enables the application to be able to collect coredump data before a reboot occurs.

To enable Memfault, you must include the :file:`../overlay-memfault.conf` when building the application.
To get started with Memfault, see :ref:`using_memfault`.

Configuration options
*********************

.. _CONFIG_DEBUG_MODULE_MEMFAULT_USE_EXTERNAL_TRANSPORT:

CONFIG_DEBUG_MODULE_MEMFAULT_USE_EXTERNAL_TRANSPORT - Configuration for transfer of Memfault data
   This option, if enabled, makes the debug module trigger events carrying Memfault data. This data can be routed through an external transport to Memfault cloud, for example, through AWS IoT, Azure IoT Hub, or `nRF Cloud`_.

.. _CONFIG_DEBUG_MODULE_MEMFAULT_HEARTBEAT_INTERVAL_SEC:

CONFIG_DEBUG_MODULE_MEMFAULT_HEARTBEAT_INTERVAL_SEC - Configuration for nRF Connect SDK Memfault metrics tracking interval
   This option sets the time interval for tracking |NCS| Memfault metrics.

.. _CONFIG_DEBUG_MODULE_MEMFAULT_CHUNK_SIZE_MAX:

CONFIG_DEBUG_MODULE_MEMFAULT_CHUNK_SIZE_MAX - Configuration for maximum size of transmitted packets
   This option sets the maximum size of packets transmitted over the configured custom transport.

Module configurations
=====================

To enable Memfault, you must set the following mandatory options:

 * :kconfig:`CONFIG_MEMFAULT`
 * :kconfig:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`

To get detailed stack traces in coredumps sent to Memfault, you can increase the values for the following options:

 * :kconfig:`CONFIG_PM_PARTITION_SIZE_MEMFAULT_STORAGE` - For example, you can increase the value to ``0x30000``.
 * :kconfig:`CONFIG_MEMFAULT_COREDUMP_STACK_SIZE_TO_COLLECT` - For example, you can increase the value to ``8192``.

For extended documentation regarding |NCS| Memfault integration, see :ref:`mod_memfault` module.

Module states
*************

This module has no internal states.

Module events
*************

The :file:`asset_tracker_v2/src/events/debug_module_event.h` header file contains a list of various events sent by the module.

Dependencies
************

This application uses the following |NCS| libraries and drivers:

* :ref:`event_manager`
* :ref:`mod_memfault`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/debug_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/debug_module_event.c`

.. doxygengroup:: debug_module_event
   :project: nrf
   :members:

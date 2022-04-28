.. _nrf_desktop_nrf_profiler_sync:

nRF Profiler synchronization module
###################################

.. contents::
   :local:
   :depth: 2

Use the nRF Profiler synchronization module to synchronize the timestamps of :ref:`nrf_profiler` events between two devices connected over a physical wire.
The timestamp synchronization is required to increase the accuracy of the amount of time measured between the nRF Profiler events originating from two different devices.

The nRF Profiler data is collected separately from both devices using dedicated Python scripts.
The data can then be merged using the :file:`merge_data.py` script with :c:struct:`sync_event` used as a synchronization event for both Peripheral and Central.
For more details, see the :ref:`nrf_profiler` documentation.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_nrf_profiler_sync_start
    :end-before: table_nrf_profiler_sync_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

A predefined signal on the GPIO is used to simultaneously generate synchronization nRF Profiler events on both devices.
For this reason, you must enable the :kconfig:option:`CONFIG_GPIO` option.

You must also enable the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER` Kconfig option.
The nRF Profiler synchronization module generates an :ref:`nrf_profiler` event (:c:struct:`sync_event`) that is not an :ref:`app_event_manager` event.
For this reason, the :c:struct:`sync_event` execution is not traced.

You must also define the following options:

* The GPIO port (:ref:`CONFIG_DESKTOP_NRF_PROFILER_SYNC_GPIO_PORT <config_desktop_app_options>`) and the pin (:ref:`CONFIG_DESKTOP_NRF_PROFILER_SYNC_GPIO_PIN <config_desktop_app_options>`) that are used for synchronization.
  These GPIOs must be defined separately for both devices and connected using a physical wire.
* The device role.
  One of the devices must be set as Central (:ref:`CONFIG_DESKTOP_NRF_PROFILER_SYNC_CENTRAL <config_desktop_app_options>`) and the other device must be set as Peripheral (:ref:`CONFIG_DESKTOP_NRF_PROFILER_SYNC_PERIPHERAL <config_desktop_app_options>`).

Implementation details
**********************

The nRF Profiler synchronization ``Central`` generates a predefined signal that is received by ``Peripheral`` over the physical wire.
Both devices generate an nRF Profiler event (:c:struct:`sync_event`) on every signal edge.
This results in :c:struct:`sync_event` being generated on both devices at the same time.
You can use the timestamps of this event on both devices for clock drift compensation.

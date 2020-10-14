.. _nrf_desktop_profiler_sync:

Profiler synchronization module
###############################

.. contents::
   :local:
   :depth: 2

Use the profiler synchronization module to synchronize the timestamps of :ref:`profiler` events between two devices connected over a physical wire.
Timestamp synchronization is required to increase the accuracy of the measured times between the profiler events that come from two different devices.

The profiler data is collected separately from both devices using dedicated Python scripts.
The data can then be merged using the ``merge_data.py`` script with ``sync_event`` used as a synchronization event for both ``Peripheral`` and ``Central``.
For more detailed information, see the :ref:`profiler` documentation.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_profiler_sync_start
    :end-before: table_profiler_sync_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

A predefined signal on the GPIO is used to simultaneously generate synchronization profiler events on both devices.
For this reason, you must enable the :option:`CONFIG_GPIO` option.

Make also sure that the :option:`CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED` Kconfig option is enabled and the :option:`CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION` Kconfig option is disabled.
The profiler synchronization module generates a :ref:`profiler` event (``sync_event``) that is not an :ref:`event_manager` event.
For this reason, the ``sync_event`` execution is not traced.

You also need to define:

* the GPIO port (:option:`CONFIG_DESKTOP_PROFILER_SYNC_GPIO_PORT`) and the pin (:option:`CONFIG_DESKTOP_PROFILER_SYNC_GPIO_PIN`) that are used for synchronization
  These GPIOs must be defined separately for both devices and connected using a physical wire.
* the device role
  One of the devices must be set as ``Central`` (:option:`CONFIG_DESKTOP_PROFILER_SYNC_CENTRAL`) and the other device must be set as ``Peripheral`` (:option:`CONFIG_DESKTOP_PROFILER_SYNC_PERIPHERAL`).

Implementation details
**********************

The profiler synchronization ``Central`` generates a predefined signal that is received by ``Peripheral`` over the physical wire.
Both devices generate a profiler event (``sync_event``) on every signal edge.
That results in ``sync_event`` being generated on both devices at the same time.
Timestamps of this event on both devices can be used for clock drift compensation.

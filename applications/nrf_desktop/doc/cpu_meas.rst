.. _nrf_desktop_cpu_meas:

CPU measurement module
######################

.. contents::
   :local:
   :depth: 2

Use the CPU measurement module to monitor CPU load.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_cpu_meas_start
    :end-before: table_cpu_meas_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Enable the module using the :option:`CONFIG_DESKTOP_CPU_MEAS_ENABLE` Kconfig option.
This Kconfig option selects the :option:`CONFIG_CPU_LOAD` option.
The :option:`CONFIG_CPU_LOAD` option enables :ref:`cpu_load`, that is used to perform the measurements.

Set the time between subsequent CPU load measurements, in milliseconds, using the :option:`CONFIG_DESKTOP_CPU_MEAS_PERIOD` option.

Implementation details
**********************

The module periodically submits the measured CPU load as :c:struct:`cpu_load_event` and resets the measurement.
The event can be displayed in the logs or using the :ref:`profiler`.
The :c:member:`cpu_load_event.load` presents the CPU load in 0,001% units.

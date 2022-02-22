.. _cpu_load:

CPU load measurement
####################

.. contents::
   :local:
   :depth: 2

The CPU load measurement module allows you to monitor the CPU utilization based on the amount of time during which the CPU has been asleep.
The module measures the amount of the sleep time and divides this value by the total amount of time since the reset of the measurement.
The resulting CPU load value is a percentage value that corresponds to the amount of time when the CPU has not been sleeping.
For example, a value of 75% means that the CPU has been busy for 75% of the time during the measurement period.
This can be used for debugging purposes to calculate the CPU load when using an application.

To precisely measure the sleep period, the module requires the POWER peripheral events SLEEPENTER and SLEEPEXIT.
The events are connected to a TIMER peripheral using PPI/DPPI.

The sleep period is measured using the TIMER peripheral, which is clocked by default by the high frequency clock.
Alternatively, it can be clocked using low frequency clock (see :kconfig:option:`CONFIG_CPU_LOAD_ALIGNED_CLOCKS`).
It is then compared against the system clock, which is clocked by the low frequency clock.
The accuracy of measurements depends on the accuracy of the given clock sources.

Configuration
*************

The module allows you to configure the following options in Kconfig:

* Enabling or disabling the shell commands for controlling the library.
* Toggling the periodic load measurement logging.
* Enabling the alignment of the clock sources for more accurate measurement.
* Choosing the TIMER instance for the load measurement.


Usage
*****

The module allows the following usage scenarios:

Enabling the module
    Use :c:func:`cpu_load_init` to initialize the module.
    Calling this function resets the TIMER peripheral and the system clock.

    The module can be enabled also by using the ``cpu_load init`` command, if you enabled the shell commands.

Getting the results
    After the initialization, you can get the CPU load value by calling the :c:func:`cpu_load_get` function.

    * If called for the first time, the function will provide the load value since the start of the measurement period.
    * If called after a reset, the function will provide the load value since the last reset.

    You can also get the CPU load value by using the ``cpu_load get`` or the ``cpu_load`` command, if you enabled the shell commands.

    In the periodic load measurement logging, the :c:func:`cpu_load_get` function is called alternately with the :c:func:`cpu_load_reset`.

Resetting the measurement
    You can reset the TIMER peripheral and the system clock read-out by using :c:func:`cpu_load_reset`.
    This provides a new reference point from which the :c:func:`cpu_load_get` function measures the CPU load.

    You can also reset the measurement using the ``cpu_load reset`` command, if you enabled the shell commands.


API documentation
*****************

| Header file: :file:`include/debug/cpu_load.h`
| Source files: :file:`subsys/debug/cpu_load/`

.. doxygengroup:: cpu_load
   :project: nrf
   :members:

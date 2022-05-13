.. _boost_cpu:

nRF5340 CPU boost
#################

.. contents::
   :local:
   :depth: 2

The CPU boost is a simple library that increases the frequency of the nRF5340 SoC's CPU clock.
When enabled, the frequency of the CPU clock is doubled, which can improve the code execution time at the expense of higher power consumption.

The CPU boost is handled through the :c:func:`cpu_boost` function.

Configuration
*************

Set :kconfig:option:`CONFIG_BOOST_CPU_CLOCK` to enable the CPU boost library for the nRF5340 SoC.
This option lets you change the frequency at run time.

You can also set :kconfig:option:`CONFIG_BOOST_CPU_CLOCK_AT_STARTUP` to enable the CPU boost library for the nRF5340 SoC on the kernel initialization.
The boost then takes place after the initialization of the kernel, but before the application is initialized.

API documentation
*****************

| Header file: :file:`include/boost_cpu.h`
| Source files: :file:`lib/boost_cpu/`

.. doxygengroup:: boost_cpu
   :project: nrf
   :members:

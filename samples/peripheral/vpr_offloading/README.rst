.. _vpr_offloading_sample:

VPR offloading
##############

.. contents::
   :local:
   :depth: 2

The VPR offloading sample demonstrates how to use the VPR core (PPR or FLPR) to offload the application core.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

You can use VPR cores to offload background tasks from the application core.
This reduces both the CPU load on the main core and the overall power consumption.
VPR cores execute code from RAM, which means non-volatile memory (NVM) does not need to be woken up.
On the nRF54H20 SoC, PPR runs in the low-power domain clocked at 16 MHz, so the power savings can be significant compared to the application core running at 320 MHz.
However, the VPR code space is limited, so it requires a lightweight offloading framework.

The sample demonstrates a lightweight framework for offloading tasks to the VPR core.
To keep the memory footprint small, the VPR core runs with a reduced configuration.
For example, the :kconfig:option:`CONFIG_MULTITHREADING` Kconfig option is disabled (see :ref:`zephyr:nothread` for details about the limitations).
With this setup, the VPR code easily fits into 32 kB of RAM, including shared RAM for IPC communication

Depending on the configuration, modules can be executed on the application core or the VPR core.
If the VPR core is the executor, the application core communicates with it through the IPC service.
The sample periodically reports the CPU load of the application core.

Offloaded modules
=================

The sample implements two modules that can be offloaded:

* LED control - Blinks the LED using a specific pattern.
* Temperature monitor - Periodically reads the temperature from a sensor and reports when it exceeds the defined range.
  It also periodically reports the average temperature.
  The module can use the LPS22HH sensor.
  If the sensor is not detected, SPI transfers are still performed but with random values.
  The module uses the NRFX_SPIM driver in blocking mode since the transfers are very short.


Example performance results
***************************

The following table shows the results of offloading in a scenario where both modules run for 2.5 seconds followed by a 0.5-second sleep.
The temperature monitor performs an SPI transfer every 1 millisecond.
The LED control module is blinking the **LED0** using various patterns.
Results include CPU load on the application core and average current.

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Target
     - No offloading
     - Offloading
   * - nrf54h20 cpuppr
     - 3.2 %, 126 uA
     - 0.1 %, 71 uA
   * - nrf54lm20a cpuflpr
     - 3.3 %, 217 uA
     - 0.0 %, 217 uA
   * - nrf54lv10a cpuflpr
     - 3.3 %, 228 uA
     - 0.0 %, 194 uA

User interface
**************

LED0:
  It is controlled by the **LED control** module and it blinks in various patterns (short, long, medium blink).
  Pattern is changed after each 0.5-second sleep.

Configuration
*************

|config|

Sample can be configured with or without offloading.

The sample supports the following configurations defined in the :file:`sample.yaml` file:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Configuration
     - Description
   * - ``sample.vpr_offloading.local``
     - Runs all modules on the application core without offloading.
   * - ``sample.vpr_offloading.flpr``
     - Offloads modules to the FLPR core.
   * - ``sample.vpr_offloading.ppr``
     - Offloads modules to the PPR core.

Building and running
********************

.. |sample path| replace:: :file:`samples/peripheral/vpr_offloading`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe the console output of the application core which reports average temperature and CPU load.
#. Observe **LED0** blinking in various patterns.

Dependencies
************

The sample uses the following Zephyr subsystems:

* ``include/ipc/ipc_service.h``
* :ref:`zephyr:logging_api`
* :ref:`zephyr:cpu_load`
* :ref:`zephyr:gpio_api`

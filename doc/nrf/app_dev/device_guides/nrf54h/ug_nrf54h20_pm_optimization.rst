.. _ug_nrf54h20_pm_optimization:

nRF54H20 power management optimization
######################################

.. contents::
   :local:
   :depth: 2

The following are some practical tips for optimizing power management on the nRF54H20 SoC.
For details on the power management architecture, see :ref:`ug_nrf54h20_architecture_pm`.

Selecting the optimal power state
*********************************

In the nRF54H20 SoC, each local domain is responsible for selecting the power state that results in minimal power consumption while maintaining an acceptable level of performance.

Entering a deeper sleep state leads to power savings when the system is idle, but it requires increased power consumption to enter and exit the sleep state.
There is a minimum sleep duration that justifies the energy spent on entering and exiting a sleep state, and this duration varies for each sleep state.

In the SoC, a local domain has full control over entering and exiting local sleep states, allowing it to assess whether entering these sleep states is optimal at any given moment.
However, entering sleep states associated with system-off requires cooperation between local domains and the System Controller.
Local domains have limited control over the time and energy required to enter or exit system-off, as well as the power consumption during system-off.

Latency management
******************

The sources of wake-up latency in the nRF54H20 SoC can be categorized into two types: local and global.
Each CPU is responsible for managing its latency sources, with local sources handled by local domains and global sources managed by the System Controller.

Local cores are responsible for handling latencies caused by restoring the system from suspend-to-RAM states.
Local cores schedule their wake-up in advance of expected events.
The timing of expected events is reported to the power management subsystem in the RTOS by the software modules anticipating these events.
The power management subsystem sets a ``GRTC`` channel in advance of the next expected event to compensate for local wake-up latency.

The System Controller is responsible for handling latencies caused by restoring the system from the system-off state (the warm boot procedure latency).
The System Controller schedules the system wake-up from the system-off state in advance of the next ``GRTC`` event to compensate for the warm-boot latency of the system.

Because the warm-boot latency is compensated by the System Controller, from a local CPU's perspective, the latency when restoring from the local-off state and the system-off state is expected to be the same.

Latency in local domains
========================

Any local software module (like a device driver) can anticipate events like ISRs.
Some of these events have predictable timing, while others have unpredictable timing.
Handling the latency of events with unpredictable timing is the same in both simple and complex systems.

If handling an event with predictable timing requires restoring the state of the software module or the peripherals used by this module before the event is processed, the software module is responsible for scheduling a timer event in advance.
This scheduled event is used to restore the state of the software module or peripherals.

The Power Management subsystem in a local domain is responsible for scheduling a wake-up in advance to compensate for the domain's core state restoration latency from the local power state.
The wake-up time scheduled in advance by the power management subsystem is combined with the advance time added by the software module.
This approach ensures that the local domain and the software modules anticipating an event have sufficient time to fully restore before the event occurs, allowing the event to be handled without latency.

Unretained hardware classification
**********************************

Some power states in the nRF54H20 SoC result in powering off certain peripherals.
The state of these peripherals is not retained by hardware and must be restored by software before the peripheral is activated.

See the following sections for the lists of peripheral groups and the related software modules responsible for restoring the peripheral's state for each group.

Peripherals in local domains
============================

All local domains include a common set of hardware modules.
In addition to these, most local domains also contain domain-specific peripherals.

Common peripherals for all local domains
----------------------------------------

Each local domain contains a set of peripherals that are classified consistently across all local domains.
The following table summarizes the active peripherals that need handling when exiting the *Local Suspend to RAM* state.

+---------------+--------------------+--------------------+--------------------+--------------------------+
|Type           | List of the        | Source of data to  | Time of restoration| Software module          |
|               | peripherals        | restore            |                    | responsible for restoring|
+===============+====================+====================+====================+==========================+
|Active         | * ``MVDMA``        | Device driver's    | Decided by the     | The device driver        |
|peripherals    |                    | code and data      | driver             |                          |
+---------------+--------------------+--------------------+--------------------+--------------------------+

Peripherals specific to the Application Domain
----------------------------------------------

There are no peripherals specific to the Application Domain.

Peripherals specific to the Secure Domain
-----------------------------------------

The Secure Domain contains additional peripherals that require handling in the *Local Suspend to RAM* state.

+---------------+--------------------+--------------------+--------------------+--------------------------+
|Type           | List of the        | Source of data to  | Time of restoration| Software module          |
|               | peripherals        | restore            |                    | responsible for restoring|
+===============+====================+====================+====================+==========================+
|Active         | * ``CRACEN``       | Device driver's    | Decided by the     | The device driver        |
|peripherals    |                    | code and data      | driver             |                          |
+---------------+--------------------+--------------------+--------------------+--------------------------+

Peripherals specific to the Radio Domain
----------------------------------------

The Radio Domain does not implement the *Local Suspend to RAM* state.

Optimization example
********************

This example focuses on a HID device use case (for example, a mouse)

A mouse has typically two main application states:

* Active, when the user is moving the mouse.
* Idle, triggered after an inactivity timeout until an input from the user is detected.

The following is an example workflow for the *Active* application state:

  1. Sample input data from sensors.
  2. Construct a HID report.
  3. Transmit the report over the radio.
  4. Sleep until the next period (usually, less than 1 ms).

Power consumption targets
=========================

The power consumption targets for this example are the following:

* Sleep currents (5 V supply):

  * Application core (Suspend-to-RAM): < 10 µA
  * Radio core (Suspend-to-Idle, cache disabled): < 10 µA

* Active currents (estimates at 5 V):

  * Radio core in idle: ~0.25 mA
  * App core in idle: ~0.15 mA
  * Core wake-up every 1 ms: +0.05 mA
  * ADC sample every 1 ms: +0.10 mA
  * SPI transaction every 1 ms: +0.20 mA
  * IPC message every 1 ms: +0.15 mA

.. note::
   The SPI current can be reduced to 0.03 mA by using the nrfy API instead of the default Zephyr blocking driver.

Recommended Kconfig configuration
=================================

Set these Kconfigs as follows for the application running on the application core:

  * :kconfig:option:`CONFIG_PM` to ``y``
  * :kconfig:option:`CONFIG_PM_S2RAM` to ``y``
  * :kconfig:option:`CONFIG_POWEROFF` to ``y``
  * :kconfig:option:`CONFIG_PM_S2RAM_CUSTOM_MARKING` to ``y``
  * :kconfig:option:`CONFIG_PM_DEVICE` to ``y``
  * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` to ``y``

Set these Kconfigs as follows for the application running on the radio core:

  * :kconfig:option:`CONFIG_PM` to ``y``
  * :kconfig:option:`CONFIG_POWEROFF` to ``y``
  * :kconfig:option:`CONFIG_PM_DEVICE` to ``y``
  * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` to ``y``

You should also follow these suggestions:

  * Disable all unused peripherals before entering sleep (Zephyr's API does this automatically when supported).
  * Add `zephyr,pm-device-runtime-auto` in the DTS for all peripherals with runtime PM support.
  * Build and program an empty image on any unused core to release shared resources.

Memory and cache optimizations
------------------------------

The following recommendations help optimize memory placement and cache usage to minimize power consumption:

    * Relocate frequently accessed code and data to local RAM to avoid waking global domains, especially MRAM, which uses high power.
      For more information, see the :ref:`zephyr:code_data_relocation` page.
    * Profile L1 cache usage to minimize MRAM accesses.
    * Ensure that the MRAM latency manager is disabled (:kconfig:option:`CONFIG_MRAM_LATENCY` is set to ``n``) to power MRAM off when idle.

Peripheral and clock recommendations
------------------------------------

The following guidelines help optimize peripheral and clock usage to minimize power consumption:

    * Use global peripherals from the slow domain (peripheral index 13X) to avoid waking up the fast global domain, which uses more power.
    * If radio is frequently active, keep HFXO enabled (wake-up ≈ 800 µs).
    * For SPI, consider using nRFy or nrfx libraries instead of Zephyr's blocking driver to save ~170 µA.

Single core or dual core
------------------------

When choosing between single-core and dual-core architectures (using either the application core, the radio core, or both), consider the following trade-offs:

    * A single-core solution (radio core only) can reduce IPC overhead (~0.15 mA at 1 kHz).
    * The larger local RAM on the radio core (compared to the application core) allows more code to run off the MRAM.
    * Always load an empty image on the unused core to free global resources allocated to this core by the System Controller on initialization.

Additional considerations
-------------------------

When implementing these recommendations, consider also the following:

    * When using a custom protocol, apply the HMPAN-216 workaround, enabling the 0.8 V rail 40 µs before every radio RX and TX operation.

      .. note::
         This workaround is already implemented for when using |BLE| and Enhanced ShockBurst (ESB).

    * Test with the latest nRF54H20 SoC bundle to benefit from all the latest fixes and improvements.
      For more information, see :ref:`abi_compatibility`.

Power management benchmark
**************************

To benchmark the power consumption in *Idle* state, see :ref:`multicore_idle_test`.

.. _ug_nrf54h20_architecture_pm:

nRF54H20 Power Management
#########################

.. contents::
   :local:
   :depth: 2

The nRF54H20 SoC is a distributed system where each domain tries to achieve minimal power consumption for itself.
When all CPUs are ready to fully minimize power consumption by entering the System Off hardware state, the System Controller prepares the system and triggers the entrance in this state.

To achieve optimal low power consumption, ensure that both the radio and application domains are programmed with firmware, as each domain independently sends its power and clock requirements to the System Controller.

For reference implementations, see the :ref:`multicore_idle_test` samples.
These samples demonstrate configurations where the radio core remains idle while the application core remains active.

Software power states
*********************

The nRF54H20 ARM Cortex CPUs currently support the following software power states:

* Active
* Idle
* Idle with cache disabled
* Local suspend to Ram

Each CPU in the nRF54H20 SoC tries to preserve as much power as possible, independently from other CPUs.
The power management subsystem, operating independently within each CPU, continuously selects the most optimal power state based on the current conditions of the CPU.

The following sections describes the details of each of the software power states available on the nRF54H20 SoC.

Active
======

*Active* is the power state in which the CPU is actively processing.

.. list-table::
   :widths: auto

   * - CPU
     - Powered on and clocked.

   * - RAM
     - Banks used by the executed code are enabled.
       Other banks may be in any state.

   * - System state
     - *System ON*

   * - Peripherals
     - All can be active.
       The state of all inactive peripherals is retained in the hardware flip-flops.

   * - Coprocessors
     - All can be active.
       The state of inactive coprocessors is retained according to their selected sleep state.

   * - System timer (based on GRTC)
     - Active

This is the power state with the highest power consumption.
The software execution latency is minimal.
The primary source of latency is the wake-up of the ``MRAMC``.
The maximum latency depends on the MRAM power state configured by the System Controller.

Idle
====

*Idle* is the lightest sleep power state available in the system where the cache content is retained.

.. list-table::
   :widths: auto

   * - CPU
     - Powered on but suspended (not clocked).

   * - RAM
     - Banks used by the executed code are enabled.
       Other banks may be in any state.

   * - System state
     - *System ON*

   * - Peripherals
     - All can be active.
       The local peripherals are powered and preserve their state.
       The state of inactive peripherals in other domains is retained in the hardware flip-flops.

   * - Coprocessors
     - All can be active.
       The state of inactive coprocessors is retained according to their selected sleep state.

   * - System timer (based on GRTC)
     - Active

The power consumption in this state is lower than in *Active*, but higher than in other power states.
The wake-up latency is higher than in *Active*, but lower than in the other power states.
The primary sources of wake-up latency are the wake-up of the ``MRAMC`` and the startup of clock sources.
The maximum latency depends on the MRAM power state configured by the System Controller.

Local Suspend to RAM
********************

*Local Suspend to RAM* is a sleep state that balances between power consumption and wake-up latency.

This state is available only for DVFS-capable domains.
Other domains, such as Radio, can retain the CPU and local peripherals in hardware flip-flops in the *System ON (All) Idle* state.

In this state, the entire local Active Power Domain, including the CPU, is unpowered.
The state of the CPU and the peripherals powered by the local Active Power Domain is not retained by hardware flip-flops.
Instead, their state is retained by software in RAM.

.. list-table::
   :widths: auto

   * - CPU
     - Unpowered.
       Its state is retained in RAM.

   * - RAM
     - Enabled or retained depending on the hardware state.

   * - System state
     - * *System ON* if at least one other CPU or a peripheral clocked greater than 32 kHz is active.
       * *System ON Idle* if all CPUs are disabled and the only active peripherals are clocked with 32 kHz (real-time part of ``GRTC``, ``RTC``, ``WDT``) or System Off wake-up sources.
       * *System ON All Idle* if all other CPUs and all peripherals except System Off wake-up sources are disabled.

       The hardware automatically selects one of these states based on the activity of other CPUs and peripherals.

   * - Peripherals
     - * Powered by the local Active Power Domain must be disabled.
       * Powered by any other power domain can be active.

       The state of the inactive peripherals located in other power domains is retained in the hardware flip-flops.

       It is recommended to use only 32 kHz clocked peripherals in this state to allow entering *System ON Idle*.
       One example could be using GPIO as CSN to wake up the system and enable an SPIS peripheral only after the system is woken up.

   * - Coprocessors
     - Global can be active.
       There are no local coprocessors in the domains supporting this sleep state.
       The state of inactive coprocessors is retained according to their selected sleep state.

   * - System timer (based on GRTC)
     - Active

The power consumption in this state depends on the overall System state but is lower than in any of the *Idle* states.
The wake-up latency is higher than in any of the *Idle* states due to the CPU state restoration procedure.

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

Power management benchmark
**************************

To benchmark the power consumption in *Idle* state, see :ref:`multicore_idle_test`.

Power management optimization example
*************************************

This example focuses on a HID device use case (for example, a mouse)

A mouse has tipically two main application states:

* Active, when the user is moving the mouse.
* Idle, triggered after an inactivity timeout until an input from the user is detected.

The following is an example workflow for the *Active* application state:

  1. Sample input data from sensors.
  2. Construct a HID report.
  3. Transmit the report over the radio.
  4. Sleep until the next period (usually, less than 1 ms).

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

Recommended Kconfig configuration
=================================

Set these Kconfigs as follows for the application running on the application core:

  * CONFIG_PM=y
  * CONFIG_PM_S2RAM=y
  * CONFIG_POWEROFF=y
  * CONFIG_PM_S2RAM_CUSTOM_MARKING=y
  * CONFIG_PM_DEVICE=y
  * CONFIG_PM_DEVICE_RUNTIME=y

Set these Kconfigs as follows for the application running on the radio core:

  * CONFIG_PM=y
  * CONFIG_POWEROFF=y
  * CONFIG_PM_DEVICE=y
  * CONFIG_PM_DEVICE_RUNTIME=y

Consider also the following:

  * Disable all unused peripherals before entering sleep (Zephyr's API does this automatically when supported).
  * Add `zephyr,pm-device-runtime-auto` in the DTS for all peripherals with runtime PM support.
    ## DTS to JSON update here
  * Build and program an empty image on any unused core to release shared resources.
    ##sample of this empty image?

##TODO

Deep-sleep policy
-----------------

Minimum sleep durations:

  * Suspend-to-Idle: ≥ 1 ms
  * Suspend-to-RAM: ≥ 2 ms

Use policy locks to prevent premature deep-sleep entry:

.. code-block:: c

   /* In active mode: lock deep states */
   pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
   pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);

   /* Before going to sleep: unlock */
   pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
   pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);

Memory and cache optimizations
------------------------------

  * Relocate frequently accessed code and data to local RAM to avoid waking global domains, especially MRAM, which uses high power.
  * Profile L1/L2 cache usage to minimize MRAM accesses.
  * Disable MRAM latency manager (`CONFIG_MRAM_LATENCY=n`) to power MRAM off when idle.

Peripheral and clock recommendations
------------------------------------

  * Use global peripherals from the slow domain (peripheral index 13X) to avoid waking up the fast global domain, which uses more power.
  * If radio is active frequently, keep HFXO enabled (wake-up ≈ 800 µs).
  * For SPI, consider nRFy or nrfx libraries instead of Zephyr’s blocking driver to save ~30 µA.

Single-core vs. dual-core
-------------------------

  * A single-core solution (radio core only) can reduce IPC overhead (~0.15 mA at 1 kHz).
  * Larger TCM on the radio core allows more code to run off-MRAM.
  * Always load an empty image on the unused core to free global resources allocated to this core by the System Controller on initialization.

Additional considerations
-------------------------

  * Apply HMPAN-216 workaround: enable the 0.8 V rail 40 µs before every radio RX/TX.
  * Test with the latest SoC bundle to benefit from all fixes and improvements.


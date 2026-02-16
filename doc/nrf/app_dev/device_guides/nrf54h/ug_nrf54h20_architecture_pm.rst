.. _ug_nrf54h20_architecture_pm:

nRF54H20 power management
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

The ARM Cortex CPUs of the nRF54H20 SoC currently support the following software power states:

* Active
* Idle
* Idle with cache disabled
* Local suspend to RAM

Each CPU in the nRF54H20 SoC tries to preserve as much power as possible, independently from other CPUs.
The power management subsystem, operating independently within each CPU, continuously selects the most optimal power state based on the current conditions of the CPU.

The following sections describe the details of each of the software power states available on the nRF54H20 SoC.

Active
======

*Active* is the power state in which the CPU is actively processing.

.. list-table::
   :widths: auto

   * - CPU
     - Powered on and clocked.

   * - RAM
     - Banks used by the executed code are enabled.
       Other banks can be in any state.

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
       Other banks can be in any state.

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

Idle with cache disabled
========================

*Idle with cache disabled* is a lightweight sleep state where the CPU is suspended and the contents of the L1 caches are discarded.

.. list-table::
   :widths: auto

   * - CPU
     - Powered on but not clocked.
       Its state is retained in the hardware flip-flops.

   * - RAM
     - Enabled or retained depending on the hardware state.
       RAM in caches is disabled.

   * - System state
     - * *System ON* if at least one other CPU or a peripheral clocked > 32 kHz is active.
       * *System ON Idle* if all CPUs are disabled and the only active peripherals are clocked with 32 kHz (real-time part of ``GRTC``, ``RTC``, ``WDT``) or System Off wake-up sources.
       * *System ON All Idle* if all other CPUs and all peripherals except System Off wake-up sources are disabled.
         Only available in domains without DVFS.

       The hardware automatically selects one of these states based on the activity of other CPUs and peripherals.

   * - Peripherals
     - All can be active.
       The local peripherals are powered and preserve their state.
       The state of inactive peripherals in other domains is retained in the hardware flip-flops.

   * - Coprocessors
     - All can be active.
       The state of inactive coprocessors is retained according to their selected sleep state.

   * - System timer (based on GRTC)
     - Active

The primary sources of wake-up latency are:

* Writing back dirty cache lines
* Waking up the MRAMC
* Refilling L1 caches
* Restarting clock sources

Latency is influenced by the number of dirty cache lines at sleep entry and the cache miss rate after wake-up.
The maximum latency depends on the MRAM power state configured by the System Controller via the ``MRAMC.POWER.AUTOPOWERDOWN`` setting.

Local suspend to RAM
====================

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

Mapping application states to software power states
***************************************************

The following table maps application states to software power states:

+---------------------+------------------------------+---------------------------------------+------------------------------------------+
| Application state   | Application core power state | Radio core power state                | SoC hardware state                       |
+=====================+==============================+=======================================+==========================================+
| Active              | Active,                      | Active,                               | System On,                               |
|                     | Idle,                        | Idle,                                 | System On Idle,                          |
|                     | Local suspend to RAM         | Idle with cache disabled              | System On All Idle                       |
+---------------------+------------------------------+---------------------------------------+------------------------------------------+
| Idle                | Local suspend to RAM         | Idle with cache disabled              | System On All Idle                       |
+---------------------+------------------------------+---------------------------------------+------------------------------------------+

Power management optimization
*****************************

For recommendations on power management optimization techniques on the nRF54H20 SoC, see the :ref:`ug_nrf54h20_pm_optimization` page.

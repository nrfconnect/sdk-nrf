.. _ug_nrf54h20_pm_optimization:

nRF54H20 power management optimization
######################################

.. contents::
   :local:
   :depth: 2

This guide provides practical tips for optimizing power management on the nRF54H20 SoC.
It is organized into two key sections:

* An overview, containing a brief summary of optimal state selection, and latency management.
* An optimization example, based on a practical mouse HID use case.

For more details on the power management architecture, see :ref:`ug_nrf54h20_architecture_pm`.
For general information on power management in the |NCS|, see the :ref:`app_power_opt_recommendations` page.

Overview
********

This section introduces the core principles of power management on the nRF54H20 SoC.


Optimal power state selection
=============================

In the nRF54H20 SoC, each local domain is responsible for selecting the power state that results in minimal power consumption while maintaining an acceptable level of performance.

Entering a deeper sleep state leads to power savings when the system is idle, but entering and exiting the sleep state increases the power consumption.
There is a minimum sleep duration that justifies the energy spent on entering and exiting a sleep state, and this duration varies for each sleep state.

In the SoC, a local domain has full control over entering and exiting local sleep states, allowing it to assess whether entering these sleep states is optimal at any given moment.
However, entering sleep states associated with system-off requires cooperation between local domains and the System Controller.
Local domains have limited control over the time and energy required to enter or exit system-off, as well as the power consumption during system-off.

Latency management
==================

The sources of wake-up latency in the nRF54H20 SoC can be categorized into local and global types.
Each CPU is responsible for managing its latency sources, with local sources handled by local domains and global sources managed by the System Controller.

Local cores are responsible for handling latencies caused by restoring the system from suspend-to-RAM states.
They schedule their wake-up before expected events.
The timing of expected events is reported to the power management subsystem in the RTOS by the software modules anticipating these events.
The power management subsystem sets a ``GRTC`` channel in advance of the next expected event to compensate for local wake-up latency.

The System Controller is responsible for handling latencies caused by restoring the system from the system-off state (the warm boot procedure latency).
It schedules the system wake-up from the system-off state in advance of the next ``GRTC`` event to compensate for the warm-boot latency of the system.

Because the warm-boot latency is compensated by the System Controller, from a local CPU's perspective, the latency when restoring from the local-off state and the system-off state is expected to be the same.

Latency in local domains
------------------------

Any local software module (such as a device driver) can anticipate events like ISRs.
Some of these events have predictable timing, while others have unpredictable timing.
Handling the latency of events with unpredictable timing is the same in both simple and complex systems.

If handling an event with predictable timing requires restoring the state of the software module or the peripherals used by this module before the event is processed, the software module is responsible for scheduling a timer event in advance.
This scheduled event is used to restore the state of the software module or peripherals.

The power management subsystem in a local domain is responsible for scheduling a wake-up in advance to compensate for the domain's core state restoration latency from the local power state.
The wake-up time scheduled in advance by the power management subsystem is combined with the advance time added by the software module.
This approach ensures that the local domain and the software modules anticipating an event have sufficient time to fully restore before the event occurs, allowing the event to be handled without latency.

Logical domains, power domains, and locality
============================================

The nRF54H20 SoC groups hardware into logical domains and power domains.
The two domain types are defined as follows:

Logical domain
  A software-visible functional grouping that aggregates cores, memories, and peripherals behind a common isolation or security boundary (for example, application domain, radio domain, global domain).

Power domain
  A granular silicon region that can be clock- or power-gated independently.
  Each physical silicon region has its own retention and leakage characteristics as well as voltage and frequency constraints (for example, high-speed instead of low-leakage subregions inside the application logical domain).

A single logical domain can span multiple power domains.
A logical domain frequently contains a low-leakage power domain (cheap retention, low frequency) and one or more high-speed power domains (higher leakage, higher attainable frequency).
DVFS applies only to high-speed domains.

Locality principle
------------------

Power optimization is driven by keeping execution and data access local:

* Prefer executing code from local RAM (TCM/local SRAM) within the active core's logical domain.
* Minimize accesses to MRAM (global non-volatile memory) to avoid waking other domains and incurring cache refill energy.
* Batch work: Gather peripheral data locally (auxiliary cores) and hand off larger buffers to high-performance cores for computation, instead of driving peripherals directly from high-performance cores.

Peripheral access strategy
--------------------------

High-performance cores (application, radio main CPUs) are optimized for computation, not low-latency I/O paths.
Accessing peripherals from these cores expands the active footprint by adding additional power domains and interconnected segments, increasing energy for each transaction.

Recommendations:

* Use auxiliary or peripheral-oriented cores (for example, PPR- or FLPR-type helper cores) to perform SPI, I2C, ADC, or UART transactions, local preprocessing, and DMA into local RAM.
* Signal (IPC or shared memory flag) the application or radio core only when a batch of data is ready.
* Avoid fine-grained peripheral polling from high-performance cores.
  Use the following instead:

  * DMA transfers initiated by auxiliary cores.
  * Hardware-trigger chains (PPI/DPPI) where possible to reduce wake-ups.

* Keep high-speed power domains off unless necessary for bursts of compute.

DMA locality constraints
------------------------

Local DMA controllers are restricted to source/destination addresses within their local RAM range.
Plan buffer placement accordingly:

* Cross-domain transfers should use the following:

  * IPC messages indicating buffer readiness.
  * Explicit software copy (only for infrequent cases).

Local and global wake-up planning
---------------------------------

On local domain wake-ups (suspend-to-RAM/suspend-to-idle), schedule GRTC events early enough to cover the following latencies:

  * Core restore latency (cache, context).
  * Software module restore latency (driver reconfiguration).

Integration checklist
---------------------

For optimal power management integration, follow these guidelines:

* Place time-critical ISR code and frequently used data structures in local RAM sections (see code/data relocation).
* Profile cache miss sources.
  Reduce MRAM fetches by relocating hot code paths.
* Disable MRAM latency manager (set :kconfig:option:`CONFIG_MRAM_LATENCY` to ``n`` and :kconfig:option:`CONFIG_MRAM_LATENCY_AUTO_REQ` to ``n``) to allow MRAM power gating.
  You can set :kconfig:option:`CONFIG_MRAM_LATENCY` to ``y`` to allow requesting low latency MRAM when needed.
  However, avoid setting :kconfig:option:`CONFIG_MRAM_LATENCY_AUTO_REQ` to ``y``, as it causes MRAM to always remain in low latency/higher current mode.
* Ensure DVFS settings are evaluated across workloads.
  Higher frequencies often yield best energy/operation for periodic burst tasks (quick wake, work, sleep), while lower or middle frequencies are better suited for long-running continuous workloads.
* Consolidate periodic tasks (sensor sample, radio tick) to a shared 1 ms or coarser schedule to avoid fragmented wake-ups.

.. note::
   The exact wake-up source list can evolve with silicon revisions.
   Always verify against the current SoC datasheet and release notes.

Optimization example
********************

This example focuses on a HID device use case (for example, a mouse).

A mouse has typically two main application states:

* Active, when the user is moving the mouse.
* Idle, triggered after an inactivity timeout until an input from the user is detected.

The following is an example workflow for the *Active* application state:

1. Sample input data from sensors.
#. Construct a HID report.
#. Transmit the report over the radio.
#. Sleep until the next period (usually, less than 1 ms).

Power consumption targets
=========================

The power consumption targets of this sample for sleep currents (5V supply) are the following:

+---------------------------------------------+----------------------------+
| Component                                   | Sleep current (5V supply)  |
+=============================================+============================+
| Application core (Suspend-to-RAM)           | < 10 µA                    |
+---------------------------------------------+----------------------------+
| Radio core (Suspend-to-Idle, cache disabled)| < 10 µA                    |
+---------------------------------------------+----------------------------+

The power consumption targets of this sample for active currents (estimates at 5V) are the following:

+--------------------------------------------+----------------------+
| Activity                                   | Active current (mA)  |
+============================================+======================+
| Radio core in idle                         | ~0.25                |
+--------------------------------------------+----------------------+
| App core in idle                           | ~0.15                |
+--------------------------------------------+----------------------+
| Core wake-up every 1 ms                    | +0.05                |
+--------------------------------------------+----------------------+
| ADC sample every 1 ms                      | +0.10                |
+--------------------------------------------+----------------------+
| SPI transaction every 1 ms                 | +0.20                |
+--------------------------------------------+----------------------+
| IPC message every 1 ms                     | +0.15                |
+--------------------------------------------+----------------------+

.. note::
   The SPI current can be reduced to 0.03 mA by using the nrfy API instead of the default Zephyr blocking driver.

Recommended Kconfig configuration
=================================

For applications running on either the application core or the radio core, set the following Kconfig options:

  * :kconfig:option:`CONFIG_PM` to ``y``
  * :kconfig:option:`CONFIG_POWEROFF` to ``y``

These options automatically enable :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`, which is required to manage the energy states of individual devices.

Consider also the following recommendations:

  * Disable all unused peripherals before entering sleep.
    When supported, Zephyr's API automatically handles this by using the PM device runtime.
  * Build and program an empty image on any unused core to release shared resources.
  * If one or more specific sleep states are not desired, disable them in the devicetree by setting their status to ``disabled``:

    .. code-block:: dts

       &s2ram {
               status = "disabled";
       };

.. _ug_nrf54h20_pm_optimizations_bootloader:

Operation with MCUboot as the bootloader
========================================

Suspend to RAM (S2RAM) operation of the application requires special support from the bootloader.

MCUboot on the nRF54H20 SoC supports Suspend to RAM (S2RAM) functionality in the application.
It can detect a wake-up from S2RAM and redirect execution to the application's resume routine.

To enable S2RAM support for your project, set the :kconfig:option:`CONFIG_SOC_EARLY_RESET_HOOK` MCUboot Kconfig option.
This option integrates the S2RAM resume bridge into the start-up code.

Also ensure that your board's DTS file includes the following Zephyr nodes, which describe the linker sections used:

* a ``zephyr,memory-region`` compatible node labeled ``mcuboot_s2ram``, with a size of 8 bytes, used for placing MCUboot's S2RAM magic variable.
* a ``zephyr,memory-region`` compatible node labeled ``pm_s2ram_stack``, with a size of 16 bytes.
  This region is used as the program stack by MCUboot during S2RAM resume.

Example DTS snippet:

.. code-block:: dts

   / {
      soc {
        /* run-time common mcuboot S2RAM support section */
        mcuboot_s2ram: cpuapp_s2ram@22007fd8 {
           compatible = "zephyr,memory-region", "mmio-sram";
           reg = <0x22007fd8 8>;
           zephyr,memory-region = "mcuboot_s2ram_context";
        };

        /* temporary stack for S2RAM resume logic */
        pm_s2ram_stack: cpuapp_s2ram_stack@22007fc0 {
           compatible = "zephyr,memory-region", "mmio-sram";
           reg = <0x22007fc0 16>;
           zephyr,memory-region = "pm_s2ram_stack";
        };
      };
   };

Memory and cache optimization recommendations
=============================================

The following recommendations help optimize memory placement and cache usage to minimize power consumption:

* Relocate frequently accessed code and data to local RAM to avoid waking global domains, especially MRAM, which uses high power.
  For more information, see the :ref:`zephyr:code_data_relocation` page.
* Execute the radio core image from Tightly Coupled Memory (TCM).
* Profile L1 cache usage to minimize MRAM accesses.
  For more information, see ``nrf_cache_hal`` in the `nrfx API documentation`_.
* Configure the MRAM latency manager:

  * :kconfig:option:`CONFIG_MRAM_LATENCY`:

    * ``n`` (default) allows MRAM to power off when idle.
    * ``y`` allows requesting low latency MRAM when needed.

  * :kconfig:option:`CONFIG_MRAM_LATENCY_AUTO_REQ` set to ``n`` prevents MRAM from always being in low latency/higher current mode.

Execute radio code from TCM
---------------------------

You can configure the radio core firmware to execute from the Tightly Coupled Memory (TCM), which is the radio core local RAM.

Accessing TCM consumes less energy and provides faster execution than accessing MRAM.
This configuration can improve application energy efficiency, depending on the amount of radio traffic in your application.
Higher radio usage results in more significant power savings.
However, if your application keeps the application core active for significantly longer periods than the radio core, the power savings from executing the radio core firmware from TCM might be negligible.
In this case, MRAM remains powered regardless of the radio core configuration.
Therefore, executing the radio core firmware from TCM provides no additional benefit.
To enable code execution from TCM on the nRF54H20 radio core, follow the steps as described in detail in the :ref:`idle_relocated_tcm_sample` sample.
The sample demonstrates how to configure the memory map in the devicetree, how to use :ref:`zephyr:sysbuild` to add the ``radio_loader`` image, and how this configuration integrates with MCUboot.
When you configure your application to execute the radio firmware from the TCM, the build system generates two images for the radio core:

* The ``radio_loader`` image.
* The radio core firmware image, built with the :kconfig:option:`CONFIG_XIP` option set to ``n``, linked against the TCM partition, and relocated to MRAM using the :kconfig:option:`CONFIG_BUILD_OUTPUT_ADJUST_LMA` option.

The linker places both images adjacent to each other.
At application boot, the radio core first executes the radio loader firmware.
The radio loader copies the radio core firmware to TCM and then jumps to the boot address of the copied image.
If your application uses the MCUboot bootloader in *Direct-XIP* mode, you can relocate the radio core firmware to TCM by using either merged-slot mode or manifest mode.
In both cases, the build system merges the radio loader and the radio core firmware images into a single image.
The bootloader then interprets them as a single image.

.. note::
    To ensure that the radio core executes code from TCM, the entire image, including all data sections, must fit within the local RAM region of the radio core (192 KB).

Peripheral and clock recommendations
====================================

The following guidelines help optimize peripheral and clock usage to minimize power consumption:

* Use global peripherals from the slow domain (peripheral index 13X) to avoid waking up the fast global domain, which uses more power.
* If radio is frequently active, keep HFXO enabled (wake-up ≈ 800 µs).
* For SPI, consider using nrfy or nrfx libraries instead of Zephyr's blocking driver to save ~170 µA.
  Using nrfy has the following advantages:

  * Non-blocking transfers without mandatory callbacks, reducing wake-ups.
  * Reuse of a constant TX buffer when sending the same data, avoiding repeated updates.

DVFS on application core
------------------------

The application core supports Dynamic Voltage and Frequency Scaling (DVFS), offering three distinct frequency options.
Higher frequencies typically provide the best power efficiency for periodic burst workloads (minimizing wake time to maximize sleep), while lower or middle frequencies are better suited for long-running continuous tasks where the CPU remains active.
It is recommended to test each setting to determine the optimal choice for your specific use case.

Reduction of wake-ups
---------------------

Waking up the radio core at a 1-ms interval consumes approximately 50 µA of average current.
To minimize power consumption, design your application to avoid frequent wake-ups by synchronizing events, such as sensor sampling, or by using the PPI to trigger tasks without CPU intervention.

Single-core and dual-core considerations
========================================

When choosing between single-core and dual-core architectures (using either the application core, the radio core, or both), consider the following trade-offs:

* A single-core solution (radio core only) can reduce IPC overhead (~0.15 mA at 1 kHz).
* The larger local RAM on the radio core (compared to the application core) allows more code to run off the MRAM.
* Always load an empty image on the unused core to free global resources allocated to this core by the System Controller on initialization.
  You can refer to ``benchmarks.multicore.idle.nrf54h20dk_cpuapp_cpurad.s2ram`` contained in the :file:`sdk-nrf/tests/benchmarks/multicore/idle/testcase.yaml` test file.
  To create an empty image, use the following code:

  .. code-block:: c

      int main()
      {
        return 0;
      }

Deep-sleep policy and latency optimization
==========================================

Most peripherals do not schedule an expected wake-up event, which can cause Zephyr's power manager to enter and almost immediately exit a deep sleep state.
This consumes unnecessary energy due to the overhead of putting the SoC to sleep and waking it up again, where entering a lighter sleep state would have been more efficient.

The minimum sleep durations that justify entering a deep sleep state are defined in the devicetree:

* Suspend-to-Idle: ≥ 1 ms
* Suspend-to-RAM: ≥ 2 ms

For sleep durations shorter than these thresholds, an idle sleep state is more efficient.

.. code-block:: devicetree

   idle_cache_disabled: idle_cache_disabled {
      compatible = "zephyr,power-state";
      power-state-name = "suspend-to-idle";
      substate-id = <2>;
      min-residency-us = <1000>;
      exit-latency-us = <7>;
   };

   s2ram: s2ram {
      compatible = "zephyr,power-state";
      power-state-name = "suspend-to-ram";
      min-residency-us = <2000>;
      exit-latency-us = <33>;
   };

To prevent the system from entering a deep sleep state prematurely, the application must inform Zephyr when the next wake-up event is expected to occur.
The following sections describe different approaches for managing sleep states and latency.

Scheduling expected wake-up events
----------------------------------

Using ``pm_policy_event_register()``, the application can define the exact time when the next wake-up event is expected to occur.
Zephyr uses this information, along with other registered events (including ``k_timer`` timeouts and when sleeping threads are expected to wake up), to prevent entering a sleep state that is too deep.
The ``min-residency-us`` values in the devicetree define the minimum sleep durations required to justify entering each sleep state.

This approach provides optimal power management for all expected wake-up events, with no notable impact on latency since Zephyr takes the ``exit-latency-us`` wake-up latency into account when selecting a sleep state.

However, unexpected wake-up events, such as a button press, will have unpredictable latency since the system may be in deep sleep when the event occurs.

Ensuring consistent latency for unexpected events
-------------------------------------------------

To ensure consistent latency for unexpected events, use the ``pm_policy_latency_request_add()`` API.
This allows the application to define the maximum acceptable latency at any given time, which corresponds to the ``exit-latency-us`` wake-up time from the power sleep state.

For example, the application can request a maximum latency of 7 microseconds, which prevents the CPU from entering the ``s2ram`` state (with its 33 µs exit latency) in the previously mentioned devicetree example, regardless of when the next event is scheduled.

Using policy locks
------------------

For simpler use cases, you can use policy locks to disable deep-sleep states when the application is expected to run again in a short period (for example, at a 1 kHz rate).
By acquiring a policy lock, you prevent the system from entering specific sleep states.
This can save up to 0.5 mA.

.. code-block:: c

   /* In active mode: lock deep states */
   pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
   pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM,   PM_ALL_SUBSTATES);

   /* Before going to sleep: unlock */
   pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
   pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM,   PM_ALL_SUBSTATES);

Additional considerations
=========================

When implementing these recommendations, consider also the following:

* When using a custom radio protocol, apply the HMPAN-216 workaround, enabling the 0.8V rail 40 µs before every radio RX and TX operation.

  .. note::
     This workaround is already implemented for Bluetooth LE and Enhanced ShockBurst (ESB).

* Test with the latest nRF54H20 SoC bundle to benefit from all the latest fixes and improvements.
  For more information, see :ref:`abi_compatibility`.

Power management benchmark
**************************

To benchmark the power consumption in *Idle* state, see :ref:`multicore_idle_test`.

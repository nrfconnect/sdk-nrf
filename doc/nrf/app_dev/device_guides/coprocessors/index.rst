.. _coprocessors_index:

Developing with coprocessors
############################

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15, nRF54LM20, nRF54LC10A, and nRF54LV10A devices.

VPR coprocessors can be primarily used to perform one of the following tasks:

* As an additional core in a multicore system using Zephyr in multithreaded mode (see the :ref:`nRF54L<vpr_flpr_nrf54l>` and :ref:`nRF54H20 devices<ug_nrf54h20_flpr>` pages).
  In this mode, the VPR core acts as a standard CPU and can be used for either of the following purposes:

  * Extra performance - Move part of the application to the FLPR (Fast Lightweight Processor) core, or use it to offload compute-heavy tasks from the application core, to increase the overall processing capacity available to the application.
  * Power optimization - Move simple, low-duty tasks to a VPR core so that the application core can stay idle or enter a low-power state for longer periods.
    This is the primary use case for the PPR (Peripheral Processor) core.
    Because VPR cores execute code from RAM, they do not need to wake up non-volatile memory (NVM); in addition PPR can runs in a low-power domain, which can significantly reduce power consumption compared to running the same task on the application core.
    For an example, see the :ref:`vpr_offloading_sample` sample.

* As a peripheral emulator on the FLPR core, using one of the following methods depending on the use case:

  * :ref:`High-Performance Framework (HPF)<hpf_index>`
  * :ref:`nrfxlib:soft_peripherals`

  .. note::
     In these usage modes, it is important to clearly differentiate between the Soft Peripheral and HPF solutions.
     Soft Peripherals serve as a direct replacement for hardware peripherals, offering guaranteed performance.
     In contrast, the HPF allows you to accelerate protocol operations, but performance depends on your implementation.
     Nordic Semiconductor recommends using the Soft Peripheral solution if it meets your product's requirements.

Peripheral emulation is only supported on the FLPR core and is not available on PPR.
Both HPF and Soft Peripherals rely on FLPR's :ref:`real-time peripherals<hpf_real_time_peripherals>`, namely VPR IO (VIO) and VPR TIMER (VTIM), which are specifically designed to provide the cycle-accurate GPIO control and timing required to emulate serial communication peripherals in software.

The following table outlines the main differences between the usage modes.
For detailed comparison see the :ref:`ug_hpf_softperipherals_comparison` page.

.. list-table:: Main differences between usage modes
   :header-rows: 1

   * - Comparison category
     - Zephyr application
     - High-Performance Framework (HPF)
     - Soft Peripherals
   * - Overview
     - Comprehensive Zephyr application with full feature access.
     - Bare-metal build from source featuring real-time I/O capabilities.
     - Custom binary that emulates a hardware peripheral.
   * - Advantages
     - Full access to Zephyr's capabilities including drivers, libraries, and OS primitives.
     - Enables custom protocol support with optimized execution latency and minimized code footprint.
     - - Pre-validated product, fully compliant with the simulated hardware peripheral specifications.
       - Compatible with higher layer driver.
   * - Limitations
     - Larger code size and higher execution latency.
     - Requires development of custom code.
     - Provided as a binary making modifications impossible.
   * - Build system
     - Built from source using Zephyr’s sysbuild.
     - Built from source using Zephyr’s sysbuild.
     - Custom HEX file loaded by the application core.
       VPR is exposed to build system.
       Only the GPIO ports and pins utilized by the VPR are configured within an overlay.
   * - Inter-processor communication
     - Zephyr’s IPC service or mbox API.
     - Zephyr’s IPC service or mbox API.
     - Managed through the peripheral driver API.
   * - Work offloading
     - Supports any task, including those requiring Zephyr libraries.
     - Handles simple data pre-processing or post-processing based on specific protocol needs.
     - Not supported.
   * - Maturity level
     - Experimental (nRF54L15, nRF54LM20, nRF54LC10A, nRF54LV10A, and nRF54H20)
     - Experimental (nRF54L15, nRF54LM20, nRF54LC10A, and nRF54LV10A)
     - Supported (see :ref:`nrfxlib:soft_peripherals` documentation for the list of supported devices)
   * - Example use case
     - - Uses VPR as a standard CPU to run part of the application or to offload compute-heavy tasks from the application core, increasing overall processing capacity.
       - Uses VPR (typically PPR) to run simple background tasks with minimal power consumption (see the :ref:`vpr_offloading_sample` sample).
     - Develops custom protocol emulators.
     - Replaces conventional hardware peripherals.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   ug_hpf_softperipherals_comparison.rst
   hpf.rst

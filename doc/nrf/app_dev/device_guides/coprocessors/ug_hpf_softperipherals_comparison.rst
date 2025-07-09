.. _ug_hpf_softperipherals_comparison:

Introduction to High-Performance Framework and Soft Peripherals
###############################################################

.. contents::
   :local:
   :depth: 2

:ref:`nrfxlib:soft_peripherals` and the :ref:`High-Performance Framework (HPF)<hpf_index>` both facilitate the creation of software-based peripherals, that emulate the functionality of physical hardware peripherals through software.

Choosing between the solutions depends on the specific requirements of your project and the resources available.

Purpose
*******

Soft Peripherals are designed to emulate digital hardware peripherals (IP) on a RISC-V coprocessor, named Fast Lightweight Peripheral Processor (FLPR).
They can be used in situations where hardware peripherals are absent or insufficient.

HPF, on the other hand, enables the development of custom, software-defined peripherals.
It enhances the main processor's functionality with real-time capabilities in a streamlined, baremetal environment, integrating with the Zephyr build system.
HPF can also offload parts of the protocol stack to the FLPR core.

.. _nrf54l_hpf_softperi_comparison_use_case:

Implementation and use cases
============================

The following comparison details implementation and typical use cases for Soft Peripherals and High Performance Framework:

.. list-table::
   :header-rows: 1

   * - Aspect
     - Soft Peripherals
     - High Performance Framework
   * - Development
     - Developed by Nordic Semiconductor, optimized for performance and power consumption.
     - Allows for the creation of specialized or custom peripherals.
   * - Use cases
     - When an application requires an extra or absent peripheral and a corresponding Soft Peripheral is available.
     - When custom peripheral development or a high-level control is needed.
   * - Suitability
     - Suitable for applications built from pre-compiled sources.
     - Ideal for applications where the FLPR handles higher layers of the software stack.

.. _nrf54l_hpf_softperi_comparison_requirements:

Requirements
************

For Soft Peripherals, there is a specific memory size requirement of approximately 16K, but there is a flexibility in placement within the FLPR execution RAM.
To enter low power consumption modes, Soft Peripherals require a slot at a specific memory address, which is platform-specific, and the application code must be able to access MEMCONF registers.
You can check these requirements in the :ref:`Soft Peripherals section<nrfxlib:soft_peripherals>`, which provide essential information for proper integration and optimization.

On the other hand, HPF necessitates that the application core uses the Zephyr operating system.
The framework allows for a high degree of configurability, which you can tailor according to your needs using Kconfig and devicetree settings.
This flexibility allows you to optimize the memory footprint and functionality of the custom peripherals, ensuring that the peripherals are well-integrated and perform efficiently within their specific application environments.

.. _nrf54l_hpf_softperi_comparison_features:

Features comparison
*******************

See the following detailed feature comparison between Soft Peripherals and High-Performance Framework:

.. list-table::
   :header-rows: 1

   * - Feature
     - Soft Peripherals
     - High Performance Framework
   * - Integration
     - Pre-compiled binary for FLPR, driver for Application Core
     - Built from source in Zephyr build system
   * - Memory requirements
     - Fixed (~16K)
     - Adjustable, depends on code and devicetree
   * - Application Compatibility
     - Can be used on baremetal or Zephyr applications
     - Uses Zephyr without kernel on FLPR side, full Zephyr on APP side
   * - Integration complexity
     - Minor integration required
     - Samples provided for the start of development
   * - IPC mechanism
     - Register-based
     - ic(b)msg or mbox-based
   * - Compliance & testing
     - Protocol-compliant, verified by Nordic Semiconductor
     - Samples provided as-is, testing is done by the user
   * - Delivery form
     - Binary
     - Source code, modifiable and extendable
   * - API level
     - Exposed at HW-driver level
     - FLPR may handle parts of SW stack above HW-driver level

.. _nrf54l_hpf_softperi_comparison_creating_peripherals:

Creation of software-based peripherals
**************************************

For Soft Peripherals, the integration process primarily involves incorporating glue code to facilitate their use.
To learn how to use those peripherals, and to see what is currently supported, refer to the :ref:`nrfxlib:soft_peripherals` documentation.

In contrast, HPF provides initial samples that serve as a starting point for development.
For detailed guide on creating your own custom peripherals, see the :ref:`hpf_index` page.

.. _nrf54l_hpf_softperi_comparison_limitations:

Performance and limitations
***************************

Each Soft Peripheral is unique and comes with its own set of limitations compared to traditional hardware IP.
These limitations are specific to the functions that the peripheral is designed to emulate and how they integrate with the rest of the system.

HPF, while offering extensive customization and control, is currently in an experimental stage and lacks full Power Management support.

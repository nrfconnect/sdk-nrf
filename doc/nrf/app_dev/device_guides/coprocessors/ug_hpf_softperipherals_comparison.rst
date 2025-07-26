.. _ug_hpf_softperipherals_comparison:

Introduction to Soft Peripherals and High-Performance Framework
###############################################################

.. contents::
   :local:
   :depth: 2

:ref:`nrfxlib:soft_peripherals` are a collection of pre-compiled binaries and API driver code designed to emulate commonly used peripherals.
These peripherals allow application code to use the API driver to load the pre-compiled binaries onto a specific RAM location, where they are executed by the RISC-V coprocessor, named Fast Lightweight Peripheral Processor (FLPR) (see the :ref:`nRF54L15<vpr_flpr_nrf54l>` and :ref:`nRF54H20 devices<ug_nrf54h20_flpr>` pages).
The application can then control the soft peripherals using API functions.

The :ref:`High-Performance Framework (HPF)<hpf_index>` is a framework designed to support the development and integration of software peripherals using coprocessors.
It offers tools such as Hardware Abstraction Layers (HALs) and CMake targets, along with application samples and guidelines on architectural issues like event, fault, and power management.
These resources will help you create customized software peripherals tailored to specific application needs.

Design rationale and usage
**************************

Both methods facilitate the creation of software-defined peripherals that emulate the functionality of physical hardware peripherals (IP) through software.
Choosing between the solutions depends on the specific requirements of your project and the resources available.

.. _nrf54l_hpf_softperi_comparison_use_case:

Implementation and use cases
============================

The following comparison details implementation and typical use cases for Soft Peripherals and High Performance Framework:

.. list-table::
   :header-rows: 1

   * - Comparison aspect
     - Soft Peripherals
     - High Performance Framework
   * - Development
     - Optimized for performance and power consumption.
     - Allows for the creation of specialized or custom peripherals.
   * - Use cases
     - - When an application requires an additional or unavailable peripheral and a corresponding Soft Peripheral is available.
       - Suitable for applications built from pre-compiled sources.
     - - When developing a custom peripheral or when high-degree control is needed.
       - Enhances the main processor's functionality with real-time capabilities in a streamlined, bare metal environment.
       - Integrated with the Zephyr build system.
       - Ideal for applications where the FLPR handles higher layers of the software stack (it allows to offload parts of the protocol stack to the FLPR core).

.. _nrf54l_hpf_softperi_comparison_features:

Features comparison
===================

See the following detailed feature comparison between Soft Peripherals and High-Performance Framework:

.. list-table::
   :header-rows: 1

   * - Feature
     - Soft Peripherals
     - High Performance Framework
   * - Integration
     - Pre-compiled binary for FLPR, driver for the application core
     - Built from source in Zephyr build system
   * - Memory requirements
     - Fixed (~16 K)
     - Adjustable, depends on code and devicetree
   * - Application compatibility
     - Can be used on baremetal or Zephyr applications
     - Uses Zephyr without kernel on FLPR side, full Zephyr on APP side
   * - Integration complexity
     - Minor integration required
     - Samples provided for the start of development
   * - IPC mechanism
     - Register-based
     - ic(b)msg or mbox-based
   * - Compliance and testing
     - Protocol-compliant, verified by Nordic Semiconductor
     - Samples provided as-is, testing is done by the user
   * - Delivery form
     - Binary
     - Source code, modifiable and extendable
   * - API level
     - Exposed at hardware driver level
     - FLPR may handle parts of the software stack above hardware driver level

.. _nrf54l_hpf_softperi_comparison_requirements:

Requirements
************

For Soft Peripherals, there is a specific memory size requirement of approximately 16 K, but there is a flexibility in placement within the FLPR execution RAM.
To enter low power consumption modes, Soft Peripherals require a slot at a specific memory address, which is platform-specific, and the application code must be able to access MEMCONF registers.
See the requirements in the :ref:`Soft Peripherals section<nrfxlib:soft_peripherals>`.
It provides essential information for proper integration and optimization.

On the other hand, HPF necessitates that the application core uses the Zephyr operating system.
The framework allows for a high degree of configurability, which you can tailor according to your needs using Kconfig and devicetree settings.
This allows you to optimize the memory footprint and functionality of the custom peripherals, ensuring that the peripherals are well-integrated and perform efficiently within their specific application environments.

.. _nrf54l_hpf_softperi_comparison_creating_peripherals:

Creation of software-defined peripherals
****************************************

For Soft Peripherals, the integration process primarily involves incorporating glue code to facilitate their use.
To learn how to use those peripherals, and to see what is currently supported, refer to the :ref:`nrfxlib:soft_peripherals` documentation.

In contrast, HPF provides initial samples that serve as a starting point for development.
For a detailed guide on creating your own custom peripherals, see the :ref:`hpf_index` page.

.. _nrf54l_hpf_softperi_comparison_limitations:

Performance and limitations
***************************

Each Soft Peripheral is unique and comes with its own set of limitations compared to traditional hardware IP.
These limitations are specific to the functions that the peripheral is designed to emulate and how they integrate with the rest of the system.

HPF, while offering extensive customization and control, is currently in an experimental stage and lacks full power management support.

.. _hpf_index:

High-Performance Framework (HPF)
################################

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

Subsequent sections explain practical aspects of using the High-Performance Framework (HPF).

The integrated RISC-V coprocessor (FLPR) provides real-time peripherals mapped to the processor's Control and Status Register (CSR) space.
It facilitates development of custom, software-defined peripherals, enhancing functionality and performance of the main processor.

HPF is designed for optimal size and latency.
To achieve this, the Zephyr kernel and other unnecessary components are disabled.
For details see the configuration file :file:`nrf/applications/hpf/mspi/boards/nrf54l15dk_nrf54l15_cpuflpr.conf`.
With these changes, the application runs in a simple, single-threaded, baremetal environment.

High-Performance Framework, is a framework designed to facilitate the creation and integration of software peripherals using coprocessors.
It provides the following resources:

* Targeted tools - Hardware Abstraction Layers (HALs) for VPR's :ref:`Real-Time peripherals<hpf_real_time_peripherals>` and :ref:`CMake targets for assembly management<hpf_assembly_management_cmake>`.

* :ref:`Application examples <hpf_applications_readme>` for the supported drivers.

* Descriptions of architectural issues when creating software peripherals: :ref:`event handling<hpf_event_handling>`, :ref:`fault handling<hpf_fault_handling>`, and :ref:`power management<hpf_power_management>`.

These resources are intended to serve as foundational starting points, enabling you to create customized software peripherals tailored to specific application needs.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   assembly_management.rst
   event_handling.rst
   fault_handling.rst
   power_management.rst
   rt_peripherals.rst

.. _abi_compatibility:

ABI Compatibility
*****************

.. contents::
   :local:
   :depth: 2

Application Binary Interface (ABI) compatibility defines how software components, like libraries and applications, interact at the machine code level through their low-level binary interface, including:

* Function-calling conventions
* Data structure layouts in memory
* Exception handling mechanisms
* Register usage conventions

When ABI compatibility is maintained, binaries of one component can interface correctly with another without needing recompilation.
For example, adding a new function to a library is typically an ABI-compatible change, as existing binaries remain functional.
However, changes that affect data structure layouts, such as altering field order or size, break ABI compatibility as they change the memory layout expected by existing binaries.

ABI Compatibility Matrix for the nrf54h20_soc_binary bundle
===========================================================

The following table illustrates ABI compatibility between different versions of the nRF54H20 SoC binaries bundle and the |NCS|:

.. list-table::
   :header-rows: 1

   * - |NCS| versions
     - Compatible nRF54H20 SoC binaries version
   * - |NCS| v2.7.99-cs2
     - nrf54h20_soc_binaries v0.6.5
   * - |NCS| v2.7.99-cs1
     - nrf54h20_soc_binaries v0.6.2
   * - |NCS| v2.7.0
     - nrf54h20_soc_binaries v0.5.0
   * - |NCS| v2.6.99-cs2
     - nrf54h20_soc_binaries v0.3.3

ABI compatibility ensures that the Secure Domain and System Controller firmware binaries do not need to be recompiled each time the Application, Radio binaries, or both are recompiled, as long as they are based on a compatible NCS version.
Additionally, maintaining ABI compatibility allows the nRF54H20 SoC binaries components to work together without recompilation when updating to newer |NCS| versions.

.. _abi_compatibility:

|ISE| ABI compatibility
#######################

.. contents::
   :local:
   :depth: 2

Application Binary Interface (ABI) compatibility defines how software components, such as libraries and applications, interact at the machine code level through their low-level binary interface.
This includes:

* Function-calling conventions
* Data structure layouts in memory
* Exception handling mechanisms
* Register usage conventions

When ABI compatibility is maintained, binaries of one component can interface correctly with another without requiring recompilation.
For example, adding a new function to a library is typically an ABI-compatible change, as existing binaries remain functional.
However, changes that affect data structure layouts, such as altering field order or size, break ABI compatibility because they change the memory layout expected by existing binaries.

This page describes the ABI compatibility between the |NCS| and the |ISE| binaries.

ABI compatibility for the nRF54H20 IronSide SE binaries
*******************************************************

To use the most recent version of the |NCS|, *always* download and provision your nRF54H20 SoC-based device with the `latest nRF54H20 IronSide SE binaries`_ available.

.. caution::
   The nRF54H20 IronSide SE binaries do not support rollbacks to previous versions.

Provisioning the nRF54H20 SoC
*****************************

To provision the nRF54H20 SoC using the nRF54H20 IronSide SE binaries, see :ref:`ug_nrf54h20_gs_bringup`.

Updating the nRF54H20 SoC
*************************

To update the nRF54H20 IronSide SE binaries to the latest version, see :ref:`ug_nrf54h20_ironside_se_update`.

nRF54H20 IronSide SE binaries changelog
***************************************

See `IronSide SE binaries changelog on the main branch`_ for the lists of changes by component.

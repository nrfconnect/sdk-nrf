.. _ug_nrf54h20_architecture_memory:

nRF54H20 Memory Layout
######################

.. contents::
   :local:
   :depth: 2

The memory inside the nRF54H20 can be classified by several characteristics, like retention without power and CPU association.
When classifying memories by storage time, the nRF54H20 contains the following types of memory:

* Static RAM (RAM)
* Magnetic RAM (MRAM) with non-volatile memory properties

MRAM retains its content after disconnecting the system from power, while RAM does not.
MRAM is intended to store executable firmware images, as well as credentials, and other data at rest.

When classifying memories by CPU association, the nRF54H20 contains the following types of memory:

* Local memory
* Global memory

The MRAM is a global memory available to all CPUs in the system.
Some RAM memories are, instead, tightly coupled to a specific CPU, while others are intended to be shared between multiple CPUs.

.. note::
   In the nRF54H20 initial limited sampling, TrustZone is disabled on all cores.

RAM
***

There are multiple RAM banks in the system.
Each local domain (like Application or Radio) contains its own RAM.
There is also a large part of the RAM in the global domain to be shared between the cores in the system.

Local RAM
=========

Local RAM is present in each of local domains

Application core RAM
--------------------

.. image:: images/nrf54h20_memory_map_app.svg
   :width: 300 px

The application core contains 32 KB of local RAM.
Accessing this memory from the application core CPU has minimal latency, but accessing it from any other core adds significant latency.
Because of this property, the local RAM in the application domain should be used mainly to store data frequently accessed by the application core, or to store timing-critical parts of the code executed by the application core.

Address range
   0x22000000 - 0x22008000

Size
   32 KB

Access control
   Application domain local RAM is accessible by the application core.
   Any core (like FLPR or PPR) or peripheral configured to be owned by application core (like UARTE or SAADC) can access this memory as well.
   Any core with access to this memory can execute code from it.

   If the TrustZone feature is enabled for the application core, this memory can be partitioned in one secure and one non-secure region.
   The secure region is accessible only by code executed with the secure attribute, while the non-secure region is accessible by any code.

   .. note::
      Code executed by VPRs (like FLPR or PPR) has its secure attribute matching the given VPR security configuration in the SPU.
      Local RAM cannot include a Non-Secure Callable section.

Radio core RAM
--------------

The radio core contains 96 KB of local RAM.
Any access to this memory has minimal latency if originated either from radio core CPU or from peripherals with EasyDMA located in the radio core.
Any access from any other core has a significant latency.
Because of this property, local RAM in the radio core should be used mainly to store data frequently accessed by the radio core CPU or the radio protocol frames to be accessed by CCM or RADIO peripherals, or to store timing critical parts of the code executed by the radio core CPU.

Address range
   0x23000000 - 0x23030000

Size
   192 KB

Access control
   The radio core local RAM is accessible by the radio core.
   Any core (like FLPR or PPR) or peripheral configured to be owned by the radio core (like UARTE or SAADC) can access this memory as well.
   Any core with access to this memory can execute code from it.

   If the TrustZone feature is enabled for the radio core, this memory can be partitioned in one secure and one non-secure region.
   The secure region is accessible only by code executed with the secure attribute, while the non-secure region is accessible by any code.

   .. note::
      Code executed by VPRs (like FLPR or PPR) has its secure attribute matching the given VPR security configuration in the SPU
      Local RAM cannot include a Non-Secure Callable section.

BBPROC memory
^^^^^^^^^^^^^

The Lower 32 KB of local RAM in the Radio Domain is tightly coupled with BBPROC.
Any access to this memory has minimal latency if originated from BBPROC.
Any access originated from the radio core or from peripherals with EasyDMA located in radio domain have a little greater latency while accessing BBPROC memory.
Access from other domains is possible, but with significant latency.

BBPROC memory is the only memory from which BBPROC can fetch its instructions.
Because of this property, this memory block is mostly intended to store BBPROC code and data.
When BBPROC is unused in a system, this memory can be used as additional local RAM in the Radio Domain.

Address range
   0x23040000 - 0x23048000

Size
   32 KB

Access control
   The access to the BBPROC memory is configured as the access to the local RAM in the Radio Domain.

Secure Domain
-------------

The Secure Domain contains 32 KB of local RAM and contains a firmware image provided by Nordic Semiconductor.

Global RAM
==========

The Global Domain RAM (or Global RAM, GRAM) is distributed in multiple instances across the system.
Each of the instances has other properties and other purposes.

.. _ug_nrf54h20_architecture_memory_gp_shared_ram:

General-purpose shared RAM (RAM0x)
----------------------------------

The biggest part of the RAM memory in the system is located in the Global Domain as general-purpose shared RAM.
Access to this memory is relatively fast from all the local domains (like the Application or the Radio ones).
Access to this memory from DMA used by USB has minimal latency.

This memory is intended to store the majority of the data used by local cores (and does not fit in local domains' RAM) including shared memory used for Inter-Processor Communication (IPC) between local cores.
Buffers for USB data must be stored in this memory part, in the region owned by the core owning USB (usually the application core in typical applications).

Address range
   0x2F000000 - 0x2F0C0000

Size
   768 KB

Access control
   The general-purpose shared RAM is split into multiple partitions.
   Each of the local cores has two partitions assigned: one configured as Secure, the other one as Non-Secure.
   The partitions are configured in the given core's UICR.

   If TrustZone is enabled for a core, the Secure partition is used to store the data of the Secure Processing Environment, while the Non-Secure partition stores the data of the Non-Secure Processing Environment and the shared memory used by the Inter-Processor Communication towards other local domains.
   If TrustZone is disabled for a core, the Secure partition assigned to this core is used to store program data, while the Non-Secure partition contains the shared memory used by IPC.

   Secure partitions are grouped at the beginning of the general-purpose shared RAM block, while Non-Secure partitions are grouped at the end.
   Non-Secure partitions are overlapping to define shared-memory IPC regions.

   A partition assigned to a core is accessible from this core, other cores owned by this core, or DMAs used by peripherals used by this core.
   Access from other cores or peripherals is prevented.
   A partition configured as Secure is accessible only from the Secure Processing Environment in the core owning the given partition.
   A partition configured as Non-Secure is accessible from both the Secure and Non-Secure Processing Environments running in the core owning the given partition.


   .. note::
      If TrustZone is disabled for a given core, the only available Processing Environment is Secure.

SYSCTRL memory (RAM20)
----------------------

The SYSCTRL memory is a part of the global RAM tightly coupled with the System Controller.
Access to this memory block from the System Controller has minimal latency and can be performed without powering up any other parts of the system.
Access to this memory from the local domains has higher latency than access to the general-purpose shared RAM.

This memory is statically partitioned.
The layout is not to be adjusted for specific products.

This memory is intended to store the code executed in the System Controller, the System Controller's data, and the shared memory used for Inter-Processor Communication between the System Controller and other cores.
Because of the static allocation property, this memory stores also the shared memory used for communication between debugger probes connected to cores in the system and the Secure Domain Core.

Address range
   0x2F880000 - 0x2F890000

Size
   64 KB

Access control
   The SYSCTRL memory is split into multiple partitions.
   The System Controller has access to all of them (System Controller's code and data, and shared memory regions).
   The shared memory regions are also accessible by the cores using particular region for communication with the System Controller and the debugger.
   The shared memory regions are statically allocated by the Secure Domain.
   Cores do not have access to other parts of the SYSCTRL memory.

   If TrustZone is enabled for a core, the shared memory region is accessible from the Non-Secure Processing Environment.
   If TrustZone is disabled for a core, the shared memory region is accessible from the Secure Processing Environment.

Fast global RAM (RAM21)
-----------------------

The Fast global RAM is a part of the global RAM tightly coupled with the Fast Lightweight Processor.
Access to this memory block from the FLPR and fast peripherals' DMA (I3C, CAN, PWM120, UARTE120, SPIS120, SPIM120, SPIM121) has minimal latency and can be performed without powering up any other parts of the system.
Access to this memory from the local domains has higher latency than access to the general-purpose shared RAM.

This memory is intended to store the code executed in the FLPR, the FLPR's data, the shared memory used for Inter-Processor Communication between the FLPR and the core managing the FLPR, and DMA buffers for the fast peripherals.

Address range
   0x2F890000 - 0x2F8A0000

Size
   64 KB

Access control
   The FLPR has access to the entire RAM21 memory region.
   Because of this property, the FLPR's owner indirectly obtains access to the entire RAM21 memory region.
   To avoid security risks, all the partitions in RAM21 must be assigned to the FLPR's owner.
   Also, all peripherals with DMA buffers in this memory must be assigned to the FLPR's owner.
   The FLPR and the fast peripherals are by default owned by the application core.
   This ownership and matching memory access rights can be reassigned to the radio core in the UICR.

   The security attribute of memory partitions DMA engines must follow the FLPR security settings.

Slow global RAM (RAM3x)
-----------------------

The Slow global RAM is a part of the global RAM close to the Peripheral Processor.
Access to this memory block from the PPR and slow peripherals' DMA has minimal latency and can be performed without powering up any other parts of the system.
Access to this memory from the local domains has higher latency than access to the general-purpose shared RAM.

This memory is intended to store the code executed in the PPR, the PPR's data, the shared memory used for Inter-Processor Communication between the PPR and the core managing the PPR, and DMA buffers for the slow peripherals.

Address range
   0x2FC00000 - 0x2FC14000

Size
   80 KB

Access control
  The PPR and its owner have access to all the partitions assigned to the PPR and its Inter-Processor Communication.
  Each of the memory partition assigned for DMA of the slow peripherals is accessible from the core owning the given set of peripherals.
  The PPR and the slow peripherals are by default owned by the application core.
  The ownership and matching memory access rights can be customized in UICRs.

  The security attribute of memory partitions must follow PPR and DMA engines' security settings.

MRAM (non-volatile memory)
**************************

The MRAM is divided in the following parts:

* MRAM_10
* MRAM_11

MRAM_10
=======

The MRAM_10 is a part of the non-volatile memory intended to keep firmware images to execute.
Access to this memory has minimal latency to avoid CPU stalls on instruction fetches.
This part of the memory is not writable while the main application is running (it is writable only during the Firmware Upgrade procedure) to avoid any latency caused by write operations.
Apart from executable code images, this part of the memory stores the Secure Information Configuration Registers (SICR) used by the programs running in the Secure Domain Core.
If code and data for the application core do not fit in MRAM_10, it can be partially or fully placed in MRAM_11.

Address range
   0x0E000000 - 0x0E100000

Size
   1024 KB

Access control
   The application core and the radio core have read and execute access to memory regions assigned to them.
   If TrustZone is disabled for any of these cores, then the assigned memory region is a single block containing secure code and data.
   If TrustZone is enabled for any of these cores, then the assigned memory region is split in three blocks:

   * Secure code and data
   * Non-secure code and data
   * Non-secure callable (NSC)

   The code executed in the Secure Processing Environment of a core has access to all three blocks assigned to the core.
   The code executed in the Non-Secure Processing Environment has access only to the Non-secure code and data block, and can call function veneers located in the NSC block.

   The System Controller's code and data region is accessible only by the Secure Domain Core.

   Secure Domain has access to all parts of the MRAM_10.
   Other cores can access only the parts assigned to them, according to the security rules described above.

MRAM_11
=======

The MRAM_11 is a part of the non-volatile memory intended to keep non-volatile writable data.
Writing to MRAM_11 can increase access latency for other cores reading from MRAM_11.
When a core is reading or executing code from MRAM_11, the impact of the additional latency must be taken in consideration.
Each of the local cores (Application, Radio, Secure Domain) has an allocated partition in MRAM_11 to store their non-volatile data.
Each of the cores has full control on the data layout and management in the assigned MRAM partition.
There is also a Device Firmware Upgrade partition used to store firmware images used during the upgrade procedure.
If code and data for the application core do not fit in MRAM_10, it can be partially or fully placed in MRAM_11.

.. to review

Address range
   0x0E100000 - 0x0E200000

Size
   1024 KB

Access control
   The application core and the radio core have read and write access to their assigned non-volatile data regions.
   The non-volatile data region assigned to the core having TrustZone disabled is marked as Secure, while the non-volatile data region assigned to the core having TrustZone enabled is marked as Non-Secure.

   If code or data for the application core is placed in MRAM_11, the application core has *read and execute* access to this partition.
   This access can be configured as follows:

   * Default configuration, in which all the application code and data is placed in MRAM_10.
     It is configured with a single MPC configuration entry contained entirely in MRAM_10.
   * All the app code and data is placed in MRAM_11.
     It is configured with a single MPC configuration entry contained entirely in MRAM_11.
   * The app code and data is partially in MRAM_10, partially in MRAM_11.
     It is configured with a single MPC configuration entry covering partially MRAM_10 and partially MRAM_11.
     Because of the continuous memory address range, it is possible to use a single memory region to describe such data block.

   The Secure Domain has access to all the parts of MRAM_11.
   The application core has read and write access to the DFU partition.
   The security configuration of this partition follows the TrustZone configuration of the application core (Secure if TrustZone is disabled, or Non-Secure if TrustZone is enabled).

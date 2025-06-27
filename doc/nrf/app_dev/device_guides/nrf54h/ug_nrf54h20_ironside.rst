:orphan:

.. _ug_nrf54h20_ironside_se:

IRONside Secure Element
#######################

The IRONside Secure Element (IRONside SE) provides security features to the nRF54H20 SoC.
This document describe the features of the IRONside SE and the configuration it applies.

.. _ug_nrf54h20_ironside_se_uicr:

Global Resource configuration
*****************************

You can configure some global resources through the applications running on local domains (like the Application core).
Specifically, you can configure all the memory not reserved by IRONside SE, referred to as *application-owned memory*, and certain peripherals located in the global domain.

This configuration is stored in the User Information Configuration Registers (``UICR``), a non-volatile memory region used to hold device-specific configuration data.
Specifically, the configuration is located in the registers of the first Non-Volatile Register (``NVR``) page of the ``UICR``, known as ``NVR0``, as well as in the data referenced by these registers.
The acronym ``UICR`` collectively refers to this region and its associated registers.

If no configuration is defined in the ``UICR``, IRONside SE applies default configurations to configurable global resources.
Applications that do not require changes to these defaults can run without modifying the ``UICR``.

The following fields are defined in the ``UICR``:

* ``VERSION`` - Specifies the version of the ``UICR``.
* ``LOCK`` - Locks the contents of the ``UICR``.
* ``APPROTECT`` - Specifies the protection settings for the various Access Ports (APs) in the system.
* ``ERASEPROTECT`` - Enables erase protection for the ``UICR``.
* ``PROTECTEDMEM`` - Specifies a memory region, starting at the end of IRONside SE memory, that is protected for integrity.
* ``RECOVERY`` - Contains the application-defined recovery mechanism details.
* ``ITS`` - Sets the location and size of the Internal Trusted Storage for the application and radio domains.
* ``PERIPHCONF`` - Points to an array of global peripheral configurations.
* ``MPCCONF`` - Points to an array of global memory configurations.

UICR.VERSION
============

``UICR.VERSION`` specifies the version of the ``UICR`` format in use.
It is divided into a 16-bit major version and a 16-bit minor version.

This versioning scheme allows IRONside to support multiple ``UICR`` formats, enabling updates to the format without breaking compatibility with existing configurations.

UICR.LOCK
=========

Enabling ``UICR.LOCK`` locks the entire contents of the ``NVR0`` page located in ``MRAM10``.
This includes all values in both the ``UICR`` and the ``BICR`` (the Board Information Configuration Registers).
When ``UICR.LOCK`` is enabled, you can modify the ``UICR`` only by performing an ``ERASEALL`` operation.

.. note::
   While ``BICR`` is not erased during an ``ERASEALL`` operation, executing ``ERASEALL`` lifts the ``UICR.LOCK`` restriction, allowing write access to ``BICR``.

Locking is enforced through an integrity check and by configuring the ``NVR`` page as read-only in the ``MRAMC``.

If the integrity check fails, the application is booted with ``CPUCONF[APP].CPUWAIT`` set.
It is not possible to boot the vendor-specified recovery firmware if the integrity check fails.

UICR.APPROTECT
==============

You can configure several Access Ports (APs) through UICR.
``UICR.APPROTECT`` controls debugger access when connected to an AP, specifically the settings in the ``TAMPC`` peripheral.
Set all APs to ``UICR_APPROTECT_PROTECTED`` to get a protected device.

The table below shows the configuration of the ``TAMPC`` peripheral for each AP.

+-----------+-----------+-----------+-----------+-----------+-----------+-------------------------------+
|                TAMPC.DOMAIN[n]                |   TAMPC.AP[n]         | Configuration                 |
+-----------+-----------+-----------+-----------+-----------+-----------+                               +
| DBGEN     | NIDEN     | SPIDEN    | SPNIDEN   | DBGEN     | SPIDEN    |                               |
+===========+===========+===========+===========+===========+===========+===============================+
|     0     |     0     |     0     |     0     |     0     |     0     | UICR_APPROTECT_PROTECTED      |
+-----------+-----------+-----------+-----------+-----------+-----------+-------------------------------+
|     1     |     1     |     1     |     1     |     1     |     1     | UICR_APPROTECT_UNPROTECTED    |
+-----------+-----------+-----------+-----------+-----------+-----------+-------------------------------+

+-----------+-----------+-----------+-----------+-----------+--------------------------------+
|                         TAMPC.CORESIGHT                   | Configuration                  |
+-----------+-----------+-----------+-----------+-----------+                                +
| DEVICEEN  | DBGEN     | NIDEN     | SPIDEN    | SPNIDEN   |                                |
+===========+===========+===========+===========+===========+================================+
|     0     |     0     |     0     |     0     |     0     | UICR_APPROTECT_PROTECTED       |
+-----------+-----------+-----------+-----------+-----------+--------------------------------+
|     1     |     1     |     1     |     1     |     1     | UICR_APPROTECT_UNPROTECTED     |
+-----------+-----------+-----------+-----------+-----------+--------------------------------+

UICR.ERASEPROTECT
=================

Enabling ``UICR.ERASEPROTECT`` blocks the ``ERASEALL`` operation.
However, it does not prevent erase operations initiated through other means, such as writing erase values via a debugger.

.. note::
   If this configuration is enabled and ``UICR.LOCK`` is also set, it is no longer possible to modify the ``UICR`` in any way.
   Therefore, this configuration should only be enabled during the final stages of production.

UICR.PROTECTEDMEM
=================

In the ``UICR.PROTECTEDMEM`` field, you can specify a memory region that will have its integrity ensured by IRONside SE.
This memory is intended for containing immutable bootloaders, ``UICR.PERIPHCONF`` entries, ``UICR.MPCCONF`` entries, and any other data that should be immutable.
By ensuring the integrity of this memory region, IRONside SE extends the Root of Trust to any immutable bootloader located in this region.
The value in this field specifies the number of 4 kB blocks, starting from the lowest MRAM address of the application-owned memory.

The following figure illustrates a configuration where an immutable bootloader is configured for each domain while being protected from the other domain.

.. image:: ../img/protectedmem.png
   :width: 60%
   :align: center
   :alt: Enabling an immutable bootloader for each domain.

UICR.PERIPHCONF
===============

``UICR.PERIPHCONF`` points to an array of key-value pairs used to initialize specific global peripherals before the application starts.
This mechanism is intended for one-time configuration of peripherals managed by IRONside SE and is not designed for general system initialization.

Each entry in the array consists of two 32-bit values.
The fields in each value are described in the following tables.

Value 0:

+-------------------+----------------------------------+-------------------------+
| Bit number(s)     | 31-2                             | 1-0                     |
+-------------------+----------------------------------+-------------------------+
| Field             | REGPTR                           | UNUSED                  |
+-------------------+----------------------------------+-------------------------+
| Description       | Bits [31:2] of a pointer to a    | Unused.                 |
|                   | peripheral register.             |                         |
+-------------------+----------------------------------+-------------------------+

Value 1:

+-------------------+----------------------------------+
| Bit number(s)     | 31-0                             |
+-------------------+----------------------------------+
| Field             | VALUE                            |
+-------------------+----------------------------------+
| Description       | Register value.                  |
+-------------------+----------------------------------+

IRONside SE processes the ``PERIPHCONF`` array sequentially, starting from the address specified by ``UICR.PERIPHCONF.ADDRESS``.
Processing continues until either the number of entries defined by ``UICR.PERIPHCONF.MAXCOUNT`` has been processed, or an entry is encountered with the ``REGPTR`` field set to ``0x3FFF_FFFF`` (all ones), which indicates the end of the array.

IRONside SE uses an allow list to determine which register addresses the ``REGPTR`` field is permitted to reference.
Each register address in the allow list has an associated bit mask that specifies which bits from the ``VALUE`` field are applied to the target register.

Given an entry in the ``PERIPHCONF`` array and a bit mask ``M``, IRONside SE performs the following write operation:

``*(REGPTR << 2) = (VALUE & M) | (*(REGPTR << 2) & ~M)``

The register allow list and corresponding bit masks are documented with each IRONside SE release.

Each entry in the ``PERIPHCONF`` array is validated during processing.
To pass validation, ``(REGPTR << 2)`` must point to a register address included in the allow list.

After applying the entry, IRONside SE performs a read-back check: it reads back the register value, applies the bit mask, and compares the result against the masked ``VALUE`` field.

The configuration procedure is aborted if an entry fails either the validation or the read-back check.
If a failure occurs, ``BOOTSTATUS.BOOTERROR`` is set to indicate the error condition, and a description of the failed entry is written to the boot report.

Configuration validation and optimization
-----------------------------------------

All configurations in ``UICR`` are verified by IRONside SE to ensure that they do not conflict with Nordic resources.
Once verified, IRONside SE stores a digest of the UICR and the memory regions referenced by UICR in the Secure Information Configuration Region (SICR).
Before performing UICR validation on subsequent boots, IRONside SE compares the current digest against the stored value.
If the digests match, the validation step is skipped, as the configuration is known to be valid.

.. _ug_nrf54h20_ironside_se_programming:

Programming
***********

By default, ``MRAM`` is configured in *Direct Write mode*, and all memory not reserved by Nordic firmware is accessible with Read/Write/Execute (RWX) permissions by any domain owner.
The Access Port (AP) for the application core is also enabled and available by default, allowing you to directly program all memory not reserved by Nordic firmware in the default configuration.

.. note::
   Access to external memory (EXMIF) requires a non-default configuration of the ``GPIO.CTRLSEL`` register.

Global domain memory can be protected from write operations using UICR configurations.
You can remove these protections by performing an ``ERASEALL`` operation, which also disables all protection mechanisms enforced through UICR settings.

.. _ug_nrf54h20_ironside_se_debug:

Debugging
*********

IRONside SE supports debugging through the ``DEBUGWAIT`` procedure.
This procedure ensures that ``CPUCONF[APP].CPUWAIT`` is set when the application core starts, preventing the CPU from executing any instructions until a debugger manually releases it.
This allows a debugger to attach and take control of the execution from the first instruction.

You can also configure the ``CPUWAIT`` value when booting other cores by using the ``cpuconf`` service.

.. _ug_nrf54h20_ironside_se_protecting:

Protecting the device
*********************

To protect the device, several security mechanisms must be enabled.
This chapter describes these mechanisms and explains how they relate to each other.

UICR Protection
===============

To protect the device, you must configure the following UICR fields:

* ``UICR.APPROTECT`` - Protect all APs.
* ``UICR.LOCK`` - Lock the UICR.
* ``UICR.PROTECTEDMEM`` - Check the integrity of the immutable bootloader.
* ``UICR.ERASEPROTECT`` - Enable erase protection.

IRONside Boot Report
********************

The IRONside boot report contains device state information communicated from IRONside SE to the local domains.
It is written to a reserved region in ``RAM20``, which is accessible to the local domain in the default system configuration.
There is one boot report per processor that is booted, either directly by IRONside SE or via the ``CPUCONF`` service.

The boot report contains the following information:

* Magic value
* IRONside SE version
* IRONside SE recovery version
* IRONside SE update status
* UICR error description
* Context data passed to the CPUCONF service
* A fixed amount of random bytes generated by a CSPRNG

Versions
========

The boot report includes version information for both IRONside SE and IRONside SE Recovery.

The regular version format consists of four fields: ``MAJOR.MINOR.PATCH.SEQNUM``, with each field occupying 8 bits.
The first three fields follow semantic versioning, while the ``SEQNUM`` field is a wrapping sequence number that increments by one with each version.
The values ``0`` and ``127`` are reserved for ``SEQNUM``.

An additional version field, referred to as the *extra version*, contains a null-terminated ASCII string with human-readable version information.
This string is informational only, and no semantics should be attached to this part of the version.

IRONside SE Update status
=========================

The Secure Domain boot ROM code (SDROM) reports the status of an IRONside SE update request through ``SICR.UROT.UPDATE.STATUS``.
The value of this register is copied to the IRONside SE update status field of the boot report.

UICR error description
======================

This field indicates if any UICR error occurred.

Local domain context
====================

This field is populated by the local domain that is invoking the CPUCONF service.
It is set to `0` for the application core which is booted by IRONside SE.
This service is used when one local domain boots another local domain.
The caller can populate this field with arbitrary data that will be made available to the local domain being booted.
Tipical examples of data that could be passed include IPC buffer sizes or the application firmware version.
The unused parts of this field are set to 0.

Random data
===========

This field is filled with random data generated by a CSPRNG.
This data is suitable as a source of initial entropy.

.. _ironside_se_booting:

Booting of other domains
************************

IRONside SE boots the System Controller core first, followed by the application core, in that order.
When booting the application core, IRONside SE does the following:

* Sets ``CPUCONF[APP].INITSVTOR`` to the first 32-bit word of the application-owned memory.
* Reads the reset vector from the second 32-bit word of the application-owned memory.
* If the reset vector is set to ``0xFFFFFFFF``, sets ``CTRL_AP.BOOTSTATUS.BOOTERROR`` to indicate that no firmware is programmed.
* If any other error is encountered during initialization, sets ``CTRL_AP.BOOTSTATUS.BOOTERROR`` accordingly.
* If ``CTRL_AP.BOOTSTATUS.BOOTERROR`` is non-zero, sets ``CPUCONF[APP].CPUWAIT`` to ``1``; otherwise, sets it to ``0``.
* Sets ``CPUCONF[APP].CPUSTART`` to ``1``.

UICR Error handling
===================

If an invalid ``UICR`` configuration is detected, IRONside SE performs the following procedure:

* Stops the allocation procedure.
* Updates the boot report to indicate the ``UICR`` entry (and, if applicable, the array index) that triggered the failure.
* Sets ``CTRL_AP.BOOTSTATUS.BOOTERROR`` to indicate the source of the error.
* Starts the application core with ``CPUCONF[APP].CPUWAIT = 1`` (halted mode).

This allows the error report to be read by a debugger, if the device is not protected.

.. _ug_nrf54h20_ironside_se_cpuconf_service:

IRONside SE CPUCONF Service
***************************

The ``IRONside SE CPUCONF`` service is available to the application core and enables it to trigger the boot of another CPU at a specified address.

Specifically, IRONside SE sets ``NRF_CPUCONF_S->INITSVTOR`` with the address provided to the IRONside call, and then writes ``0x1`` to ``NRF_RADIOCORE_CPUCONF_S->CPUSTART`` to start the target CPU.

When ``cpuwait`` is enabled in the IRONside service call, the target CPU is stalled by writing ``0x1`` to ``NRF_RADIOCORE_CPUCONF_S->CPUWAIT``.
This feature is intended for debugging purposes.

.. note::
   * ``NRF_CPUCONF_S->TASKS_ERASECACHE`` is not yet supported.
   * ``NRF_CPUCONF_S->INITNSVTOR`` will not be supported.
For details about the ``CPUCONF`` peripheral, refer to the nRF54H20 SoC datasheet.

.. _ug_nrf54h20_ironside_se_update_service:

IRONside SE update Service
**************************

IRONside SE is updated by the Secure Domain ROM (SDROM), which performs the update operation when triggered by a set of SICR registers.
SDROM verifies and copies the update candidate specified through these registers.

IRONside SE exposes an update service that allows local domains to trigger the update process by indirectly writing to the relevant SICR registers.

The  release ZIP archive for IRONside SE includes the following components:

* A HEX file containing the update candidate for IRONside SE.
* A HEX file for IRONside SE Recovery.
* An application core image that executes the IRONside SE update service to install the update candidate HEX files.

.. _ug_nrf54h20_ironside_se_bootmode_register_format:

CTRLAP.BOOTMODE register format
*******************************

.. _ironside_se_boot_commands:

The format of the ``CTRLAP.MAILBOX.BOOTMODE`` register is described in the following table.

+------------------+--------+------------------+-----+----------------+--------+------------+
| Bit number(s)    | 31-8   | 7                | 6-5 | 4              | 3-1    | 0          |
+------------------+--------+------------------+-----+----------------+--------+------------+
| Field            | N/A    | SECDOM_DEBUGWAIT | RFU | SAFEMODE (ROM) | OPCODE | MODE (ROM) |
+------------------+--------+------------------+-----+----------------+--------+------------+

.. _ug_nrf54h20_ironside_se_bootstatus_register_format:

CTRLAP.BOOTSTATUS register format
*********************************

The general format of the ``CTRLAP.BOOTSTATUS`` register is described in the following table.

+------------------+-------+-----------+------+
| Bit number(s)    | 31-28 | 27-24     | 23-0 |
+------------------+-------+-----------+------+
| Field            | RFU   | BOOTSTAGE | INFO |
+------------------+-------+-----------+------+

Fields marked as ``RFU`` (Reserved for Future Use) are set to ``0``, unless otherwise specified.
The ``BOOTSTAGE`` field indicates which component in the boot sequence encountered a failure.
This field can have one of the following values:

.. list-table::
   :header-rows: 1

   * - ``BOOTSTAGE`` value
     - Description
   * - ``0x0``
     - Unset (reset value)
   * - ``0x1``
     - SysCtrl ROM
   * - ``0x2``
     - Secure domain ROM
   * - ``0xB``
     - Secure domain firmware with SUIT (major version < 20)
   * - ``0xC``
     - Secure domain firmware (major version >= 20)
   * - ``0xD``
     - Secure domain recovery firmware (major version >= 20)

.. note::
The value ``0xB`` indicates a boot status error reported by the Secure Domain running a version earlier than version 20.

The format of the ``INFO`` field depends on the ``BOOTSTAGE`` value.
If ``BOOTSTAGE`` is set to ``0x1`` or ``0x2``, the register has the following format:

+------------------+-------+-----------+----------+--------+
| Bit number(s)    | 31-28 | 27-24     | 23-16    | 15-0   |
+------------------+-------+-----------+----------+--------+
| Field            | RFU   | BOOTSTAGE | BOOTSTEP | STATUS |
+------------------+-------+-----------+----------+--------+

If ``BOOTSTAGE`` is set to ``0xC``, the register has the following format

+------------------+-------+-----------+-------+-----------+-----------+-----------+-----+-------------+
| Bit number(s)    | 31-28 | 27-24     | 23-22 | 21-15     | 14-12     | 11-9      | 8   | 7-0         |
+------------------+-------+-----------+-------+-----------+-----------+-----------+-----+-------------+
| Field            | RFU   | BOOTSTAGE | RFU   | FWVERSION | CMDOPCODE | CMDERROR  | RFU | BOOTERROR   |
+------------------+-------+-----------+-------+-----------+-----------+-----------+-----+-------------+

The register is written by the Secure Domain Firmware (SDFW) component of IRONside SE at the end of every cold boot sequence.
A value of ``0`` indicates that the SDFW did not complete the boot process.

The following fields are reported by the secure domain firmware:

``FWVERSION``
  Reports the ``SEQNUM`` field of the IRONside SE version.
The value reported in this field is incremented with each released version of the firmware.
It can be used to distinguish between firmware versions within a specific release window.

``CMDOPCODE``
  The opcode of the boot command issued to the SDFW in the ``CTRLAP.MAILBOX.BOOTMODE`` register.
  A value of ``0`` indicates that no boot command has been issued.

``CMDERROR``
  A code indicating the execution status of the boot command specified in ``CMDOPCODE``:

  * A status value of ``0`` indicates that the command was executed successfully.
  * A non-zero value indicates that an error condition occurred during execution of the command.
    The error code ``0x7`` means that an unexpected condition happened that may have prevented the command from executing.
    Other error codes must be interpreted based on the boot command in ``CMDOPCODE``.

``BOOTERROR``
  A code indicating the status of the application domain boot sequence:

  * A status value of ``0`` indicates that the CPU was started normally.
  * A non-zero value indicates that an error condition occurred, preventing the CPU from starting.
    Detailed information about the issue can be found in the boot report.

.. _ug_nrf54h20_ironside_se_boot_commands:

Boot commands
*************

The debugger can instruct IRONside SE to perform an action during the boot sequence.
These actions are called *boot commands*.
Boot commands are issued through the ``CTRLAP.MAILBOX.BOOTMODE`` register and are processed only during a cold boot.
IRONside SE indicates that a boot command was executed by setting the ``CTRLAP.BOOTSTATUS`` register.

The recommended flow for issuing a boot command if the following:

1. Write the command opcode to the OPCODE field in ``CTRLAP.MAILBOX.BOOTMODE``.
#. Trigger a global reset by setting ``CTRLAP.RESET = 1``.

   .. note::
      Any global reset that does not involve a power cycle can be used in place of a CTRLAP reset here.

#. Wait for the command status to be acknowledged in ``CTRLAP.BOOTSTATUS``.
#. Clear the command opcode by writing zeroes to the OPCODE field in ``CTRLAP.MAILBOX.BOOTMODE``.
   Because this register is retained across resets, it must be cleared to prevent the command from being re-executed on the next cold boot.

See the following table for a summary of the available boot commands:

.. list-table::
   :header-rows: 1

   * - Opcode
     - Command name
     - Description
   * - ``ERASEALL``
     - ``0x1``
     - Erase all user data.
   * - ``DEBUGWAIT``
     - ``0x2``
     - Start the application CPU with ``CPUCONF.CPUWAIT = 1``.

The following chapters describe each command in detail.

Command ``ERASEALL``
=====================

The ``ERASEALL`` command instructs IRONside SE to erase all application-owned memory.
The following steps are performed:

* Erases pages in ``MRAMC10``, starting from the first page following the Secure Domain Recovery Firmware code, up to and including the last page in ``MRAMC10``.
* Writes zeroes to all global domain general-purpose RAM.
* Erases page 0 of the ``MRAMC10`` NVR, excluding ``BICR``.
  This operation erases the ``UICR``.
* Erases all pages in ``MRAM11``, excluding NVR pages.

Page 1 of the ``MRAMC10`` NVR is not erased.

The ``ERASEALL`` command can be explicitly permitted by disabling erase protection through the ``UICR.ERASEPROTECT`` field in the application's ``UICR``.
The purpose of erase protection is to prevent unauthorized re-purposing of the device.
In the context of lifecycle management, enabling both access port protection and erase protection makes it impossible to re-enter the *Configuration* state using a debugger.

The ``ERASEALL`` command defines the following ``CTRLAP.BOOTSTATUS.CMDERROR`` values:

* ``0x1``: an ``ERASEALL`` request was issued through ``CTRLAP.MAILBOX.BOOTMODE``, but the operation was blocked by ``UICR.ERASEPROTECT`` in the UICR.

Command ``DEBUGWAIT``
=====================

The ``DEBUGWAIT`` command instructs IRONside SE to start the application core with ``CPUCONF.CPUWAIT = 1``, unconditionally.
This prevents the CPU from executing any instructions until the ``CPUWAIT`` register is cleared by a connected debugger.
This command can be used to enable debugging from the first executed instruction of the application code.
It can also be used by flash programmers to safely program memory without the risk of the CPU accessing the memory concurrently.

The ``DEBUGWAIT`` command does not define any command-specific values for the ``CTRLAP.BOOTSTATUS.CMDERROR`` field.

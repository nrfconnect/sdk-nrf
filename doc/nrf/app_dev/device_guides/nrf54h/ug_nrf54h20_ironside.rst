:orphan:

.. _ug_nrf54h20_ironside_se:

IronSide Secure Element
#######################

.. contents::
   :local:
   :depth: 2

The IronSide Secure Element (IronSide SE) is a firmware for the Secure Domain of the nRF54H20 SoC that provides security features.
It is provided as a precompiled binary, part of the nRF54H20 SoC bundle.

IronSide SE provides the following features:

* Global memory configuration (via UICR.MPCCONF)
* Peripheral configuration (via UICR.PERIPHCONF)
* Boot commands (ERASEALL, DEBUGWAIT)
* CPUCONF service
* Update service

.. _ug_nrf54h20_ironside_se_uicr:

Global Resource configuration
*****************************

IronSide SE uses the User Information Configuration Registers (UICR) to securely configure persistent global resource settings.
The UICR is a dedicated non-volatile memory region (and its associated registers) located in the first NVR page (NVR0).
It holds device-specific configuration data, such as application-owned memory boundaries and selected peripheral parameters, before the application starts.

The following UICR fields are supported:

* ``UICR.VERSION`` - A 32-bit value that encodes the UICR format version (16-bit major and 16-bit minor).
* ``UICR.LOCK`` - Locks all contents of NVR0, preventing any further writes without performing an ``ERASEALL`` operation.
* ``UICR.APPROTECT`` - Configures debugger and access-port permissions for each AP via the ``TAMPC`` peripheral.
* ``UICR.ERASEPROTECT`` - Blocks ``ERASEALL`` commands to NVR0 (does not block debug-initiated erases).
* ``UICR.PROTECTEDMEM`` - Defines the size (in 4 KiB blocks) of an integrity-checked memory region at the end of the IronSide SE code.
* ``UICR.RECOVERY`` - Holds application-specific parameters used by the recovery mechanism.
* ``UICR.ITS`` - Specifies the base address and size of Internal Trusted Storage for application and radio domains.
* ``UICR.PERIPHCONF`` - Points to an array of key-value entries used to initialize approved global peripherals.
* ``UICR.MPCCONF`` - Points to an array of memory-protection entries used to configure global memory regions.

.. note::
   If no UICR values are programmed, IronSide SE applies a set of default configurations.
   Applications that do not require custom settings can rely on these defaults without modifying the UICR.

UICR.VERSION
============

``UICR.VERSION`` specifies the version of the ``UICR`` format in use.
It is divided into a 16-bit major version and a 16-bit minor version.

This versioning scheme allows IronSide to support multiple ``UICR`` formats, enabling updates to the format without breaking compatibility with existing configurations.

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

The following table shows the configuration of the ``TAMPC`` peripheral for each AP.

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

In the ``UICR.PROTECTEDMEM`` field, you can specify a memory region that will have its integrity ensured by IronSide SE.
This memory can contain immutable bootloaders, ``UICR.PERIPHCONF`` entries, ``UICR.MPCCONF`` entries, or any other data that should be immutable.
By ensuring the integrity of this memory region, IronSide SE extends the Root of Trust to any immutable bootloader located in this region.

The value in this field specifies the number of 4 kB blocks, starting from the lowest MRAM address of the application-owned memory.

The following figure illustrates a configuration where an immutable bootloader is configured for each domain while being protected from the other domain.

##TODO add image

.. image:: ../img/protectedmem.png
   :width: 60%
   :align: center
   :alt: Enabling an immutable bootloader for each domain.

UICR.PERIPHCONF
===============

``UICR.PERIPHCONF`` points to an array of key-value pairs used to initialize specific global peripherals before the application starts.
This mechanism allows for the one-time configuration of peripherals managed by IronSide SE and is not designed for general system initialization.

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

IronSide SE processes the ``PERIPHCONF`` array sequentially, starting from the address specified by ``UICR.PERIPHCONF.ADDRESS``.
Processing continues until either the number of entries defined by ``UICR.PERIPHCONF.MAXCOUNT`` has been processed, or an entry is encountered with the ``REGPTR`` field set to ``0x3FFF_FFFF`` (all ones), which indicates the end of the array.

IronSide SE uses an allow list to determine which register addresses the ``REGPTR`` field is permitted to reference.
Each register address in the allow list has an associated bit mask that specifies which bits from the ``VALUE`` field are applied to the target register.

Given an entry in the ``PERIPHCONF`` array and a bit mask ``M``, IronSide SE performs the following write operation:

``*(REGPTR << 2) = (VALUE & M) | (*(REGPTR << 2) & ~M)``

The register allow list and corresponding bit masks are documented with each IronSide SE release.

Each entry in the ``PERIPHCONF`` array is validated during processing.
To pass validation, ``(REGPTR << 2)`` must point to a register address included in the allow list.

After applying the entry, IronSide SE performs a read-back check: it reads back the register value, applies the bit mask, and compares the result against the masked ``VALUE`` field.

The configuration procedure is aborted if an entry fails either the validation or the read-back check.
If a failure occurs, ``BOOTSTATUS.BOOTERROR`` is set to indicate the error condition, and a description of the failed entry is written to the boot report.

Configuration validation and optimization
=========================================

IronSide SE verifies all configurations in ``UICR`` to ensure that they do not conflict with Nordic-managed resources.
Once verified, IronSide SE stores a digest of the UICR (its referenced memory regions) in the Secure Information Configuration Region (SICR).

Before performing UICR validation on subsequent boots, IronSide SE compares the current digest against the stored value.
If the digests match, the validation step is skipped, as the configuration is known to be valid.

.. _ug_nrf54h20_ironside_se_programming:

Programming
***********

By default, MRAM operates in *Direct Write mode*:

* All memory not reserved by Nordic firmware is accessible with read, write, and execute (RWX) permissions by any domain.
* The Access Port (AP) for the application core is also enabled and available by default, allowing direct programming of all the memory not reserved by Nordic firmware in the default configuration.

.. note::
   Access to external memory (EXMIF) requires a non-default configuration of the ``GPIO.CTRLSEL`` register.

Global domain memory can be protected from write operations by configuring UICR registers.
To remove these protections and disable all other protection mechanisms enforced through UICR settings, perform an ``ERASEALL`` operation.

.. _ug_nrf54h20_ironside_se_debug:

Debugging
*********

IronSide SE provides the ``DEBUGWAIT`` boot command to halt the application core immediately after reset.
This ensures that a debugger can attach and take control from the very first instruction.

When ``DEBUGWAIT`` is enabled, IronSide SE sets ``CPUCONF[APP].CPUWAIT`` when the application core starts.
This prevents the CPU from executing any instructions until a debugger manually releases it.

.. note::
   You can also use the ``cpuconf`` service to set ``CPUWAIT`` when booting other cores.

.. _ug_nrf54h20_ironside_se_protecting:

Protecting the device
*********************

To protect the nRF54H20 SoC in a production-ready device, you must enable the following UICR-based security mechanisms:

* ``UICR.APPROTECT`` - Disables all debug and AP access.
  It restricts debugger and access-port (AP) permissions, preventing unauthorized read/write access to memory and debug interfaces.
* ``UICR.LOCK`` - Freezes non-volatile configuration registers.
  It locks the UICR, ensuring that no further UICR writes are possible without issuing an `ERASEALL` command.
* ``UICR.PROTECTEDMEM`` - Enforces integrity checks on critical code and data.
  It defines a trailing region of application-owned MRAM whose contents are integrity-checked at each boot, extending the root of trust to your immutable bootloader or critical data.
* ``UICR.ERASEPROTECT`` - Prevent bulk erasure of protected memory.
  It blocks all `ERASEALL` operations on NVR0, preserving UICR settings even if an attacker attempts a full-chip erase.


IronSide boot report
********************

The IronSide boot report contains device state information communicated from IronSide SE to the local domains.
It is written to a reserved region in ``RAM20``, which is accessible to the local domain in the default system configuration.
There is one boot report per processor that is booted, either directly by IronSide SE or via the ``CPUCONF`` service.

The boot report contains the following information:

* Magic value
* IronSide SE version
* IronSide SE recovery version
* IronSide SE update status
* UICR error description
* Context data passed to the CPUCONF service
* A fixed amount of random bytes generated by a CSPRNG

Versions
========

The boot report includes version information for both IronSide SE and IronSide SE Recovery.

The regular version format consists of four fields: ``MAJOR.MINOR.PATCH.SEQNUM``, with each field occupying 8 bits.
The first three fields follow semantic versioning, while the ``SEQNUM`` field is a wrapping sequence number that increments by one with each version.
The values ``0`` and ``127`` are reserved for ``SEQNUM``.

An additional version field, referred to as the *extra version*, contains a null-terminated ASCII string with human-readable version information.
This string is informational only, and no semantics should be attached to this part of the version.

IronSide SE update status
=========================

The Secure Domain boot ROM code (SDROM) reports the status of an IronSide SE update request through ``SICR.UROT.UPDATE.STATUS``.
The value of this register is copied to the IronSide SE update status field of the boot report.

UICR error description
======================

This field indicates if any UICR error occurred.

Local domain context
====================

This field is populated by the local domain that is invoking the CPUCONF service.
It is set to `0` for the application core which is booted by IronSide SE.
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

IronSide SE boots the System Controller core first, followed by the application core, in that order.
When booting the application core, IronSide SE does the following:

* Sets ``CPUCONF[APP].INITSVTOR`` to the first 32-bit word of the application-owned memory.
* Reads the reset vector from the second 32-bit word of the application-owned memory.
* If the reset vector is set to ``0xFFFFFFFF``, sets ``CTRL_AP.BOOTSTATUS.BOOTERROR`` to indicate that no firmware is programmed.
* If any other error is encountered during initialization, sets ``CTRL_AP.BOOTSTATUS.BOOTERROR`` accordingly.
* If ``CTRL_AP.BOOTSTATUS.BOOTERROR`` is non-zero, sets ``CPUCONF[APP].CPUWAIT`` to ``1``; otherwise, sets it to ``0``.
* Sets ``CPUCONF[APP].CPUSTART`` to ``1``.

UICR Error handling
===================

If an invalid ``UICR`` configuration is detected, IronSide SE performs the following procedure:

* Stops the allocation procedure.
* Updates the boot report to indicate the ``UICR`` entry (and, if applicable, the array index) that triggered the failure.
* Sets ``CTRL_AP.BOOTSTATUS.BOOTERROR`` to indicate the source of the error.
* Starts the application core with ``CPUCONF[APP].CPUWAIT = 1`` (halted mode).

This allows the error report to be read by a debugger, if the device is not protected.

.. _ug_nrf54h20_ironside_se_cpuconf_service:

IronSide SE CPUCONF service
***************************

The ``IronSide SE CPUCONF`` service enables the application core to trigger the boot of another CPU at a specified address.

Specifically, IronSide SE sets ``NRF_CPUCONF_S->INITSVTOR`` with the address provided to the IronSide call, and then writes ``0x1`` to ``NRF_RADIOCORE_CPUCONF_S->CPUSTART`` to start the target CPU.

When ``cpuwait`` is enabled in the IronSide service call, the target CPU is stalled by writing ``0x1`` to ``NRF_RADIOCORE_CPUCONF_S->CPUWAIT``.
This feature is intended for debugging purposes.

.. note::
   * ``NRF_CPUCONF_S->TASKS_ERASECACHE`` is not yet supported.
   * ``NRF_CPUCONF_S->INITNSVTOR`` will not be supported.
For details about the ``CPUCONF`` peripheral, refer to the nRF54H20 SoC datasheet.

.. _ug_nrf54h20_ironside_se_update_service:

IronSide SE update service
**************************

IronSide SE is updated by the Secure Domain ROM (SDROM), which performs the update operation when triggered by a set of SICR registers.
SDROM verifies and copies the update candidate specified through these registers.

IronSide SE exposes an update service that allows local domains to trigger the update process by indirectly writing to the relevant SICR registers.

The  release ZIP archive for IronSide SE includes the following components:

* A HEX file containing the update candidate for IronSide SE.
* A HEX file for IronSide SE Recovery.
* An application core image that executes the IronSide SE update service to install the update candidate HEX files.

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

The register is written by the Secure Domain Firmware (SDFW) component of IronSide SE at the end of every cold boot sequence.
A value of ``0`` indicates that the SDFW did not complete the boot process.

The following fields are reported by the secure domain firmware:

``FWVERSION``
  Reports the ``SEQNUM`` field of the IronSide SE version.
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

The debugger can instruct IronSide SE to perform an action during the boot sequence.
These actions are called *boot commands*.

Boot commands are issued through the ``CTRLAP.MAILBOX.BOOTMODE`` register and are processed only during a cold boot.
IronSide SE indicates that a boot command was executed by setting the ``CTRLAP.BOOTSTATUS`` register.

The recommended flow for issuing a boot command if the following:

1. Write the command opcode to the OPCODE field in ``CTRLAP.MAILBOX.BOOTMODE``.
#. Trigger a global reset by setting ``CTRLAP.RESET = 1``.

   .. note::
      Any global reset that does not involve a power cycle can be used in place of a CTRLAP reset here.

#. Wait for the command status to be acknowledged in ``CTRLAP.BOOTSTATUS``.
#. Clear the command opcode by writing zeroes to the OPCODE field in ``CTRLAP.MAILBOX.BOOTMODE``.
   As this register is retained across resets, it must be cleared to prevent the command from being re-executed on the next cold boot.

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

``ERASEALL`` commmand
=====================

The ``ERASEALL`` command instructs IronSide SE to erase all application-owned memory.
When executed, the ``ERASEALL`` command performs the following operations:

#. Erases all pages in ``MRAMC10``, from the first page immediately after the Secure Domain Recovery Firmware through the last page in the region.
#. Clears all global domain general-purpose RAM by writing zeros.
#. Erases page 0 of the ``MRAMC10`` NVR (excluding the ``BICR``), which also clears the UICR.
#. Erases all non-NVR pages in ``MRAM11``.

.. note::
  Page 1 of the ``MRAMC10`` NVR is preserved and not erased.

To explicitly permit the ``ERASEALL`` command, disable erase protection by clearing the ``UICR.ERASEPROTECT`` field in the application's UICR.

Erase protection prevents unauthorized device repurposing.
In production-ready devices, enabling both access-port protection (``UICR.APPROTECT``) and erase protection (``UICR.ERASEPROTECT``) prevents the device from re-entering the *configuration* state using a debugger.

.. note::
   When an ``ERASEALL`` request is blocked by ``UICR.ERASEPROTECT``, ``CTRLAP.BOOTSTATUS.CMDERROR`` is set to ``0x1``.

``DEBUGWAIT`` command
=====================

The ``DEBUGWAIT`` command instructs IronSide SE to start the application core in a halted state by setting ``CPUCONF.CPUWAIT = 1``.
This prevents the CPU from executing any instructions until the ``CPUWAIT`` register is cleared by a connected debugger.

Use this command to begin debugging at the very first instruction or to program flash memory safely without concurrent CPU access.

The ``DEBUGWAIT`` command does not define any command-specific values for the ``CTRLAP.BOOTSTATUS.CMDERROR`` field.

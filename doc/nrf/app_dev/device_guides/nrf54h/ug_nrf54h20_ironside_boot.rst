.. _ug_nrf54h20_ironside_se_boot_management:

Managing boot flows
###################

.. contents::
   :local:
   :depth: 2


This page describes the following:

* The :ref:`boot flow of local domains <ug_nrf54h20_ironside_se_local_domain_boot>`
* The :ref:`reset handling of local domains <ug_nrf54h20_ironside_se_local_domain_reset>`
* The :ref:`secondary firmware boot path <ug_nrf54h20_ironside_se_secondary_firmware>`
* The :ref:`IronSide SE boot report <ug_nrf54h20_ironside_se_boot_report>`
* :ref:`Register formats <ug_nrf54h20_ironside_se_boot_register_formats>`
* :ref:`Boot commands <ug_nrf54h20_ironside_se_boot_commands>`
* :ref:`How to configure IronSide SE SPU MRAMC feature <ug_nrf54h20_ironside_se_spu_mramc_feature_configuration>`

.. _ug_nrf54h20_ironside_se_local_domain_boot:

Booting local domains
*********************

This section describes the default boot flow used by |ISE|.
For information about the alternative boot flow that uses the secondary firmware, see :ref:`ug_nrf54h20_ironside_se_secondary_firmware`.

|ISE| boots only the application core CPU.
The application core then triggers the boot of other local domain CPUs, such as the radio core, through the :ref:`ug_nrf54h20_ironside_se_cpuconf_service`.

Application domain boot sequence
================================

When booting the application domain, |ISE| performs the following operations:

* Sets the processor's vector table address to the start of the application-owned memory region.
* Verifies for firmware availability by reading the reset vector from the second 32-bit word of the vector table and comparing it to the erased value (``0xFFFFFFFF``).
* Sets the secure vector table offset register (INITSVTOR) to point to the vector table address.
* Enables the CPU with the appropriate start mode:

  * |ISE| enables the CPU in halted mode if any of the following conditions are met:

    * No firmware is available.
    * Boot errors occurred.
    * The ``DEBUGWAIT`` boot command was issued.
  * Otherwise, |ISE| enables and starts the CPU normally.

* Updates :ref:`CTRL_AP.BOOTSTATUS <ug_nrf54h20_ironside_se_bootstatus_register_format>` and writes the :ref:`boot report <ug_nrf54h20_ironside_se_boot_report>` to reflect any boot errors encountered during the initialization process.

For more information on the boot sequence, see :ref:`ug_nrf54h20_architecture_boot`.

.. _ug_nrf54h20_ironside_se_local_domain_reset:

Reset handling of local domains
*******************************

When a local domain resets, |ISE| detects the event in the RESETHUB peripheral and triggers a global system reset, reported as ``SECSREQ`` in the local domain ``RESETINFO.RESETREAS.GLOBAL``.

Certain local domain reset reasons can trigger a boot into the secondary boot mode.
For more information, see :ref:`ug_nrf54h20_ironside_se_secondary_conf_trigger`.

.. _ug_nrf54h20_ironside_se_secondary_firmware:

Booting a secondary firmware
****************************

The secondary firmware feature provides an alternative boot path that can be triggered implicitly or explicitly.
It can be used for different purposes, some examples are recovery firmware, analysis firmware, and DFU applications in systems that do not use dual banking.

For more information on the boot sequence, see :ref:`ug_nrf54h20_architecture_boot`.

.. note::
   The term "primary firmware" is rarely used when describing the firmware that is booted by default by |ISE|, as it is implicit when the term "secondary" is not specified.

.. note::
   The term "secondary slot" and "secondary image" are used in the MCUboot context.
   This usage is unrelated to the "secondary firmware" described in this section.

.. _ug_nrf54h20_ironside_se_secondary_conf_trigger:

Configuring the secondary firmware
==================================

To enable secondary firmware support, you must complete the following steps:

1. Enable UICR secondary configuration by adding the following to :file:`sysbuild/uicr.conf`:

   .. code-block:: kconfig

      CONFIG_GEN_UICR_SECONDARY=y

#. Create a separate Zephyr application for your secondary firmware (for example in a :file:`secondary/` directory).

#. Include this Zephyr application as an external project in :file:`sysbuild.cmake`:

   .. code-block:: cmake

      ExternalZephyrProject_Add(
        APPLICATION secondary
        SOURCE_DIR ${APP_DIR}/secondary
      )

#. *Configure secondary firmware Kconfig*: The secondary firmware image itself must enable the appropriate Kconfig option to indicate it is a secondary firmware.
   Add the following to your secondary firmware's :file:`prj.conf`:

   .. code-block:: kconfig

      CONFIG_IS_IRONSIDE_SE_SECONDARY_IMAGE=y

#. *Configure secondary firmware placement*: The secondary firmware image must be placed in the DT partition named ``secondary_partition``.
   Add the following to your secondary firmware's :file:`app.overlay`:

   .. code-block:: devicetree

      / {
          chosen {
              zephyr,code-partition = &secondary_partition;
          };
      };

#. *Optional: Configure secondary firmware features*: Additional features can be configured by adding more options to your :file:`sysbuild/uicr.conf`:

   For instance, if you want automatic triggering based on reset reasons, add trigger options:

   .. code-block:: kconfig

      CONFIG_GEN_UICR_SECONDARY_TRIGGER=y
      CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONLOCKUP=y

For more examples of secondary firmware configuration, see the samples in :file:`nrf/samples/ironside_se/`:

* :ref:`secondary_boot_sample` - Basic secondary firmware configuration
* :ref:`secondary_boot_trigger_lockup_sample` - Secondary firmware with automatic triggers

Triggering the secondary firmware
=================================

The secondary firmware can be triggered automatically, through ``CTRLAP.BOOTMODE`` or through an IPC service (``ironside_bootmode`` service).
Any component that communicates with |ISE| over IPC can leverage this service.
Setting bit 5 in ``CTRLAP.BOOTMODE`` will also trigger secondary firmware.

|ISE| automatically triggers the secondary firmware in any of the following situations:

* The integrity check of the memory specified in :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM` fails.
* Any boot failure occurs, such as missing primary firmware or failure to apply UICR.PERIPHCONF configurations.
* If :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER` is enabled, and a UICR-configurable trigger occurs.
  See the following table for UICR-configurable triggers:

.. list-table:: Secondary firmware trigger Kconfig options
   :header-rows: 1
   :widths: auto

   * - Kconfig option
     - Description
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONWDT0`
     - Trigger on Application domain watchdog 0 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONWDT1`
     - Trigger on Application domain watchdog 1 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONLOCKUP`
     - Trigger on Application domain CPU lockup reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCOREWDT0`
     - Trigger on Radio core watchdog 0 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCOREWDT1`
     - Trigger on Radio core watchdog 1 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCORELOCKUP`
     - Trigger on Radio core CPU lockup reset

Protecting the secondary firmware
=================================

The secondary firmware can be protected through integrity checks by enabling :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM`.
The ``PERIPHCONF`` entries for the secondary firmware can also be placed in memory covered by :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM` to create a fully immutable secondary firmware and configuration.

If the integrity check of the memory specified in this configuration fails, the secondary firmware will not be booted.
Instead, |ISE| will attempt to boot the primary firmware, and information about the failure is available in the boot report and boot status.

Updating the secondary firmware
===============================

As with the primary firmware, |ISE| does not facilitate updating the secondary firmware.
The secondary image can be updated by other components as long as :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM` is not set.
Using the secondary firmware as a bootloader capable of validating and updating a second image enables updating firmware in the secondary boot flow while having secure boot enabled through :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM`.

Using the radio core for the secondary firmware
===============================================

By default, the secondary firmware uses the application processor.
The radio core can be used instead by enabling the :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROCESSOR_RADIOCORE` option.

Changing the secondary firmware location
========================================

You can customize the location of the secondary firmware by modifying the ``secondary_partition`` DT partition in both the UICR image and the secondary firmware image.
This is typically done by editing the relevant devicetree source files (such as ``nrf54h20dk_nrf54h20_common.dts`` or board-specific overlay files) in your application and UICR image projects.

.. _ug_nrf54h20_ironside_se_boot_report:

|ISE| boot report
*****************

The |ISE| boot report contains device state information communicated from |ISE| to the local domains.
It is written to a reserved region in RAM20, which is accessible to the local domain in the default system configuration.
There is one boot report per processor that is booted, either directly by |ISE| or through the CPUCONF service.

The boot report contains the following information:

* Magic value
* |ISE| version
* |ISE| recovery version
* |ISE| update status
* UICR error description
* Context data passed to the CPUCONF service
* A fixed amount of random bytes generated by a CSPRNG
* Universal Unique Identifier (UUID) data

Versions
========

The boot report includes version information for both |ISE| and |ISE| Recovery.

The regular version format consists of four fields: ``MAJOR.MINOR.PATCH.SEQNUM``, with each field occupying 8 bits.
The first three fields follow semantic versioning, while the ``SEQNUM`` field is a wrapping sequence number that increments by one with each version.
The values ``0`` and ``127`` are reserved for ``SEQNUM``.

An additional version field, referred to as the *extra version*, contains a null-terminated ASCII string with human-readable version information.
This string is informational only, and no semantics should be attached to this part of the version.

|ISE| update status
===================

The |ISE| boot ROM code (SDROM) reports the status of an |ISE| update request through SICR.UROT.UPDATE.STATUS.
The value of this register is copied to the |ISE| update status field of the boot report.

.. note::
   After an update is installed or attempted, |ISE| resets the update status to ``0xFFFFFFFF`` on the next boot.
   This means that the update status is only valid for a single execution.

When updating |ISE| Recovery, the status will always be ``0xFFFFFFFF``.
Verify that |ISE| Recovery has updated to the desired version by checking the version field of the boot report.

UICR error description
======================

This field indicates if any UICR error occurred.

Local domain context
====================

This field is populated by the local domain that is invoking the CPUCONF service.
It is set to ``0`` for the application core which is booted by |ISE|.
This service is used when one local domain boots another local domain.
The caller can populate this field with arbitrary data that will be made available to the local domain being booted.
Typical examples of data that could be passed include IPC buffer sizes or the application firmware version.
The unused parts of this field are set to 0.

Random data
===========

This field is filled with random data generated by a CSPRNG.
This data is suitable as a source of initial entropy.

UUID data
=========

This field contains 16 bytes that represent the device's unique identity.
The bytes are provided as a stream of 16 raw bytes, rather than in the standard ``8-4-4-4-12`` UUID format.

.. _ug_nrf54h20_ironside_se_boot_register_formats:

Register formats
****************

This section describes the layout and bit fields of the following registers used during the boot process:

* CTRLAP.MAILBOX.BOOTMODE
* CTRLAP.BOOTSTATUS


.. _ug_nrf54h20_ironside_se_bootmode_register_format:

CTRLAP.BOOTMODE register format
===============================

.. _ironside_se_boot_commands:

The format of the CTRLAP.MAILBOX.BOOTMODE register is described in the following table:

+------------------+--------+------------+-----+----------------+----------------+--------+------------+
| Bit numbers      | 31-8   | 7          | 6   | 5              | 4              | 3-1    | 0          |
+------------------+--------+------------+-----+----------------+----------------+--------+------------+
| Field            | N/A    | DEBUGWAIT  | RFU | SECONDARYMODE  | SAFEMODE (ROM) | OPCODE | MODE (ROM) |
+------------------+--------+------------+-----+----------------+----------------+--------+------------+

.. _ug_nrf54h20_ironside_se_bootstatus_register_format:

CTRLAP.BOOTSTATUS register format
=================================

The general format of the CTRLAP.BOOTSTATUS register is described in the following table:

+------------------+-------+-----------+------+
| Bit numbers      | 31-28 | 27-24     | 23-0 |
+------------------+-------+-----------+------+
| Field            | RFU   | BOOTSTAGE | INFO |
+------------------+-------+-----------+------+

Fields marked as RFU (Reserved for Future Use) are set to 0, unless otherwise specified.
The BOOTSTAGE field indicates which component in the boot sequence encountered a failure.

If ``BOOTSTAGE`` is set to ``0xC`` or ``0xD``, the register has the following format:

+------------------+-------+-----------+-------+-----------+-----------+-----------+----------------+-------------+
| Bit numbers      | 31-28 | 27-24     | 23-22 | 21-15     | 14-12     | 11-9      | 8              | 7-0         |
+------------------+-------+-----------+-------+-----------+-----------+-----------+----------------+-------------+
| Field            | RFU   | BOOTSTAGE | RFU   | FWVERSION | CMDOPCODE | CMDERROR  | SECONDARYMODE  | BOOTERROR   |
+------------------+-------+-----------+-------+-----------+-----------+-----------+----------------+-------------+

This field can have one of the following values:

+--------------------+--------------------------------------------------------------+
| BOOTSTAGE value    | Description                                                  |
+====================+==============================================================+
| 0x0                | Unset (reset value)                                          |
+--------------------+--------------------------------------------------------------+
| 0x1                | SysCtrl ROM                                                  |
+--------------------+--------------------------------------------------------------+
| 0x2                | Secure domain ROM                                            |
+--------------------+--------------------------------------------------------------+
| 0xB                | Secure domain firmware with SUIT (major version < 20)        |
+--------------------+--------------------------------------------------------------+
| 0xC                | Secure domain firmware (major version >= 20)                 |
+--------------------+--------------------------------------------------------------+
| 0xD                | Secure domain recovery firmware (major version >= 20)        |
+--------------------+--------------------------------------------------------------+

.. note::
   The value ``0xB`` indicates a boot status error reported by the Secure Domain running a firmware version earlier than 20.
   See :ref:`abi_compatibility` for more information.

The register is written by |ISE| at the end of every cold boot sequence.
A value of ``0`` indicates that |ISE| did not complete the boot process.

The following fields are reported by |ISE|:

FWVERSION
  Reports the SEQNUM field of the |ISE| version.
  The value reported in this field is incremented with each released version of the firmware.
  It can be used to distinguish between firmware versions within a specific release window.

CMDOPCODE
  The opcode of the boot command issued to |ISE| in the CTRLAP.MAILBOX.BOOTMODE register.
  A value of ``0`` indicates that no boot command has been issued.

CMDERROR
  A code indicating the execution status of the boot command specified in CMDOPCODE:

  * A status value of ``0`` indicates that the command was executed successfully.
  * A non-zero value indicates that an error condition occurred during execution of the command.
    The error code ``0x7`` means that an unexpected condition happened that might have prevented the command from executing.
    Other error codes must be interpreted based on the boot command in CMDOPCODE.

SECONDARYMODE
  Indicates whether the secondary firmware was booted.
  A value of ``1`` indicates that the secondary firmware was booted, while a value of ``0`` indicates that the primary firmware was booted.

BOOTERROR
  A code indicating the status of the application domain boot sequence:

  * A status value of ``0`` indicates that the CPU was started normally.
  * A non-zero value indicates that an error condition occurred, preventing the CPU from starting.
    Detailed information about the issue can be found in the boot report.

.. _ug_nrf54h20_ironside_se_boot_commands:

Boot commands
*************

The debugger can instruct |ISE| to perform an action during the boot sequence.
These actions are called *boot commands*.

Boot commands are issued through the CTRLAP.MAILBOX.BOOTMODE register and are processed only during a cold boot.
|ISE| indicates that a boot command was executed by setting the CTRLAP.BOOTSTATUS register.

The recommended flow for issuing a boot command is the following:

1. Write the command opcode to the OPCODE field in CTRLAP.MAILBOX.BOOTMODE.
#. Trigger a global reset by setting CTRLAP.RESET = 1.

   .. note::
      Any global reset that does not involve a power cycle can be used in place of a CTRLAP reset here.

#. Wait for the command status to be acknowledged in CTRLAP.BOOTSTATUS.
#. Clear the command opcode by writing zeroes to the OPCODE field in CTRLAP.MAILBOX.BOOTMODE.
   As this register is retained across resets, it must be cleared to prevent the command from being re-executed on the next cold boot.

See the following table for a summary of the available boot commands:

.. list-table::
   :header-rows: 1

   * - Command name
     - Opcode
     - Description
   * - ``ERASEALL``
     - ``0x1``
     - Erase all user data.
   * - ``DEBUGWAIT``
     - ``0x2``
     - Start the application CPU with ``CPUCONF.CPUWAIT = 1``.

The following chapters describe each command in detail.

.. _ug_nrf54h20_ironside_se_eraseall_command:

``ERASEALL`` command
====================

The ``ERASEALL`` command instructs |ISE| to erase all application-owned memory.
When executed, the ``ERASEALL`` command performs the following operations:

#. Erases all pages in MRAM10, from the first page immediately after the |ISE| Recovery Firmware through the last page in the region.
#. Clears all global domain general-purpose RAM by writing zeros.
#. Erases page 0 of the MRAM10 NVR (excluding the BICR), which also clears the UICR.
#. Erases all non-NVR pages in MRAM11.

.. note::
  Page 1 of the MRAM10 NVR is preserved and not erased.

To explicitly permit the ``ERASEALL`` command, disable erase protection by clearing the :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT` field in the application's UICR.

Erase protection prevents unauthorized device repurposing.
In production-ready devices, enabling both access-port protection (:kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED`) and erase protection (:kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT`) prevents the device from re-entering the *configuration* state using a debugger.

.. note::
   When an ``ERASEALL`` request is blocked by :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT`, CTRLAP.BOOTSTATUS.CMDERROR is set to ``0x1``.

.. _ug_nrf54h20_ironside_se_debugwait_command:

``DEBUGWAIT`` command
=====================

The ``DEBUGWAIT`` command instructs |ISE| to start the application core in a halted state by setting ``CPUCONF.CPUWAIT = 1``.
This prevents the CPU from executing any instructions until the CPUWAIT register is cleared by a connected debugger.

Use this command to begin debugging at the very first instruction or to program flash memory safely without concurrent CPU access.

The ``DEBUGWAIT`` command does not define any command-specific values for the CTRLAP.BOOTSTATUS.CMDERROR field.

.. note::
   You can also use the ``cpuconf`` service to set CPUWAIT when booting other cores.

.. _ug_nrf54h20_ironside_se_spu_mramc_feature_configuration:

Configuring |ISE| SPU MRAMC feature
***********************************

|ISE| configures the SPU.FEATURES.MRAMC registers with default settings for both MRAMC110 and MRAMC111.
Local domains have access to the READY/READYNEXT status registers for monitoring the status of the MRAM controller.
All other MRAMC features (like WAITSTATES and AUTODPOWERDOWN) are managed by |ISE|, with all configurations locked at boot time.

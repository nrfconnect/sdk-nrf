.. _ug_nrf54h20_ironside_se_snapshot:

IronSide SE snapshot services
#############################

.. contents::
   :local:
   :depth: 2

The |ISE| snapshot services help recover selected MRAM content after |ISE| detects MRAM corruption.
This guide is intended for developers of firmware that uses the snapshot services, including customer application firmware, MCUboot, |NCS| system software, and Zephyr components.

Overview
********

Snapshot services capture a configured set of MRAM regions, encrypt the captured data, and store the encrypted data in external memory.
When corruption is detected in a captured region, |ISE| restores the affected MRAM content from the encrypted snapshot.

.. note::
   Snapshot services depend on device support, |ISE| support, and production-time configuration.
   Snapshot functionality must be enabled and configured in one-time programmable (OTP) memory before the snapshot APIs can be used.
   For information about checking support on a device, see :ref:`ug_nrf54h20_ironside_se_snapshot_identify_support`.

|ISE| version requirements
**************************

Use |ISE| v23.7.0+30 or later for snapshot deployments.
For more information, see :ref:`abi_compatibility`.

Snapshot support was introduced over several |ISE| releases:

* |ISE| v23.5.0+28 introduced snapshot services for system-critical regions configured by |ISE|.
* |ISE| v23.6.0+29 added support for application-defined regions configured through ``UICR.SNAPSHOT_REGIONS``.
* |ISE| v23.7.0+30 improved resilience against MRAM corruption and is the minimum version recommended for deployment.

.. _ug_nrf54h20_ironside_se_snapshot_limitations:

Known issues and limitations
****************************

The following issues affect snapshot deployments.

.. _ug_nrf54h20_ironside_se_snapshot_limitation_se0:

BICR CRC is not populated or recalculated (SE0)
===============================================

Some versions of the BICR generator do not populate ``BICR.CRC``.
In addition, |ISE| v23.6.0+29 does not recalculate ``BICR.CRC`` when it writes to BICR.

Affected configurations
  Devices that use an |ISE| version earlier than v23.7.0+30 or a BICR generated without CRC support are affected.
  This includes BICR files generated with |NCS| v3.3.0.

Condition
  A strong magnetic field corrupts BICR, and BICR was generated without the `Zephyr BICR CRC fix <https://github.com/zephyrproject-rtos/zephyr/commit/a6d085d891e28d229cd6c1157bf4a823ec2bac48>`_.

Effect
  The device does not detect the corrupted BICR, attempts to use an invalid BICR configuration, and does not boot.

Workarounds
  * Keep strong magnets at a distance from the device.
  * Use |ISE| v23.7.0+30 or later and populate ``BICR.CRC`` with a valid CRC.
  * Use a BICR generator that includes the Zephyr BICR CRC fix, or apply the fix to the generator.

For information about generating BICR, see :ref:`ug_nrf54h20_custom_pcb_bicr`.

.. _ug_nrf54h20_ironside_se_snapshot_limitation_se1:

|ISE| resilience against MRAM corruption (SE1)
==============================================

Affected versions
  |ISE| versions earlier than v23.7.0+30 are affected.

Condition
  A strong magnetic field corrupts MRAM in a way that is not detected by the affected |ISE| version.

Effect
  |ISE| can read corrupted MRAM and crash before booting the application core.

Workarounds
  * Keep strong magnets at a distance from the device.
  * Update to |ISE| v23.7.0+30 or later.

.. _ug_nrf54h20_ironside_se_snapshot_limitation_se2:

Initial provisioning clears SICR snapshot regions (SE2)
=======================================================

Initial provisioning of |ISE| v23.6.0+29 or later clears configurable snapshot regions that were programmed directly in SICR.
DFU does not clear this configuration.

Affected versions
  |ISE| v23.6.0+29 or later is affected.

Condition
  Configurable snapshot regions are programmed directly in SICR before |ISE| is provisioned.

Effect
  The configured regions are cleared, and the wrong set of MRAM regions is captured.
  The device is then not resilient against corruption in the intended regions.

Workarounds
  * Configure snapshot regions through ``UICR.SNAPSHOT_REGIONS``.
  * If you program SICR directly, provision |ISE| first, and then configure the regions while the device is still in lifecycle state ``EMPTY``.

.. _ug_nrf54h20_ironside_se_snapshot_limitation_se3:

Updating |ISE| Recovery triggers snapshot recovery (SE3)
========================================================

Affected versions
  All |ISE| releases are affected.

Condition
  The :ref:`IronSide SE Recovery component <ug_nrf54h20_ironside_se_deliverables>` is updated on a snapshot-enabled device.

Effect
  The update unintentionally triggers snapshot recovery and restores the device to its pre-update state.
  |ISE| Recovery is therefore effectively immutable on a snapshot-enabled device.

Workaround
  Do not update |ISE| Recovery on a snapshot-enabled device.

.. _ug_nrf54h20_ironside_se_snapshot_limitation_se4:

Even UICR snapshot region count causes a boot loop (SE4)
========================================================

Affected versions
  |ISE| v23.7.0+30 or later is affected.

Condition
  Snapshot regions are configured through ``UICR.SNAPSHOT_REGIONS``, and the configuration contains an even number of regions.

Effect
  Snapshot recovery is triggered unintentionally, and the device enters a boot loop.

Workarounds
  * Configure an odd number of regions through UICR.
  * Configure the regions directly in SICR after provisioning |ISE| and before leaving lifecycle state ``EMPTY``.

.. caution::
   Apply one of these workarounds before transitioning the device from lifecycle state ``EMPTY`` to ``RoT``.
   The transition from ``EMPTY`` to ``RoT`` is permanent and cannot be reversed.

.. _ug_nrf54h20_ironside_se_snapshot_workflow:

Snapshot workflow
*****************

Snapshot setup is a production workflow.
Configure snapshot regions before locking the UICR, and capture the first snapshot after the UICR is locked.

The following setup workflow shows the typical order of operations:

1. Identify the critical MRAM regions that must be recoverable.
#. Configure snapshot regions in UICR during production.
#. Lock the UICR configuration.
#. Invoke the |ISE| snapshot capture service from local-domain firmware.
#. Check the :ref:`boot report <ug_nrf54h20_ironside_se_boot_report>` after the device resets.

During runtime, snapshot behavior follows this flow:

1. The device boots normally after a successful capture.
#. If MRAMC detects corruption during an MRAM read, |ISE| handles the MRAMC interrupt.
#. If the corrupted address is inside a captured snapshot region, |ISE| requests automatic snapshot recovery from SDROM and resets the device.
#. If the corrupted address is outside captured snapshot regions, |ISE| reports the address through the event report.
#. Local-domain firmware can run integrity checks before reading MRAM content and request a snapshot recovery when those checks fail.
#. Local-domain firmware can read the boot report or event report and handle any application-specific recovery.

.. _ug_nrf54h20_ironside_se_snapshot_protection_model:

Snapshot protection model
*************************

Snapshot recovery depends on corruption detection, snapshot region coverage, and application-level integrity checks.
The MRAM Controller (MRAMC), |ISE|, SDROM, System Controller (SysCtrl), and local-domain firmware each have a role in the protection model.

MRAMC corruption detection
==========================

The MRAMC uses error correction code (ECC) for each 16-byte MRAM word.
When MRAMC detects uncorrectable corruption during a read, it triggers an interrupt and a bus fault instead of returning corrupted data.

The MRAMC interrupt is connected to and handled by |ISE|.
When |ISE| handles the interrupt, it checks whether the corrupted address belongs to a captured snapshot region.

Snapshot region handling
========================

|ISE| handles detected corruption differently depending on whether the corrupted address is covered by snapshot:

* If the address is in a captured snapshot region, |ISE| requests snapshot recovery from SDROM and resets the device without local-domain involvement.
* If the address is outside the captured snapshot regions, |ISE| reports the corrupted address to local domains through the |ISE| event report.

The ability to recover the snapshot content is limited to the regions that were included in the snapshot capture.
Local domains remain responsible for recovering corrupted content outside snapshot regions.

Application integrity checks
============================

MRAMC ECC cannot detect every corruption scenario.
Any firmware that reads MRAM content must run integrity checks on MRAM before reading it and request a snapshot recovery if the integrity check fails.
If mounting with no-format in ZMS fails, request a snapshot recovery.
For more information, see :ref:`ug_nrf54h20_zms_no_format_snapshot_recovery`.

The System Controller (SysCtrl) ROM, SDROM, and |ISE| run integrity checks for the MRAM content that they read.
Together, these checks verify the integrity of the following areas:

* FICR
* RICR
* SICR
* BICR
* UICR
* UROT, including |ISE| firmware
* The area defined by UICR.PROTECTEDMEM or UICR.SECONDARY.PROTECTEDMEM, depending on whether secondary firmware is booted
* The area defined by UICR.SECURESTORAGE
* |ISE| internal storage

.. _ug_nrf54h20_ironside_se_snapshot_regions:

Snapshot region configuration
*****************************

Snapshot can capture up to 16 MRAM regions.
Nine regions are predefined by |ISE|, and up to seven regions are configurable during production.

Region limits
=============

All snapshot regions must meet the following requirements:

* The region start address must be aligned to 1 KiB.
* The region size must be aligned to 1 KiB.
* Configurable regions must not overlap each other.
* Configurable regions must not overlap the predefined regions.
* A region must not cross the MRAM10 and MRAM11 boundary at ``0x0E10_0000``.
* Configurable regions can be configured only once during production.

Predefined regions
==================

Eight predefined regions cover the four 2 KiB NVR pages, also known as information pages, for each MRAM instance:

* MRAM10
* MRAM11

The ninth predefined region covers SICR and |ISE| firmware.
This region starts at the beginning of MRAM10, ``0x0E00_0000``, and continues until, but does not include, the immutable bootloader for the application core at ``0x0E03_0000``.

Configurable regions
====================

Configure up to seven application-defined snapshot regions in ``UICR.SNAPSHOT_REGIONS`` during production.
Because of :ref:`SE4 <ug_nrf54h20_ironside_se_snapshot_limitation_se4>`, configure one, three, five, or seven regions when using UICR.

At minimum, include the memory protected by :ref:`UICR.PROTECTEDMEM <ug_nrf54h20_ironside_se_uicr_protectedmem>` in a configurable snapshot region.
This memory starts at ``0x0E03_0000`` and typically contains the immutable bootloader for the application core.
Place UICR.PERIPHCONF and UICR.MPCCONF data in the UICR.PROTECTEDMEM area.

If secure storage is enabled, also include the memory configured by :ref:`UICR.SECURESTORAGE <ug_nrf54h20_ironside_uicr_securestorage>` in a configurable snapshot region.

Recommended region strategies
=============================

Choose snapshot regions based on the content that must be recoverable after MRAM corruption.
Common strategies include the following:

* Capture only required immutable and secure regions, such as UICR.PROTECTEDMEM and UICR.SECURESTORAGE.
* Cover all MRAM with the combined predefined and configurable regions to maximize recoverability.
* Cover all MRAM except areas that contain file systems.

When a region is not covered by snapshot, you must provide your own recovery strategy.
For example, corruption in a file system might require reformatting or another application-specific recovery action, while corruption in firmware might require device firmware update (DFU).
If the device is in a lifecycle state where its ZMS file systems are expected to already be formatted, mount them with no-format.
If mounting with no-format fails, request a snapshot recovery.
For more information, see :ref:`ug_nrf54h20_zms_no_format_snapshot_recovery`.

.. _ug_nrf54h20_ironside_se_snapshot_external_memory:

External memory requirements
****************************

Snapshot stores encrypted copies of captured regions in external memory.
The external memory chip used by snapshot must have a page size of 4 KiB.

Before using snapshot, make sure that the external memory device is available and configured for your board.
This includes the external memory interface, pin assignment, operating frequency, and any UICR configuration required to access the external memory.
For general information about nRF54H20 devicetree and UICR configuration, see :ref:`ug_nrf54h20_configuration`.

Use the following formula to calculate the required external memory capacity:

.. code-block:: text

   2 * sum(round_up(region.size, 4096) for region in snapshot.regions) + 466944

In this formula:

* ``region.size`` is the size of one configurable snapshot region in bytes.
* ``round_up(region.size, 4096)`` rounds each configurable snapshot region up to a 4 KiB boundary.
* ``snapshot.regions`` is the complete set of configurable snapshot regions.
* ``466944`` is the constant external memory requirement for two copies of the predefined snapshot regions and snapshot metadata.

.. _ug_nrf54h20_ironside_se_snapshot_configure_regions:

Configure snapshot regions
**************************

Configure snapshot regions when you prepare the device for production.
The configured regions determine which MRAM content |ISE| can restore during snapshot recovery.

Before configuring snapshot regions, ensure the following:

* You have planned the UICR.PROTECTEDMEM and UICR.SECURESTORAGE locations.
* You have confirmed that the selected regions satisfy the requirements in :ref:`ug_nrf54h20_ironside_se_snapshot_regions`.

Use the UICR generator to configure :ref:`UICR.SNAPSHOT_REGIONS <ug_nrf54h20_ironside_se_uicr_snapshot_regions>`.
For a complete configuration example, see :ref:`ironside_se_snapshot_capture_recover`.

.. caution::
   Configure snapshot regions carefully before locking UICR.
   Configurable snapshot regions can be configured only once during production.
   When using UICR, configure an odd number of regions to avoid the boot loop described in :ref:`ug_nrf54h20_ironside_se_snapshot_limitation_se4`.

To configure snapshot regions, complete the following steps:

1. Add the memory protected by UICR.PROTECTEDMEM to a configurable snapshot region.
#. If secure storage is enabled, add the UICR.SECURESTORAGE memory to a configurable snapshot region.
#. Add any other application-specific MRAM regions that must be recoverable.
#. Lock the UICR configuration as part of production provisioning.

If you configure the regions directly in SICR instead of using UICR, provision |ISE| before programming the SICR region configuration.
Program the SICR configuration while the device is still in lifecycle state ``EMPTY`` and before transitioning to ``RoT``.

After the UICR is locked, capture the first snapshot.
For more information, see :ref:`ug_nrf54h20_ironside_se_snapshot_capture`.

.. _ug_nrf54h20_ironside_se_snapshot_capture:

Capture a snapshot
******************

Capture a snapshot after the snapshot regions are configured and the UICR is locked.
The capture creates the encrypted external-memory copy that |ISE| uses for recovery.

Before capturing a snapshot using the |ISE| snapshot capture service, make sure that the following requirements are met:

* The snapshot regions are configured.
* The UICR is locked.
* The device has sufficient power for the complete capture operation.
* The device is in an environment where MRAM corruption is not expected during capture.

.. caution::
   Do not capture a snapshot when power loss or MRAM corruption can occur.
   The capture operation is not resistant to power loss or magnetic corruption.

To capture a snapshot, complete the following steps:

1. Boot the device.
   Capture shortly after boot because the boot process detects most corruption scenarios.

#. From local-domain firmware, invoke the |ISE| snapshot capture service.
   The device resets during the capture operation.

#. After reset, read the :ref:`boot report <ug_nrf54h20_ironside_se_boot_report>` to check the capture result.

The ``snapshot.status`` field in the boot report indicates whether the capture completed successfully or failed.

.. _ug_nrf54h20_ironside_se_snapshot_recovery:

Handle snapshot recovery
************************

Snapshot recovery is automatic when |ISE| detects recoverable corruption in a captured snapshot region.
Local domains do not need to take part in the restore operation.

.. note::
   Snapshot recovery erases the device MRAM and restores only the captured snapshot regions.
   MRAM content outside the captured snapshot regions is lost.

During snapshot recovery, SDROM performs the following operations:

1. Performs a hardware ERASEALL operation for the MRAM10 bank.
#. Performs a hardware ERASEALL operation for the MRAM11 bank.
#. Performs a hardware page erase for MRAM10 NVR pages 1, 2, and 3.
#. Decrypts the captured snapshot regions from external memory.
#. Writes the decrypted regions back to MRAM.

After recovery, local domains can detect that snapshot recovery occurred by reading the ``snapshot.status`` field in the :ref:`boot report <ug_nrf54h20_ironside_se_boot_report>`.

.. _ug_nrf54h20_ironside_se_snapshot_event_report:

Handle corruption outside snapshot regions
******************************************

|ISE| reports detected uncorrectable MRAM corruption outside snapshot regions through the |ISE| event report.
The event report is written asynchronously by |ISE| when the event occurs.

For snapshot, the relevant event is a read of an MRAM address that is not included in a captured snapshot region.
The event report contains the corrupted address and is written to RAM that local domains can read.
|ISE| also rings a bell for each domain.

When local-domain firmware receives this event, it must choose an application-specific recovery action.
The snapshot mechanism does not recover MRAM outside the captured regions.

Typical recovery actions include the following:

* Reformat a corrupted file system.
* Trigger DFU when firmware corruption is detected.

.. _ug_nrf54h20_ironside_se_snapshot_identify_support:

Identify snapshot support
*************************

You can identify snapshot support in two ways:

* In lifecycle state ``EMPTY``, read FICR directly.
* In lifecycle state ``RoT`` or ``DEPLOYED``, check the |ISE| boot report in RAM.

Identify support in lifecycle state EMPTY
=========================================

To identify whether an nRF54H20 device in lifecycle state ``EMPTY`` supports the |ISE| snapshot service, read the FICR directly:

.. code-block:: console

   nrfutil device read --traits jlink --core secure --direct --address 0x0FFFE054

If the value is ``0x0005420B``, the device supports the |ISE| snapshot service.

To identify whether a supported device is an engineering sample or a production device, read the following FICR address:

.. code-block:: console

   nrfutil device read --traits jlink --core secure --direct --address 0x0FFFE008 --bytes

If the value is ``1``, the device is an engineering sample release 1 (Eng1) device.
If the value is ``2``, the device is a production device.

Identify support in lifecycle state RoT or DEPLOYED
===================================================

To identify whether a device in lifecycle state ``RoT`` or ``DEPLOYED`` supports snapshot, check the |ISE| boot report in RAM.
The ``snapshot.enabled`` field is ``0`` when snapshot is disabled and non-zero when snapshot is enabled.
The ``snapshot.status`` field reports the result of the most recent snapshot operation.

For the boot report fields that indicate snapshot support, see the :file:`zephyr/modules/hal_nordic/ironside/se/include/ironside/se/boot_report.h` header file.

Related documentation
*********************

For more information, see the following pages:

* :ref:`ug_nrf54h20_ironside_se_uicr` for UICR configuration.
* :ref:`ug_nrf54h20_ironside_se_protecting` for production device protection.
* :ref:`ug_nrf54h20_ironside_se_secure_storage` for secure storage configuration.
* :ref:`ug_nrf54h20_ironside_se_boot_report` for boot report contents.
* :ref:`ug_nrf54h20_ironside_se_update` for |ISE| update information.
* :ref:`ironside_se_snapshot_capture_recover` for a snapshot configuration and capture example.
* :ref:`abi_compatibility` for |ISE| ABI and release compatibility information.

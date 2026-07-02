.. _ironside_se_snapshot_suspend_to_ram:

IronSide SE: Snapshot and suspend-to-RAM
########################################

.. contents::
   :local:
   :depth: 2

The Snapshot and suspend-to-RAM sample demonstrates the |ISE| snapshot capture and recovery features, followed by a suspend-to-RAM low-power idle loop.
The snapshot feature helps recover the content of the MRAM if it becomes corrupted.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

The sample also requires |ISE| version 23.7.0+30 or later.
The snapshot feature requires an nRF54H20 device that has snapshot support enabled in OTP during production.
This is the case for the nRF54H20 D00 revision (build code ``D00``) and later.
Earlier revisions cannot run the snapshot phase.
Because snapshot support depends on the OTP configuration, confirm support on your device as described in :ref:`ug_nrf54h20_ironside_se_snapshot_identify_support`.
In lifecycle state ``EMPTY``, read the Factory Information Configuration Registers (FICR): the value ``0x0005420B`` at address ``0x0FFFE054`` indicates support.
In lifecycle state ``RoT`` or ``DEPLOYED``, check the |ISE| boot report.
For more information, see :ref:`abi_compatibility` and :ref:`ug_nrf54h20_ironside_se_snapshot`.

Overview
********

The sample performs multiple snapshot capture and recovery operations, followed by a suspend-to-RAM low-power idle loop.

In each cycle, the sample completes the following steps:

1. Validates the boot report.
#. Increments an |ISE| NV counter (:ref:`ug_nrf54h20_ironside_se_counter_service`).
#. Logs the snapshot status.
#. Executes a *capture → recovery → cold* reboot sequence.

To keep execution bounded and repeatable, the sequence stops when the counter reaches ``SNAPSHOT_MAX_CYCLES`` in :file:`src/main.c`.
After reaching this limit, the sample enters the low-power idle loop.

During the low-power idle loop, the application core sleeps with suspend-to-RAM enabled and wakes up periodically.
The empty idle image keeps the radio core idle, which lets the whole SoC reach low power.

The sample uses the same configuration as the multicore idle benchmark (:file:`tests/benchmarks/multicore/idle`, the ``s2ram`` scenario).
It owns the radio core and runs an empty idle image on it, enables suspend-to-RAM, and disables serial, console, and GPIO so the whole SoC reaches low power.

UICR configuration
==================

The sample configures the following UICR settings:

* *UICR.LOCK* - Not written by :file:`sysbuild/uicr.conf`.
  After each boot, the sample reads the NV counter.
  If the value is ``0``, it calls ``uicr_deploy_lock_contents()`` and performs a cold reboot.

* *PROTECTEDMEM* - Protects ``cpuapp_boot_partition`` and ``periphconf_partition`` (72 KB).
  This layout is consistent with other |ISE| samples, where ``periphconf`` is placed immediately after the boot partition.

* *UICR.SNAPSHOT.REGIONS* - Defines snapshot regions.
  Region 0 is located at the physical address ``0x0E030000`` (72 KB) and covers the boot and ``periphconf`` partitions.
  Additional regions defined in :file:`sysbuild/uicr.conf` cover secure storage and the radio core firmware.

Building and running
********************

.. |sample path| replace:: :file:`samples/ironside_se/snapshot_suspend_to_ram`

.. include:: /includes/build_and_run.txt

To reset the persistent state, including all counters, before starting a clean execution, run the following command:

.. code-block:: console

   west flash --recover

Testing
*******

The sample does not produce console output because the serial port is disabled to reduce power consumption.
After programming the sample to your development kit, complete the following steps to test it:

1. Connect a current measurement tool, such as the Power Profiler Kit II (PPK2), to power the device and measure its current.
#. Reset the development kit.
#. Observe the current consumption as the sample locks UICR on the first boot, then runs several snapshot capture and recovery cycles (the NV counter increments on each boot, each cycle ends in a cold reboot).
#. After ``SNAPSHOT_MAX_CYCLES`` boots, the sample enters the low-power idle loop, where the current drops to the low-power floor and shows periodic wake-ups.

Dependencies
************

This sample uses the following |NCS| subsystems:

* |ISE| snapshot service - Captures and recovers configured MRAM and NVR regions
* |ISE| counter service - Tracks the boot cycle for the bounded capture/recovery flow
* Sysbuild - Builds the application, radio core idle, and UICR images together
* UICR generation - Configures PROTECTEDMEM and snapshot regions in :file:`sysbuild/uicr.conf`
* :ref:`Power management <zephyr:pm-guide>` - Suspend-to-RAM low-power idle on both cores

In addition, it uses the following Zephyr subsystem:

* :ref:`Kernel <kernel>` - Provides basic system functionality and threading

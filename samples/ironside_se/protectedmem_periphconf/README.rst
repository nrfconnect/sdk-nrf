.. _protectedmem_periphconf_sample:

Protected Memory with PERIPHCONF Partition
##########################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to protect the PERIPHCONF partition using UICR.PROTECTEDMEM.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample relocates the ``periphconf_partition`` right after ``cpuapp_boot_partition`` and configures PROTECTEDMEM to cover both partitions.
The primary firmware uses :c:func:`flash_write` to write a test pattern to the last write block of ``protectedmem_partition`` and reboots after 1 second.
|ISE| detects the PROTECTEDMEM integrity failure on the next boot and launches the secondary firmware instead of the primary application.

The secondary firmware reads the boot report to confirm it booted due to a PROTECTEDMEM integrity failure, uses :c:func:`flash_erase` to erase the corrupted region, and reboots after 1 second.
On the following boot, the integrity check passes and |ISE| launches the primary firmware again.

UICR.SECONDARY.PROTECTEDMEM is also configured, providing the secondary firmware with the same protection.

Building and running
*********************

.. |sample path| replace:: :file:`samples/ironside_se/protectedmem_periphconf`

.. include:: /includes/build_and_run.txt

Testing
*******

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the development kit.

The following is the expected output from the primary firmware:

.. code-block:: console

   Writing test pattern to protected memory...
   Rebooting after 1s. Secondary firmware should boot if protection is working.

The following is the expected output from the secondary firmware:

.. code-block:: console

   Secondary: Secondary firmware booted
   Secondary: Booted because of PROTECTEDMEM failure
   Secondary: Reverting test pattern write to protected memory...
   Secondary: Rebooting after 1s. Primary firmware should boot again since the corruption was fixed.

The sample alternates indefinitely between the primary and secondary firmware.

Configuration
*************

The sample uses the following key configurations:

Shared Memory Map
  The ``sysbuild/nrf54h20dk_nrf54h20_memory_map.dtsi`` file defines the partition layout for the board.
  It relocates ``periphconf_partition`` to be placed right after ``cpuapp_boot_partition`` at offset 0x40000, and defines a ``protectedmem_partition`` subpartition grouping both.
  A matching ``secondary_protectedmem_partition`` covers the secondary firmware's boot and periphconf partitions.
  Both ``app.overlay`` and ``secondary/app.overlay`` include this file, and ``sysbuild/uicr.overlay`` applies it to the UICR image so all images see the same partition layout.

Kconfig Configuration
  The ``sysbuild/uicr.conf`` file enables PROTECTEDMEM (``CONFIG_GEN_UICR_PROTECTEDMEM=y``), secondary firmware (``CONFIG_GEN_UICR_SECONDARY=y``), and SECONDARY.PROTECTEDMEM (``CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM=y``).
  The PROTECTEDMEM size is derived automatically from the ``protectedmem_partition`` devicetree node.
  Both images enable ``CONFIG_FLASH=y`` to use the Zephyr flash driver API.

Secondary Firmware
  The ``secondary/`` directory contains the secondary firmware that boots when the PROTECTEDMEM integrity check fails.
  It reads the :c:struct:`ironside_se_boot_report` to confirm the boot reason, then uses :c:func:`flash_erase` to erase the last write block of ``protectedmem_partition``, restoring the region to its erased state and allowing the integrity check to pass on the next boot.

Flash Ordering
  ``sysbuild.cmake`` uses ``sysbuild_add_dependencies(FLASH ...)`` to ensure the UICR image is programmed last.

Dependencies
************

This sample uses the following |NCS| subsystems:

* UICR generation - Configures UICR.PROTECTEDMEM and UICR.SECONDARY.PROTECTEDMEM to protect the memory regions and enables secondary firmware boot
* Sysbuild - Enables building the UICR image and secondary firmware with the protected memory configuration

In addition, it uses the following Zephyr subsystems:

* :ref:`Kernel <kernel>` - Provides basic system functionality and threading
* :ref:`Console <console>` - Enables UART console output for debugging and user interaction
* Devicetree - Defines the partition layout
* :ref:`Flash <flash_api>` - Provides :c:func:`flash_write` and :c:func:`flash_erase` for accessing MRAM

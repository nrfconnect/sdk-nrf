.. _protectedmem_periphconf_sample:

Protected Memory with PERIPHCONF Partition
##########################################

.. contents::
   :local:
   :depth: 2

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

This sample demonstrates how to protect the ``periphconf_partition`` using UICR.PROTECTEDMEM.
The sample relocates ``periphconf_partition`` to be placed right after ``cpuapp_boot_partition`` and configures PROTECTEDMEM to cover both partitions.
When protected memory is modified, the integrity check fails on the next boot, causing IronSide SE to boot a secondary firmware instead of the main application.

Building and running
*********************

.. |sample path| replace:: :file:`samples/ironside_se/protectedmem_periphconf`

.. include:: /includes/build_and_run_ns.txt

To build the sample, run the following command:

.. code-block:: console

   west build -b <board> samples/ironside_se/protectedmem_periphconf

To program the sample on the device, run the following command:

.. code-block:: console

   west flash --recover

Testing
*******

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.

The application writes a test pattern to protected memory and then reboots.
On the next boot, if protection is working correctly, the secondary firmware will boot instead of the main application, indicating that the integrity check detected the modification.

Configuration
*************

The sample uses the following key configurations:

Device Tree Overlay
===================

The ``app.overlay`` file relocates the ``periphconf_partition`` to be placed right after ``cpuapp_boot_partition`` at offset 0x40000.
The ``sysbuild.cmake`` file applies this same overlay to the UICR image so both images see the same partition layout.

Kconfig Configuration
=====================

The ``sysbuild/uicr.conf`` file configures the PROTECTEDMEM size to 72KB (73728 bytes) to cover both ``cpuapp_boot_partition`` (64KB) and ``periphconf_partition`` (8KB).

Secondary Firmware
==================

The sample includes a secondary firmware (in the ``secondary/`` directory) that boots automatically when the PROTECTEDMEM integrity check fails.
The secondary firmware is enabled via ``CONFIG_GEN_UICR_SECONDARY=y`` in ``sysbuild/uicr.conf`` and is built as part of the sysbuild process (configured in ``sysbuild.cmake``).

Dependencies
************

This sample uses the following |NCS| subsystems:

* UICR generation - Configures UICR.PROTECTEDMEM to protect the memory region and UICR.SECONDARY to enable secondary firmware boot
* Sysbuild - Enables building the UICR image and secondary firmware with the protected memory configuration

In addition, it uses the following Zephyr subsystems:

* :ref:`Kernel <kernel>` - Provides basic system functionality and threading
* :ref:`Console <console>` - Enables UART console output for debugging and user interaction
* Device Tree - Defines the partition layout

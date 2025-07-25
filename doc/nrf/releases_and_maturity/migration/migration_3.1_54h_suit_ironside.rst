.. _migration_3.1_54h_suit_ironside:

Migration from nRF Connect SDK 3.0 (SUIT) to 3.1 (IronSide SE) on nRF54H20 SoC
==============================================================================

.. contents::
   :local:
   :depth: 2

This document explains how to migrate your existing |NCS| v3.0 application for the nRF54H20 SoC running SUIT to the |NCS| v3.1 using IronSide SE.

To follow this guide, you must meet the following prerequisites:

* You have a working |NCS| 3.0 application for the nRF54H20 SoC using SUIT.
* You have installed |NCS| 3.1 and its toolchain.

.. note::
   To program IronSide SE on your nRF54H20 SoC-based device, your device must be in lifecycle state (LCS) ``EMPTY``.
   Devices using SUIT in LCS RoT cannot be transitioned back to LCS EMPTY.

Breaking Changes
================

The following is a summary of the breaking changes that apply when migrating applications:

* SUIT support is removed in |NCS| 3.1.
  The Secure Domain now runs IronSide SE, which uses MCUboot for device firmware update.
* IronSide SE uses a new UICR format and moves the peripheral configuration into a dedicated MRAM partition.
* Shared memory (IPC) is relocated to a fixed RAM-20 region.
  You no longer reserve it manually.
* DFU support moves from SUIT to MCUboot.
  You must enable and configure MCUboot in your project.

Update prj.conf
---------------

Remove the following SUIT-specific Kconfig options:

* :kconfig:option:`CONFIG_SUIT`
* :kconfig:option:`CONFIG_SUIT_SECURE_DOMAIN`

Enable MCUboot for device firmware update (DFU) in the :file:`sysbuild.conf` file as follows:

* Set :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` to ``y``.


Update devicetree files
-----------------------

You must update your devicetree files as follows:

* Remove the old UICR partition.
  In your board's DTS overlay, remove any node that defined the ``uicr`` partition.

* Add the IronSide ``peripconf`` partition.
  IronSide expects a blob of address-value pairs called ``peripconf`` stored in the MRAM.

* Remove IPC-shared-memory reservation.
  As IronSide relocates the IPC buffer to a fixed RAM20 address, you can delete any manual reservation in RAM0.
  Refer to the `Memory map changes`_ section.

Update peripheral configuration (peripconf)
-------------------------------------------

The new UICR format no longer holds peripheral initial values.
You must generate a peripconf blob at build time.

* Zephyr build applies an ``nrf_rpc_peripconf`` script.
* The script reads ``peripconf`` nodes in the DTS.
* It outputs a binary blob and embeds it into the MRAM partition.

You do not need to modify your application code.
You only need to ensure the DTS partition exists.

Memory protection and isolation
-------------------------------

IronSide SE currently grants full memory-access permissions to both application and radio domains by default.
Delete any UICR settings related to the following:

  * IPC location
  * Secure-Domain offsets
  * Partition lock bits

Memory map changes
------------------

With IronSide SE, the memory map changed as follows:

* The application core firmware now always starts at address ``0xE03_0000``, which is the first address in ``MRAM00`` immediately following the IronSide firmware.
* The radio core is not started automatically.
* Nordic-reserved partitions in ``MRAM10`` and ``RAM0x`` have been removed.
* IPC buffers toward the Secure Domain are relocated to fixed addresses in ``RAM20``.
  Memory previously reserved in ``RAM0x`` for IPC can now be repurposed.
* The devicetree no longer uses the ``nordic,owned-memory`` or ``nordic,owned-partitions`` compatibles.
  Refer to the `nRF54H20 DK memory map`_ for details.

To enable ``UICR/PERIPHCONF`` generation, ensure a DTS partition labeled ``peripconf_partition`` exists with sufficient size (for example, 8 KBs) to embed the generated address-value blob.
Move all partition definitions directly under the ``mram1x`` partitions node.

DFU support with MCUboot
------------------------

IronSide SE drops SUIT in favor of MCUboot.
To migrate DFU, do the following:

* Remove SUIT-specific Kconfig symbols from both :file:`prj.conf` and :file:`sysbuild.conf` files.
* Delete SUIT manifest templates, typically located in the :file:`suit` directory.
* Choose one of the supported MCUboot bootloader modes.
* Rename ``dfu_partition`` to ``cpuapp_slot1_partition``, or remove it if your chosen mode does not require a DFU slot.
* If your application uses the radio core:

  * Add the radio image to the updateable image list by calling the ``UpdateableImage_Add`` function in your CMake build.
  * Enable the :kconfig:option:`CONFIG_SOC_NRF54H20_CPURAD_ENABLE` Kconfig option to ensure the radio core starts at runtime.

* Remove recovery and companion images, as MCUboot no longer supports them.

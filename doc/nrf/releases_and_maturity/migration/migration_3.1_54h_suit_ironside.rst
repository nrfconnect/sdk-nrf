:orphan:

.. _migration_3.1_54h_suit_ironside:

Migrating applications from |NCS| v3.0.0 (SUIT) to |NCS| v3.1.0 (IronSide SE) on the nRF54H20 SoC
#################################################################################################

.. contents::
   :local:
   :depth: 2

This document explains how to migrate your existing |NCS| v3.0.0 application for the nRF54H20 SoC running SUIT to the |NCS| v3.1.0 using IronSide SE.

To follow this guide, you must meet the following prerequisites:

* You have a working the |NCS| v3.0.0 application for the nRF54H20 SoC using SUIT.
* You have installed the |NCS| v3.1.0 and its toolchain.
  For more information, see :ref:`install_ncs`.

Moreover, to program your modified application on the nRF54H20 SoC, your nRF54H20-based device must be provisioned with the relevant nRF54H20 SoC binaries version.
For more information, see :ref:`abi_compatibility`.

.. caution::
   To program the nRF54H20 SoC binaries based on IronSide SE on your nRF54H20 SoC-based device, your device must be in lifecycle state (LCS) ``EMPTY``.
   Devices already provisioned using SUIT-based SoC binaries and in LCS ``RoT`` cannot be transitioned back to LCS ``EMPTY``.

   For more information on provisioning devices, see :ref:`ug_nrf54h20_gs_bringup`.
   For more information on the nRF54H20 SoC binaries, see :ref:`abi_compatibility`.

Breaking changes
****************

The following is a summary of the breaking changes that apply when migrating applications:

* SUIT support is removed in the |NCS| v3.1.0.
  The Secure Domain now runs IronSide SE, which is agnostic to the device firmware update (DFU) mechanism used.
  DFU is handled by the local domain (for example, using MCUboot).
* IronSide SE uses a new UICR format and moves the peripheral configuration into a dedicated MRAM partition.
* The shared memory used for communication with the Secure Domain is now located in a fixed RAM-20 region.
  You no longer reserve it manually.
* DFU support moves from SUIT to MCUboot.
  You must enable and configure MCUboot in your project.

Update prj.conf
===============

To update :file:`prj.conf`, complete the following steps:

1. Remove the following SUIT-specific Kconfig options:

   * :kconfig:option:`CONFIG_SUIT`
   * :kconfig:option:`CONFIG_SUIT_SECURE_DOMAIN`
   * :kconfig:option:`SB_CONFIG_SUIT_ENVELOPE`
   * :kconfig:option:`CONFIG_SUIT_ENVELOPE_TEMPLATE_FILENAME`

#. Disable SUIT-specific SMP extensions:

   * :kconfig:option:`CONFIG_MGMT_SUITFU`
   * :kconfig:option:`CONFIG_MGMT_SUITFU_GRP_SUIT`

#. Disable legacy RC code encoding (:kconfig:option:`CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR`) as it is no longer needed.
#. Enable MCUboot for device firmware update (DFU) in the :file:`sysbuild.conf` by setting :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` to ``y``.
   If the application uses a custom memory map, include the map in the MCUboot overlay (for example, :file:`sysbuild/mcuboot.overlay`).
   If the customized MCUboot overlay is defined, it must also include the following lines:

   .. code-block::

      / {
         chosen {
            zephyr,code-partition = &boot_partition;
         };
      };

Update devicetree files
=======================

To update your devicetree files, complete the following steps:

1. Remove the old UICR partition.
   In your board's DTS overlay, remove any node that defined the ``uicr`` partition.

#. Add the PERIPHCONF array.
   In your devicetree, under the ``mram1x`` partitions node, define a partition node labeled ``periphconf_partition`` with a size of at least 8 KB to embed the generated address-value blob.

#. Remove IPC-shared-memory reservation.
   As IronSide relocates the IPC buffer to a fixed RAM20 address, you can delete any manual reservation in RAM0.
   Refer to the `Memory map changes`_ section.

#. Update IPC configuration for IronSide SE.
   The shared memory for communication with the Secure Domain now uses fixed addresses in ``RAM20``.
   A single memory region is used for both RX and TX operations.
   The IPC nodes use the ``nordic,ironside-call`` compatible and communicate using the new *IronSide Calls* IPC driver.

   For custom board devicetree files, you can copy the IPC configuration from the nRF54H20 DK reference implementation.
   The devicetree defines the shared memory region and IPC nodes as follows:

   .. code-block:: dts

      // Shared memory region in RAM20
      cpusec_cpuapp_ipc: memory@2f88f000 {
          reg = <0x2f88f000 DT_SIZE_K(4)>;
      };

   .. code-block:: dts

      // IPC nodes using IronSide calls driver
      cpusec_cpuapp_ipc_tx: ipc@deadbeef {
          compatible = "nordic,ironside-call";
          mboxes = <&cpuapp_cpusec_ipc 0>, <&cpuapp_cpusec_ipc 1>;
          mbox-names = "rx", "tx";
          memory-region = <&cpusec_cpuapp_ipc>;
          status = "okay";
      };

   .. code-block:: dts

      cpusec_cpuapp_ipc_rx: ipc@deadbeef {
          compatible = "nordic,ironside-call";
          mboxes = <&cpusec_cpuapp_ipc 2>, <&cpuapp_cpusec_ipc 3>;
          mbox-names = "rx", "tx";
          memory-region = <&cpusec_cpuapp_ipc>;
          status = "okay";
      };

#. Remove the SUIT recovery partitions (``cpuapp_recovery_partition`` and ``cpurad_recovery_partition``).

Update PERIPHCONF
=================

The new UICR format no longer holds peripheral configuration initial values.
You must generate a PERIPHCONF blob at build time.

The Zephyr build invokes the :file:`gen_uicr.py` script (:file:`soc/nordic/common/uicr/gen_uicr.py` in the Zephyr tree) using ``nrf-regtool`` in the |NCS|'s implementation of :ref:`configuration_system_overview_sysbuild`.
When the following Kconfig options are set:

  * :kconfig:option:`CONFIG_NRF_HALTIUM_GENERATE_UICR` to ``y``
  * :kconfig:option:`CONFIG_NRF_HALTIUM_UICR_PERIPHCONF` to ``y``

the script does the following:

  1. It reads the ``periphconf_partition`` node in the devicetree to discover the partition's address and size.
  #. It extracts the address/value pairs from the ``PERIPHCONF`` section of the Zephyr ELF image.
  #. It generates two Intel HEX files:

    * :file:`uicr.hex` - The new UICR entries
    * :file:`periphconf.hex` - The MRAM-resident ``PERIPHCONF`` blob

Both HEX files must be programmed alongside your firmware image.
``west flash`` handles this automatically.

You do not need to modify your application code.
You only need to ensure the DTS partition exists.

Memory protection and isolation
===============================

IronSide SE currently grants full memory-access permissions to both application and radio domains by default.
Delete any UICR settings related to the following:

* Secure Domain IPC buffer location
* Secure-Domain offsets
* Partition lock bits

Memory map changes
==================

With IronSide SE, the memory map changed as follows:

* The application core firmware now always starts at address ``0xE03_0000``, which is the first address in ``MRAM00`` immediately following the IronSide firmware.
  If the application uses MCUboot, the application starts at address ``0xE04_0000``.
  The default location for the radio firmware is now ``0xE09_2000``.
* Nordic-reserved partitions in ``MRAM11`` and ``RAM0x`` have been removed.
* IPC buffers toward the Secure Domain are relocated to fixed addresses in ``RAM20``.
  Memory previously reserved in ``RAM0x`` for IPC can now be repurposed.
* The devicetree no longer uses the ``nordic,owned-memory`` or ``nordic,owned-partitions`` compatibles.
  Remove memory access groups, such as ``cpuapp_rx_partitions``, ``cpurad_rx_partitions``, ``cpuapp_rw_partitions`` and define partitions under the ``partitions`` node under the ``mram1x`` node.
  Refer to the `nRF54H20 DK memory map`_ for details.

To enable ``UICR/PERIPHCONF`` generation, ensure a DTS partition labeled ``periphconf_partition`` exists with sufficient size (for example, 8 KBs) to embed the generated address-value blob.

DFU support with MCUboot
========================

IronSide SE drops SUIT in favor of MCUboot.
To migrate the DFU solution, complete the following steps:

1. Remove SUIT-specific Kconfig symbols from both :file:`prj.conf` and :file:`sysbuild.conf` files.
#. Delete SUIT manifest templates, typically located in the :file:`suit` directory.
#. Choose one of the supported MCUboot bootloader modes.
#. If your chosen mode does not require a DFU slot, remove the ``dfu_partition``.
   Otherwise, split the ``dfu_partition`` into ``cpuapp_slot1_partition`` and ``cpurad_slot1_partition``.
   These partitions must match the size of their counterparts (``cpuapp_slot0_partition`` and ``cpurad_slot0_partition``, respectively).
#. If your application uses the radio core:

  a. Add the radio image to the updateable image list by calling the ``UpdateableImage_Add`` function in your CMake build.
  b. Enable the :kconfig:option:`CONFIG_SOC_NRF54H20_CPURAD_ENABLE` Kconfig option to ensure the radio core starts at runtime.

#. Remove recovery and companion images, as MCUboot no longer supports them.

Other changes
=============

The radio core is no longer started automatically.

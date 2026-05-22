.. _dfu_conf:

Configuring sysbuild and partitions
###################################

.. contents::
   :local:
   :depth: 2

When enabling DFU support in Bluetooth Mesh samples, you need to configure the non-volatile memory layout to accommodate the bootloader (MCUboot), the application image, a secondary slot for the update image, and a storage partition for settings.
This page describes how to configure DFU for Bluetooth Mesh samples using devicetree overlays and sysbuild.

Memory partitioning
*******************

For DFU to work, the non-volatile memory must be partitioned into the following regions:

* Boot partition (``mcuboot``) - Contains the MCUboot bootloader image.
* Slot 0 (``image-0``) - Contains the active application image.
* Slot 1 (``image-1``) - Contains the candidate (update) image that MCUboot swaps with Slot 0 during an upgrade.
* Storage partition (``storage``) - Used for non-volatile settings storage (NVS or ZMS).

The partitioning depends on the SoC family, the available non-volatile memory size, and whether an external flash is used for the secondary slot or settings storage.

.. note::
   The default partition layout defined in the SoC devicetree files may not leave enough space for both image slots.
   The DFU-enabled Bluetooth Mesh samples provide custom overlay files that redefine the partition layout to fit all required partitions.

Using external flash for the secondary slot
===========================================

When internal non-volatile memory is insufficient to hold both image slots, the secondary image slot (Slot 1) can be placed on an external flash device.
This maximizes the space available for the application in the internal non-volatile memory.
The :ref:`ble_mesh_dfu_target`, :ref:`ble_mesh_dfu_distributor` and :ref:`bluetooth_mesh_light` samples demonstrate this ability.
See the respective sample documentation for details on enabling external flash for DFU.

Using external flash for settings storage
=========================================

On devices with limited internal non-volatile memory, you can also move the settings storage partition to an external flash device.
Refer to any mesh samples that demonstrate this configuration.

MCUboot overlay compatibility
=============================

MCUboot requires its own devicetree overlay that defines the same partition layout as the application overlay, and additionally sets the ``zephyr,code-partition`` chosen node to the boot partition.
The partition offsets and sizes in the MCUboot overlay must exactly match those in the application overlay.
For more information on MCUboot partition requirements and configuration, see the :ref:`mcuboot_ncs` documentation and the :doc:`mcuboot:design` documentation.

.. _dfu_dts_conf_sysbuild:

Sysbuild configuration
**********************

DFU in Bluetooth Mesh samples uses sysbuild to build MCUboot alongside the application.
To enable MCUboot in the :file:`sysbuild.conf` file, set the :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` Kconfig option to ``y``.
For the :ref:`ble_mesh_dfu_target` and :ref:`ble_mesh_dfu_distributor` samples, the following additional options generate the DFU zip archive with Bluetooth Mesh metadata:

.. code-block:: kconfig

   SB_CONFIG_BOOTLOADER_MCUBOOT=y
   SB_CONFIG_DFU_ZIP=y
   SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA=y
   SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA_FWID_MCUBOOT_VERSION=y

.. _dfu_dts_conf_customizing:

Customizing the memory layout
*****************************

To customize the memory layout for your application, perform the following steps:

1. Determine the total available non-volatile memory for your target SoC.
#. Allocate space for the MCUboot bootloader (typically 48-56 KB).
#. Divide the remaining internal non-volatile memory between Slot 0, Slot 1, and storage, ensuring that Slot 0 and Slot 1 are the same size.
#. If the application image does not fit in half the remaining non-volatile memory, consider placing Slot 1 on external flash.
#. Create a board-specific overlay file in the :file:`boards/` directory of your sample.
#. Create a matching MCUboot overlay in the :file:`sysbuild/mcuboot/boards/` directory with the same partition layout and the ``zephyr,code-partition`` chosen node.

Example: Adding DFU support to a custom Bluetooth Mesh application
==================================================================

To enable point-to-point DFU over Bluetooth Low Energy for a custom Bluetooth Mesh application on an ``nrf54l15dk/nrf54l15/cpuapp`` board target, perform the following steps:

1. Create a board overlay :file:`boards/nrf54l15dk_nrf54l15_cpuapp.overlay` file:

   .. code-block:: devicetree

      /delete-node/ &boot_partition;
      /delete-node/ &slot0_partition;
      /delete-node/ &slot1_partition;
      /delete-node/ &storage_partition;

      &cpuapp_rram {
         partitions {
            boot_partition: partition@0 {
               compatible = "zephyr,mapped-partition";
               label = "mcuboot";
               reg = <0x0 0xe000>;
            };

            slot0_partition: partition@e000 {
               compatible = "zephyr,mapped-partition";
               label = "image-0";
               reg = <0xe000 0xa7000>;
            };

            slot1_partition: partition@b5000 {
               compatible = "zephyr,mapped-partition";
               label = "image-1";
               reg = <0xb5000 0xa7000>;
            };

            storage_partition: partition@15d000 {
               compatible = "zephyr,mapped-partition";
               label = "storage";
               reg = <0x15d000 0x8000>;
            };
         };
      };

#. Create an MCUboot overlay :file:`sysbuild/mcuboot/boards/nrf54l15dk_nrf54l15_cpuapp.overlay` file with the same partition layout and the chosen node:

   .. code-block:: devicetree

      / {
         chosen {
            zephyr,code-partition = &boot_partition;
         };
      };

      /delete-node/ &boot_partition;
      /delete-node/ &slot0_partition;
      /delete-node/ &slot1_partition;
      /delete-node/ &storage_partition;

      &cpuapp_rram {
         partitions {
            boot_partition: partition@0 {
               compatible = "zephyr,mapped-partition";
               label = "mcuboot";
               reg = <0x0 0xe000>;
            };

            slot0_partition: partition@e000 {
               compatible = "zephyr,mapped-partition";
               label = "image-0";
               reg = <0xe000 0xa7000>;
            };

            slot1_partition: partition@b5000 {
               compatible = "zephyr,mapped-partition";
               label = "image-1";
               reg = <0xb5000 0xa7000>;
            };

            storage_partition: partition@15d000 {
               compatible = "zephyr,mapped-partition";
               label = "storage";
               reg = <0x15d000 0x8000>;
            };
         };
      };

#. Create a :file:`sysbuild.conf` file with the :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` Kconfig option set to ``y``.
#. Enable MCUmgr and SMP in your application's :file:`prj.conf` file as described in :ref:`dfu_over_ble`.

Migrating from Partition Manager
********************************

Previous versions of the |NCS| used the Partition Manager for non-volatile memory partitioning in Bluetooth Mesh samples.
Partition Manager is deprecated in favor of devicetree-based partition configuration.
If your application still relies on Partition Manager for partition layout, see :ref:`migration_partitions` for instructions on how to migrate to devicetree overlays.

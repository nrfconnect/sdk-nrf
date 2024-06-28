.. _ug_nrf70_fw_patch_update:

Firmware patch update
#####################

.. contents::
   :local:
   :depth: 2

This guide explains the available option for updating the nRF70 Series firmware patches that reside in the external memory using the Device Firmware Updates (DFU) procedure.

.. note::
    External memory refers to the memory that is outside the System-on-Chip (SoC), for example, an external flash memory chip, or an external nonvolatile memory (NVM) chip.

.. note::
    Currently, you cannot build an example with the both ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE`` and :kconfig:option:`CONFIG_XIP_SPLIT_IMAGE` Kconfig options enabled.
    To enable XIP support use the ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_XIP`` Kconfig option instead of the ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE`` Kconfig option.

Overview
========

By default, the patch is located in the on-chip memory as part of the nRF Wi-Fi driver code.
However, you have the option to store it in external memory, which frees up space in the on-chip memory.
If the patch is situated outside the SoC memory space, it will not be updated automatically during a regular firmware update of the application core.
In such cases, you must enable the DFU procedure specifically for the nRF70 Series firmware patch on your device.

To learn more about the firmware patch located in the external memory, see the :ref:`ug_nrf70_developing_fw_patch_ext_flash` guide.

Prerequisites
=============

To use this feature, ensure that the following prerequisites are met:

* The external memory is available and properly configured in the device tree.
* The external memory has sufficient capacity to accommodate the firmware patches.
  This includes additional space for potential patch upgrades, such as those required for DFU.
  The combined size of all firmware patches should not exceed 128 kB.
* MCUboot is enabled, and the ``SB_CONFIG_BOOTLOADER_MCUBOOT`` Kconfig option is set to ``y``.

Supported platforms
===================

The following platforms are supported:

* nRF5340 DK with nRF7002 EK as a shield

Preparing the application
=========================

The DFU procedure for the nRF70 Series firmware patch, when located in external memory, requires the creation of additional MCUboot partitions.
This can be accomplished using the Partition Manager by defining the necessary partition layout in the Partition Manager configuration file specific to your application.
For instructions on configuring basic partitions with the Partition Manager, refer to the :ref:`partition_manager` documentation.

The instructions in this guide assume you are using the Partition Manager and its associated configuration files.

.. _nrf70_fw_patch_update_mcuboot_partitions:

Defining nRF70 Series MCUboot partitions
----------------------------------------

The number of MCUboot partitions required depends on the platform being used and its memory layout.
In addition to a partition dedicated to the nRF70 Series firmware patch, you will need to create specific partitions based on the platform:

* For applications that are based on a single image, define a new partition with the number ``1``.
* For multi-image builds, define the subsequent partition to follow the last existing one.

Below are examples of MCUboot partition names for updating the nRF70 Series firmware patch, which vary depending on the platform and the number of cores used:

* For the nRF5340 DK in a single-core variant (without the network core): ``mcuboot_primary_1`` and ``mcuboot_secondary_1``.
* For the nRF5340 DK in a multi-core variant (with the network core): ``mcuboot_primary_2`` and ``mcuboot_secondary_2``.

.. _nrf70_fw_patch_update_adding_partitions:

Adding nRF70 Series firmware patch partitions
---------------------------------------------

The examples below assume that there are two existing MCUboot partitions (for the application and network cores) and that the starting address of the free external memory space is ``0x12f000``.

To add the required partitions for the nRF70 Series firmware patch update, complete the following steps:

1. Create the ``nrf70_wifi_fw_mcuboot_pad`` partition for the MCUboot header.

   This partition should start from the first available address in the external memory space and have a size equal to the MCUboot image header length.

   For example:

    .. code-block:: console

        nrf70_wifi_fw_mcuboot_pad:
            address: 0x12f000
            size: 0x200
            device: MX25R64
            region: external_flash

#. Create the ``nrf70_wifi_fw`` partition for the firmware patch.

   This partition should start from the end address of the previously created MCUboot header partition and have a size of 128 kB (``0x20000``).

   For example:

    .. code-block:: console

        nrf70_wifi_fw:
            address: 0x12f200
            size: 0x20000
            device: MX25R64
            region: external_flash

#. Create the ``mcuboot_primary_X`` partition for MCUboot where ``X`` represents the appropriate partition number as described previously.

   This partition should have the same starting address as the ``nrf70_wifi_fw_mcuboot_pad`` partition, and a size of 132 kB + 200 B aligned to the device's sector size.
   It includes both the MCUboot header and the nRF70 Series firmware patch.

   For example, the MX25R64 device has a sector size of 4 kB, so the following configuration can be used:

    .. code-block:: console

        mcuboot_primary_2:
            orig_span: &id003
            - nrf70_wifi_fw_mcuboot_pad
            - nrf70_wifi_fw
            span: *id003
            address: 0x12F000
            size: 0x21000
            device: MX25R64
            region: external_flash

#. Create the ``mcuboot_secondary_X`` partition for MCUboot, where ``X`` represents the appropriate partition number as described in the :ref:`nrf70_fw_patch_update_mcuboot_partitions` section.

   This partition should start at the address immediately following the end of the ``mcuboot_primary_X`` partition and have the same size as the primary partition.
   This partition will be used to store the new nRF70 Series firmware patch during the DFU procedure.

   For example:

    .. code-block:: console

        mcuboot_secondary_2:
            address: 0x150000
            size: 0x21000
            device: MX25R64
            region: external_flash

#. Update the ``external_flash`` partition to allocate all available memory space to it.

   For example:

    .. code-block:: console

        external_flash:
            address: 0x171000
            size: 0x68F000
            device: MX25R64
            region: external_flash

.. note::
    The actual configuration syntax for the Partition Manager will depend on the specific system and tools being used.
    The example provided is for illustrative purposes and may need to be adjusted to fit the actual configuration file format and syntax required by the Partition Manager in use.

Configuring build system
------------------------

To enable the DFU procedure for the nRF70 Series firmware patch, complete the following steps depending on the platform:

* For the nRF5340 DK without the network core:

    1. Set the :kconfig:option:`CONFIG_NRF_WIFI_FW_PATCH_DFU` Kconfig option to ``y``.
    #. Set the ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES`` Kconfig option to ``2``.

* For the nRF5340 DK with the network core:

    1. Set the :kconfig:option:`CONFIG_NRF_WIFI_FW_PATCH_DFU`` Kconfig option to ``y``.
    #. Set the ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES`` Kconfig option to ``3``.

For example, to build the sample with the DFU procedure for the nRF70 Series firmware patch on the nRF5340 DK platform, which includes the network core image, run the following commands:

.. tabs::

   .. group-tab:: West

        .. code-block:: console

            west build -d nrf5340dk/nrf5340/cpuapp -d -- -DSHIELD=nrf7002ek -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y -DSB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES=3

   .. group-tab:: CMake

        .. code-block:: console

            cmake -GNinja -Bbuild -DBOARD=nrf5340dk/nrf5340/cpuapp -DSHIELD=nrf7002ek -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y -DSB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES=3 sample
            ninja -C build

   .. group-tab:: nRF Connect for VS Code

        1. When `building an application <How to build an application_>`_ as described in the |nRFVSC| documentation, follow the steps for setting up the build configuration.
        #. In the Add Build Configuration screen, click the Add argument button under the Extra CMake argument section.
        #. Add the following Kconfig options:

        .. code-block:: console

            -- -DSHIELD=nrf7002ek -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y -DSB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES=3

If you want to use the :ref:`sysbuild_images` feature, you need to set the ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH`` Kconfig option to ``y``, and must also set the :kconfig:option:`CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT` Kconfig option to one of the following values:

* For the nRF5340 DK without the network core: ``2``
* For the nRF5340 DK with the network core: ``3``

Performing the update of the nRF70 Series firmware patch
========================================================

To perform the update of the nRF70 Series firmware patch, you can use all available DFU alternatives described in the :ref:`ug_fw_update` page.

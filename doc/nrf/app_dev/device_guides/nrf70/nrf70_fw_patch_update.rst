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
    Currently, you cannot build an example with the both :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE` and :kconfig:option:`SB_CONFIG_QSPI_XIP_SPLIT_IMAGE` Kconfig options enabled.
    To enable XIP support use the :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_XIP` Kconfig option instead of the :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE` Kconfig option.

For nRF70 Series firmware patch update, define external memory regions in devicetree (DTS) using ``compatible = "fixed-partitions"`` under the external flash device node.
See :ref:`zephyr:flash_map_api` for details on how partitions map to the flash map used by MCUboot and the application.

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
* MCUboot is enabled, and the :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` Kconfig option is set to ``y``.

Supported platforms
===================

The following platforms are supported:

* nRF7002 DK
* nRF5340 DK with nRF7002 EK as a shield

.. note::

   For nRF7002 DK, this feature does not work with tools from the `nRF Command Line Tools`_ product page and nrfjprog programming tool.
   To program the nRF7002 DK device, you need to use the `nRF Util`_ tool that is part of the :ref:`nRF Connect SDK toolchain bundle <requirements_toolchain>` when you :ref:`gs_installing_toolchain`.

Preparing the application
=========================

.. include:: ../../../includes/pm_deprecation.txt

When the nRF70 Series firmware patch is stored in external memory, the DFU procedure requires additional MCUboot primary and secondary slot partitions on the external flash.
Define these regions in devicetree overlays or shared DTS fragments for your board or application.

The instructions in this guide assume that you are using devicetree-based partitioning (``compatible = "fixed-partitions"`` on the external NVM device).

.. _nrf70_fw_patch_update_mcuboot_partitions:

Defining nRF70 Series MCUboot partitions
----------------------------------------

The number of MCUboot partitions required depends on the platform being used and its memory layout.
In addition to a partition dedicated to the nRF70 Series firmware patch, you will need to create specific partitions based on the platform:

* For applications that are based on a single image, define a new partition with the number ``1``.
* For multi-image builds, define the subsequent partition to follow the last existing one.

Below are examples of MCUboot partition names for updating the nRF70 Series firmware patch, which vary depending on the platform and the number of cores used:

* For the nRF5340 DK and nRF7002 DK in a single-core variant (without the network core): ``mcuboot_primary_1`` and ``mcuboot_secondary_1``.
* For the nRF5340 DK and nRF7002 DK in a multi-core variant (with the network core): ``mcuboot_primary_2`` and ``mcuboot_secondary_2``.

.. _nrf70_fw_patch_update_adding_partitions:

Adding nRF70 Series firmware patch partitions
---------------------------------------------

The examples below assume that there are two existing MCUboot partitions (for the application and network cores) and that the starting address of the free external memory space is ``0x12f000``.

To add the required regions for the nRF70 Series firmware patch update, complete the following steps.
Define partitions under the external flash device (for example, ``&mx25r64``) in devicetree overlays or shared DTS fragments.
Within a single ``fixed-partitions`` node, sibling partitions must not overlap.

1. Specify the MCUboot image slot for the Wi‑Fi firmware child image so that the slot begins with an MCUboot image header (512 bytes, ``0x200`` in this example), followed by 128 kB (``0x20000``) for the firmware image body.
   The total primary slot size is 132 kB + 512 bytes (``0x21000``), aligned to the external flash sector size (for example, 4 kB on MX25R64kB on MX25R64).

#. Add a partition with the node label ``nrf70_wifi_fw_partition`` for the firmware image body (128 kB).
   Its base address must match the location where the nRF Wi-Fi build places the patch, immediately after the MCUboot header within the primary slot, such as ``0x12f200`` in this example.
   The firmware body resides within the same address range as the MCUboot primary slot, so you cannot describe both as overlapping children of the same fixed-partitions node.
   Follow your board's pattern instead (for example, define MCUboot partitions in one overlay and ``nrf70_wifi_fw_partition`` in the application overlay, or use a single merged memory map used by sysbuild).

#. Reserve any remaining external flash with additional partition nodes as required by your product.

The following devicetree fragment shows the ``nrf70_wifi_fw_partition`` node that the nRF Wi-Fi build expects.
Ensure that the parent device, partition offsets, and sizes match your complete external flash layout.

.. code-block:: devicetree

   &mx25r64 {
           partitions {
                   compatible = "fixed-partitions";
                   #address-cells = <1>;
                   #size-cells = <1>;

                   nrf70_wifi_fw_partition: nrf70_fw_partition: partition@12f200 {
                           reg = <0x12f200 0x20000>;
                   };
           };
   };

The ``nrf70-fw-patch-ext-flash`` snippet uses a smaller example layout; for DFU, you must align the MCUboot slot addresses and sizes with this guide and with :ref:`ug_fw_update`.
If you migrate devices from an older release, keep the partition addresses and sizes identical to the previous layout.

Configuring build system
------------------------

To enable the DFU procedure for the nRF70 Series firmware patch, complete the following steps depending on the platform:

.. tabs::

    .. group-tab:: nRF5340 DK

        1. Set the :kconfig:option:`CONFIG_NRF_WIFI_FW_PATCH_DFU` Kconfig option to ``y``.
        #. Set the :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE` Kconfig option to ``y``.
        #. Use the ``nrf70-fw-patch-ext-flash`` snippet, by adding ``-D<project_name>_SNIPPET=nrf70-fw-patch-ext-flash`` to the build command.
        #. Add shield configuration, by adding ``-DSHIELD=nrf7002ek`` to the build command.

        For example, to build the :ref:`wifi_shell_sample` sample with the DFU procedure for the nRF70 Series firmware patch on the nRF5340 DK platform, which includes the network core image, run the following commands:

        .. tabs::

            .. group-tab:: West

                    .. code-block:: console

                        west build -p -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y -Dshell_SNIPPET=nrf70-fw-patch-ext-flash

            .. group-tab:: CMake

                    .. code-block:: console

                        cmake -GNinja -Bbuild -DBOARD=nrf5340dk/nrf5340/cpuapp -DSHIELD=nrf7002ek -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y -Dshell_SNIPPET=nrf70-fw-patch-ext-flash -DAPP_DIR=*app_path* *path_to_zephyr*/share/sysbuild
                        ninja -C build

            .. group-tab:: nRF Connect for VS Code

                    1. When `building an application <How to build an application_>`_ as described in the |nRFVSC| documentation, follow the steps for setting up the build configuration.
                    #. In the **Add Build Configuration** screen, click the **Add argument** button under the **Extra CMake argument** section.
                    #. Add the following Kconfig options:

                    .. code-block:: console

                        -- -DSHIELD=nrf7002ek -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y -Dshell_SNIPPET=nrf70-fw-patch-ext-flash

    .. group-tab:: nRF7002 DK

            1. Set the :kconfig:option:`CONFIG_NRF_WIFI_FW_PATCH_DFU` Kconfig option to ``y``.
            #. Set the :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE` Kconfig option to ``y``.
            #. Use the ``nrf70-fw-patch-ext-flash`` snippet, by adding ``-D<project_name>_SNIPPET=nrf70-fw-patch-ext-flash`` to the build command.

        For example, to build the :ref:`wifi_shell_sample` sample with the DFU procedure for the nRF70 Series firmware patch on the nRF7002 DK platform, which does not include the network core image, run the following commands:

        .. tabs::

            .. group-tab:: West

                    .. code-block:: console

                        west build -p -b nrf7002dk/nrf5340/cpuapp -- -Dshell_SNIPPET=nrf70-fw-patch-ext-flash -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y

            .. group-tab:: CMake

                    .. code-block:: console

                        cmake -GNinja -Bbuild -- -DBOARD=nrf7002dk/nrf5340/cpuapp -Dshell_SNIPPET=nrf70-fw-patch-ext-flash -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y -DAPP_DIR=*app_path* *path_to_zephyr*/share/sysbuild
                        ninja -C build

            .. group-tab:: nRF Connect for VS Code

                    1. When `building an application <How to build an application_>`_ as described in the |nRFVSC| documentation, follow the steps for setting up the build configuration.
                    #. In the **Add Build Configuration** screen, click the **Add argument** button under the **Extra CMake argument** section.
                    #. Add the following Kconfig options:

                    .. code-block:: console

                        -- -Dshell_SNIPPET=nrf70-fw-patch-ext-flash -DSB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE=y -DCONFIG_NRF_WIFI_FW_PATCH_DFU=y

If you want to use the :ref:`sysbuild_images` feature, you need to set the :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH` Kconfig option to ``y``, and must also set the :kconfig:option:`CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT` Kconfig option to one of the following values:

* For the nRF5340 DK and nRF7002 DK without the network core: ``2``
* For the nRF5340 DK and nRF7002 DK with the network core: ``3``

Performing the update of the nRF70 Series firmware patch
========================================================

To perform the update of the nRF70 Series firmware patch, you can use all available DFU alternatives described in the :ref:`ug_fw_update` page.

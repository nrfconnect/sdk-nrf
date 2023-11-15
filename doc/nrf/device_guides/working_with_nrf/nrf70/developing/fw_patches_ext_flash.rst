.. _ug_nrf70_developing_fw_patch_ext_flash:

Firmware patches in the external memory
#######################################

.. contents::
   :local:
   :depth: 2

This guide explains the available options for having the nRF70 Series firmware patches reside in the external memory.

.. note::
  External memory refers to the memory that is outside the SoC, for example, an external flash memory chip, or external non-volatile memory (NVM) chip etc.

Overview
********

By default the nRF70 Series firmware patches are built as part of the nRF Wi-Fi host driver code, residing in on-chip memory.
The firmware patches include code that is executed on the nRF70 Series device.
The size of the firmware patches may be considerably large, which limits the amount of on-chip code memory available for the user application.
In order to increase the amount of on-chip memory available for user applications, the nRF70 Series Wi-Fi driver supports the option of using external memory, if that is available.

Prerequisites
=============

Before using this feature, make sure that the following prerequisites are completed:

 * The external memory must be available and configured in the devicetree.
 * The external memory must be large enough to store the firmware patches, taking into consideration also the patch upgrade requirement, for example, Device Firmware Update (DFU).
   The maximum size of all the firmware patches combined is 128 KB.

.. note::
    Currently, in Nordic Semiconductor SoCs, access to external memory and the Execute in Place (XIP) feature are available through the QSPI interface only.

Supported platforms
===================

The following platforms are supported:

* nRF5340 DK with nRF7002 EK as a shield
* nRF52840 DK with nRF7002 EK as a shield

.. note::
    Due to limitations mentioned above, the nRF7002 DK is not supported as it needs access to external memory through the SPI interface.

Available options
*****************

There are two supported options for offloading the firmware patches in the external memory feature:

* using XIP access
* using QSPI transfers to RAM

using XIP access
================

If the application supports XIP from external memory, then the firmware patches can be loaded as part of the nRF Wi-Fi driver code (RODATA) and then relocated to the external memory.
The nRF Wi-Fi driver accesses the firmware patches using XIP feature and downloads the firmware patches to the nRF70 device.
To enable this feature, set the :kconfig:option:`CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP` kconfig option to ``y``.
Once the build is complete, the feature can be verified by checking the memory regions summary in the build output.
A new memory region called ``EXTFLASH`` is added to the memory regions summary, and the firmware patches are placed in this region.
The size of the ``FLASH`` used is reduced by the size of the firmware patches.

A sample memory regions summary is shown below:

.. code-block:: console

  Memory region         Used Size  Region Size  %age Used
        EXTFLASH:       64544 B         8 MB      0.77%
           FLASH:      618752 B         1 MB     59.01%
             RAM:      170636 B       448 KB     37.20%
        IDT_LIST:          0 GB         2 KB      0.00%

using QSPI transfers to RAM
===========================

The nRF Wi-Fi driver supports the option for offloading the nRF70 firmware patch to external non-XIP memory.
In this case the upload of the firmware patch from the external memory to the nRF70 device happens in two stages:

* first the firmware patch is loaded from the external memory onto internal RAM and
* then it is uploaded to the nRF70 device.

This feature can be enabled using DTS or :ref::`snippets` feature or by using :ref::`partition_manager`.

Configuration
-------------

The following configuration options are available:

* :kconfig:option:`CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE`: Enables the option to store the firmware patch in external non-XIP memory.
* :kconfig:option:`CONFIG_NRF_WIFI_FW_FLASH_CHUNK_SIZE`: Defines the size of the chunks used to read the firmware patches from the external non-XIP memory.
  The default value is 8192 bytes.

The external memory partition name must be defined in the devicetree or in the partition manager configuration file.

* ``nrf70_fw_partition``: Defines the name of the external memory partition that stores the firmware patches.
  This must be defined in the devicetree, for example:

.. code-block:: dts

    &flash0 {
        partitions {
            compatible = "fixed-partitions";
            #address-cells = <1>;
            #size-cells = <1>;
            nrf70_fw_partition: partition@0 {
                label = "nrf70_fw_partition";
                reg = <0x00000000 DT_SIZE_K(128)>;
            };
        };
    };

* ``nrf70_wifi_fw`` : Defines the name of the external memory partition that stores the firmware patches.
  This must be defined in the partition manager configuration file, for example:

.. code-block:: console

      nrf70_wifi_fw:
        address: 0x12f000
        size: 0x20000
        device: MX25R64
        region: external_flash

Building
--------

See :ref:`nrf7002dk_nrf5340` for general instructions on building.

Additionally, you must enable either the `nrf70-fw-patch-ext-flash` snippet or the :kconfig:option:`CONFIG_PARTITION_MANAGER_ENABLED` option.

For example, to build the :ref:`wifi_shell_sample` sample for the nRF5340 DK with the ``nrf70-fw-patch-ext-flash`` snippet enabled, run the following commands:

With west
^^^^^^^^^

.. code-block:: console

    west build -p -b nrf5340dk_nrf5340_cpuapp -S nrf70-fw-patch-ext-flash samples/wifi/shell -- -DSHIELD=nrf7002ek

With CMake
^^^^^^^^^^

.. code-block:: console

    cmake -GNinja -Bbuild -DBOARD=nrf5340dk_nrf5340_cpuapp -DSHIELD=nrf7002ek -DSNIPPET=nrf70-fw-patch-ext-flash samples/wifi/shell
    ninja -C build

For example, to build the :ref:`wifi_shell_sample` sample for the nRF5340 DK with the :kconfig:option:`CONFIG_PARTITION_MANAGER_ENABLED` option enabled, run the following commands:

With west
^^^^^^^^^

.. code-block:: console

    west build -p -b nrf5340dk_nrf5340_cpuapp samples/wifi/shell -- -DSHIELD=nrf7002ek -DCONFIG_PARTITION_MANAGER_ENABLED=y -DCONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE=y

With CMake
^^^^^^^^^^

.. code-block:: console

    cmake -GNinja -Bbuild -DBOARD=nrf5340dk_nrf5340_cpuapp -DSHIELD=nrf7002ek -DCONFIG_PARTITION_MANAGER_ENABLED=y -DCONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE=y samples/wifi/shell
    ninja -C build

Programming
-----------

To program the firmware image with the firmware patches stored in the external memory, use the following commands:

With west
^^^^^^^^^

No changes are needed to the programming command:

.. code-block:: console

    west flash

With other tools
^^^^^^^^^^^^^^^^

You must use the :file:`merged.hex` file instead of the :file:`zephyr.hex` file to choose the program image explicitly.

For example, for nrfjprog:

.. code-block:: console

    nrfjprog -f nrf53 -s 0 --program build/zephyr/merged.hex ---sectorerase --qspisectorerase --verify --reset

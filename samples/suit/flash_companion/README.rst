.. _suit_flash_companion:

SUIT: Flash companion
#####################

.. contents::
   :local:
   :depth: 2

The SUIT flash companion sample allows the Secure Domain Firmware to access the external memory during the :ref:`Software Updates for Internet of Things (SUIT) <ug_nrf54h20_suit_dfu>` firmware upgrade.

.. _suit_flash_companion_reqs:

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp

.. _suit_flash_companion_overview:

Overview
********

The flash companion sample implements a device driver for the external flash device and provides an IPC service to the Secure Domain Firmware.
The Secure Domain Firmware uses the IPC service to read, erase, or write data to the external memory.

The sample is meant to be booted by the Secure Domain while performing the firmware update process using the :ref:`SUIT <ug_nrf54h20_suit_dfu>` firmware upgrade.

The flash companion sample is not a stand-alone firmware, it is intended to be used with the ``nrf54h_suit_sample`` to complete a firmware transfer with external flash.

.. _suit_flash_companion_config:

Configuration
*************

|config|

Setup
=====

You can build the sample using :ref:`sysbuild <configuration_system_overview_sysbuild>` by enabling the ``SB_CONFIG_SUIT_BUILD_FLASH_COMPANION`` Kconfig option.
The memory partition from which the firmware will run can be configured by providing a devicetree overlay through sysbuild.
You should create a dedicated partition in non-volatile memory and override the ``zephyr,code-partition``.
The memory partition must not be used by any other firmware image.

The sample is booted during the SUIT firmware upgrade process.
See the :ref:`ug_nrf54h20_suit_external_memory` user guide to learn how to setup your sample to boot and use the flash companion's services during firmware upgrade.

Configuration options
=====================

Check and configure the following configuration option:

.. _SB_CONFIG_SUIT_BUILD_FLASH_COMPANION:

SB_CONFIG_SUIT_BUILD_FLASH_COMPANION - Configuration for the firmware
   This option enables the sample and builds it during the sysbuild.

.. _suit_flash_companion_build_run:

Building and running
********************

The flash companion sample is not a stand-alone firmware.
To build it, open a different sample or application, such as :file:`samples/suit/smp_transfer`.
Make sure that both the main application and the flash companion support your target platform.

Perform the following steps in the main application directory:

1. Enable the ``SB_CONFIG_SUIT_BUILD_FLASH_COMPANION`` sysbuild option.

#. Create :file:`sysbuild/flash_companion.overlay` devicetree overlay file and add the following content:

   a. Create a dedicated partition in non-volatile memory:

      .. code-block:: devicetree

         &cpuapp_rx_partitions {
            cpuapp_slot0_partition: partition@a6000 {
               reg = <0xa6000 DT_SIZE_K(324)>;
            };
            companion_partition: partition@f7000 {
               reg = <0xf7000 DT_SIZE_K(36)>;
            };
         };

      In the above example the executable memory partition of the main application (``cpuapp_slot0_partition``) is shrunk to make space for the flash companion executable memory partition (``companion_partition``).

   #. Apply the same memory partition configuration to the main application's devicetree overlay.

   #. Enable SPI NOR devicetree node.
      In the case of nRF54H20 DK, you can enable the following node:

      .. code-block:: devicetree

         &mx25uw63 {
            status = "okay";
         };

#. Build and flash the main application:

   .. code-block:: console

      west build -b nrf54h20dk/nrf54h20/cpuapp
      west flash

The flash companion sample will be built flashed automatically by sysbuild.

Dependencies
************

This sample uses the following |NCS| libraries:

* :file:`include/sdfw_services/ssf_client.h`
* `zcbor`_

It uses the following Zephyr library:

* :ref:`zephyr:flash_api`

The sample also uses drivers from `nrfx`_.

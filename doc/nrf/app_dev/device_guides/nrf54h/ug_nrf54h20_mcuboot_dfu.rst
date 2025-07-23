.. _ug_nrf54h20_mcuboot_dfu:

Configuring DFU and MCUboot
###########################

.. contents::
   :local:
   :depth: 2

This page provides an overview of Device Firmware Update (DFU) for the nRF54H Series devices, detailing the necessary steps, configurations, and potential risks involved in setting up secure boot and firmware updates.

On the nRF54H20 SoC, you can use MCUboot as a *standalone immutable bootloader*.
If you want to learn how to start using MCUboot in your application, refer to the :ref:`ug_bootloader_adding_sysbuild` page.
For full introduction to the bootloader and DFU solution, see :ref:`ug_bootloader_mcuboot_nsib`.

.. note::
   |NSIB| is not supported on the nRF54H20 SoC.

You must select a sample that supports DFU to ensure proper testing of its functionality.
In the following subsections, the Zephyr's :zephyr:code-sample:`smp-svr` sample is used as an example.

Configuring MCUboot on the nRF54H20 DK
**************************************

You can build any nRF54H20 SoC sample with MCUboot support by passing the :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` Kconfig option.
This enables the default *swap using move* bootloader mode, supports a single updateable image, and applies the standard MCUboot configurations.

See the ``smp_svr`` sample for reference.
It demonstrates how to enable mcumgr commands in the application, allowing you to read information about images managed by MCUboot.
To configure the sample for using MCUboot, follow these steps:

1. Navigate to the :file:`zephyr/samples/subsys/mgmt/mcumgr/smp_svr` directory.

#. Build the firmware:

   .. parsed-literal::
      :class: highlight

      west build -b nrf54h20dk/nrf54h20/cpuapp -p -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

#. Program the firmware onto the device:

   .. parsed-literal::
      :class: highlight

      west flash

RAM cleanup
***********

To prevent data leakage, MCUboot can clear its RAM upon completion of execution.
This feature is controlled by the :kconfig:option:`CONFIG_SB_CLEANUP_RAM` Kconfig option.

Supported signatures
********************

MCUboot supports the following signature types:

* Ed25519
* Ed25519-pure - Recommended option.
  It cannot be used with external memory.
* ECDSA-P256 (enable by setting :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256` to ``y``)

Signature keys
**************

By default, MCUboot stores the public key in its own bootloader image.
The build system automatically embeds the key at compile time.
For more information, see `Adding external flash`_.

Image encryption
****************

MCUboot supports AES-encrypted images on the nRF54H20 SoC.

.. warning::
   On the nRF54H20 SoC, private and public keys are currently stored in the image.
   Embedding keys directly within the firmware image could pose a potential security risk.

DFU configuration example
*************************

MCUboot supports various methods for updating firmware images.
On the nRF54H platform, you can use :ref:`swap and direct-xip modes<ug_bootloader_main_config>`.

For more information, see the Zephyr sample in the :file:`nrf/samples/zephyr/subsys/mgmt/mcumgr/smp_svr` directory.
This sample demonstrates how to configure DFU in your project using the Simple Management Protocol (mcumgr) for DFU and querying device information.

The following build flavours are available:

* ``sample.mcumgr.smp_svr.bt.nrf54h20dk`` - DFU over BLE using the default IPC radio core image and *Swap using move* MCUboot mode.
  This flavour is compatible with the nRF Device Manager.
* ``sample.mcumgr.smp_svr.bt.nrf54h20dk.direct_xip_withrevert`` - DFU over BLE using *Direct-XIP with revert* MCUboot mode.
* ``sample.mcumgr.smp_svr.serial.nrf54h20dk.ecdsa`` - DFU over serial port with ECDSA P256 signature verification.
  This flavour supports the ``mcumgr`` command-line tool and AuTerm.

To build and run the sample, use the following commands:

.. code-block:: console

    west build -b nrf54h20dk/nrf54h20/cpuapp -T ./sample.mcumgr.smp_svr.bt.nrf54h20dk
    west flash

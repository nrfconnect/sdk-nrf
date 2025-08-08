.. _ug_nrf54h20_mcuboot_dfu:

Configuring DFU and MCUboot
###########################

.. contents::
   :local:
   :depth: 2

This page provides an overview of Device Firmware Update (DFU) for the nRF54H Series devices, detailing the necessary steps, configurations, and potential risks involved in setting up secure boot and firmware updates.

On the nRF54H20 SoC, you can use MCUboot as a *standalone immutable bootloader*.
If you want to learn how to start using MCUboot in your application, refer to the :ref:`ug_bootloader_adding_sysbuild` page.
For full introduction to the bootloader and DFU solution, see :ref:`ug_bootloader_mcuboot_nsib` and :ref:`ug_bootloader_adding_sysbuild_immutable_mcuboot`.

.. note::
   |NSIB| is not supported on the nRF54H20 SoC.

You must select a sample that supports DFU to ensure proper testing of its functionality.
In the following sections, the SMP server sample variant in the :file:`samples/zephyr/subsys/mgmt/mcumgr/smp_svr` folder is used.
It extends the Zephyr's :zephyr:code-sample:`smp-svr` sample and adapts it for nRF54H20 platform.

.. note::
   There are two variants of the SMP server sample:

   * The new ``sdk-nrf`` sample supporting the nRF54H20 SoC, located in the :file:`samples/zephyr/subsys/mgmt/mcumgr/smp_svr` folder.
   * The :zephyr:code-sample:`smp-svr`, located in the :file:`samples/subsys/mgmt/mcumgr/smp_svr` folder, is the Zephyr sample that supports MCUboot and DFU.

Configuring MCUboot on the nRF54H20 DK
**************************************

You can build any nRF54H20 SoC sample with MCUboot support by passing the :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` Kconfig option.
This enables the default *swap using move* bootloader mode, supports a single updateable image, and applies the standard MCUboot configurations.

To configure the :zephyr:code-sample:`hello_world` sample for using MCUboot, follow these steps:

1. Navigate to the :file:`zephyr/samples/hello_world` directory.

#. Build the firmware:

   .. parsed-literal::
      :class: highlight

      west build -b nrf54h20dk/nrf54h20/cpuapp -p -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

#. Program the firmware onto the device:

   .. parsed-literal::
      :class: highlight

      west flash

See the :zephyr:code-sample:`smp-svr` sample for a reference of how you can further configure your application with MCUboot.
It demonstrates how to enable :ref:`dfu_tools_mcumgr_cli` commands in the application, allowing you to read information about images managed by MCUboot.

RAM cleanup
***********


To prevent data leakage, enable MCUboot's RAM cleanup by setting the :kconfig:option:`SB_CLEANUP_RAM` Kconfig option as follows:

* When building with CMake, add ``-Dmcuboot_CONFIG_SB_CLEANUP_RAM``.
* When using sysbuild, add ``CONFIG_SB_CLEANUP_RAM=y`` to :file:`sysbuild/mcuboot.conf`.

This feature is controlled by the :kconfig:option:`CONFIG_SB_CLEANUP_RAM` Kconfig option.

Supported signatures
********************

MCUboot supports the following signature types:

* Ed25519 - default for nRF54H20
* ECDSA P256 - enabled by setting :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256` to ``y``

By default, MCUboot stores the public key in its own bootloader image.
The build system automatically embeds the key at compile time.
For more information, see `DFU with custom keys`_.

Image encryption
****************

MCUboot supports AES-encrypted images on the nRF54H20 SoC, using ECIES-X25519 for key exchange.
For detailed information on ECIES-X25519 support, refer to the :ref:`ug_nrf54h_ecies_x25519` documentation page.

.. warning::
   On the nRF54H20 SoC, private and public keys are currently stored in the image.
   Embedding keys directly within the firmware image could pose a potential security risk.

DFU configuration example
*************************

MCUboot supports various methods for updating firmware images.
On the nRF54H platform, you can use :ref:`swap and direct-xip modes<ug_bootloader_main_config>`.

For more information, see the :file:`samples/zephyr/subsys/mgmt/mcumgr/smp_svr` sample.
This sample demonstrates how to configure DFU feature in both MCUBoot and user application in your project.
It uses Simple Management Protocol for DFU and querying device information from the application.

The following build flavours are available:

* ``sample.mcumgr.smp_svr.bt.nrf54h20dk`` - DFU over BLE using the default :ref:`ipc_radio` image and *Swap using move* MCUboot mode.
* ``sample.mcumgr.smp_svr.bt.nrf54h20dk.direct_xip_withrevert`` - DFU over BLE using *Direct-XIP with revert* MCUboot mode.
* ``sample.mcumgr.smp_svr.serial.nrf54h20dk.ecdsa`` - DFU over serial port with ECDSA P256 signature verification.

To build and run the sample, use the following commands:

.. code-block:: console

    west build -b nrf54h20dk/nrf54h20/cpuapp -T ./sample.mcumgr.smp_svr.bt.nrf54h20dk
    west flash

Testing steps
=============

You can test the :zephyr:code-sample:`smp-svr` sample by performing a FOTA update.
To do so, complete the following steps:

1. Locate the :file:`dfu_application.zip` archive in the build directory of the build from the previous chapter.
   The archive is automatically generated after adding the DFU configuration and building your project.

   .. note::
      For each image included in the DFU-generated package, use a higher version number than your currently active firmware.
      You can do this by modifying the VERSION file in the application directory or by making changes to the application code.
      For the semantic versioning, modify the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option.
      Otherwise, the DFU target may reject the FOTA process due to a downgrade prevention mechanism.

#. Download the :file:`dfu_application.zip` archive to your mobile phone.
   See :ref:`app_build_output_files` for more information about the contents of update archive.

   .. note::
      nRF Connect for Desktop does not currently support the FOTA process.

#. Use the `nRF Connect Device Manager`_ mobile app to update your device with the new firmware.

   a. Ensure that you can access the :file:`dfu_application.zip` archive from your phone or tablet.
   #. In the mobile app, scan and select the device to update.
   #. Switch to the :guilabel:`Image` tab.
   #. Tap the :guilabel:`SELECT FILE` button and select the :file:`dfu_application.zip` archive.
   #. Tap the :guilabel:`START` button.
   #. Initiate the DFU process of transferring the image to the device:

      * If you are using an Android phone or tablet, select a mode in the dialog window, and tap the :guilabel:`START` button.
      * If you are using an iOS device, tap the selected mode in the pop-up window.

      .. note::
         For samples using random HCI identities, the Test and Confirm mode should not be used.

   #. Wait for the DFU to finish and then verify that the new application works properly by observing the new device name visible in the Device Manager app.


Additional Information
**********************

You can test BLE-based FOTA samples with the `nRF Connect Device Manager`_.
For DFU over a serial connection, use the :ref:`dfu_tools_mcumgr_cli` tool.

.. note::
   On the nRF54H20 SoC, Direct XIP mode uses a merged image slot that combines both application and radio core images.
   Refer to the sample's DTS overlay files to understand the partition layout.
   In contrast, Swap modes place application and radio images in separate MCUboot slots, enabling multi-image updates.

   Direct XIP (merged) build artifacts are generated in ``build/zephyr``.
   Swap-mode artifacts reside in subdirectories under ``build`` (for example, ``build/smp_svr/zephyr`` or ``build/ipc_radio/zephyr``).

.. note::
   DFU from external flash is currently not supported on the nRF54H20 SoC.

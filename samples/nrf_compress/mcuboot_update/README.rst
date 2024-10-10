.. _nrf_compression_mcuboot_compressed_update:

nRF Compression: MCUboot compressed update
##########################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to enable and use :ref:`image compression within MCUboot <mcuboot_image_compression>`, which allows for smaller application updates to be loaded to a device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

This sample is using the :ref:`nrf_compression` library.

The sample's default configuration has the following features enabled:

* MCUboot in the upgrade-only mode with the Kconfig option :kconfig:option:`CONFIG_BOOT_UPGRADE_ONLY` set to ``y``.
* Single-update image with the Kconfig option :kconfig:option:`CONFIG_UPDATEABLE_IMAGE_NUMBER` set to ``1``.

When the sample is built, the build system automatically generates the update binary files with compressed image data and flags set.
These files match the :ref:`requirements for MCUboot image compression <mcuboot_image_compression_setup>`.

Configuration
*************

|config|

See :ref:`configuring_kconfig` for information about the different ways you can set Kconfig options in the |NCS|.

Configuration options
=====================

.. _CONFIG_OUTPUT_BOOT_MESSAGE:

CONFIG_OUTPUT_BOOT_MESSAGE
    When enabled, this optional configuration option outputs a message at run-time that allows for checking if the image has been updated successfully.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf_compress/mcuboot_update`

.. include:: /includes/build_and_run.txt

To upload MCUboot and the bundle of images to the device, program the sample by using the :ref:`flash command without erase <programming_params_no_erase>`.

Testing over Bluetooth LE
=========================

The testing scenario uses the FOTA over Bluetooth LE method to load the firmware update to the device.
For the testing scenario to work, make sure you meet the following requirements:

* Your update image differs from the base image so that the update can be applied.
* You changed the firmware version by setting :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` to ``"2.0.0"``.
* You set the :kconfig:option:`CONFIG_OUTPUT_BOOT_MESSAGE` Kconfig option to ``y``.
* You are familiar with the FOTA over Bluetooth LE method (see :ref:`the guide for the nRF52840 DK <ug_nrf52_developing_ble_fota_steps>` and :ref:`the guide for the nRF5340 DK <ug_nrf53_developing_ble_fota_steps>`).
* You have the `nRF Connect Device Manager`_ mobile app installed on your smartphone to update your device with the new firmware over Bluetooth LE.

Meeting these requirements requires rebuilding the sample.

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Start the `nRF Connect Device Manager`_ mobile app.
#. Follow the testing steps for the FOTA over Bluetooth LE.
   See the following sections of the documentation:

   * :ref:`Testing steps for FOTA over Bluetooth LE with nRF52840 <ug_nrf52_developing_ble_fota_steps_testing>`
   * :ref:`Testing steps for FOTA over Bluetooth LE with nRF5340 <ug_nrf53_developing_ble_fota_steps_testing>`
   * *Not yet available*: Testing steps for FOTA over Bluetooth LE with nRF54L15

#. Once the firmware update has been loaded, check the UART output.
   See the following `Sample output`_ section.
#. Reboot the device.
   The compressed firmware update will be applied to the device and it will boot into it.

   .. note::
        At this point, the compressed flags will not be present in the running application image, as the firmware update has been decompressed whilst copying to the primary slot.

Sample output
=============

After a firmware update has been loaded, the UART will output the information on the image which will be one of the following:

LZMA2 with ARM
    The following output is logged:

    .. code-block:: console

       Secondary slot image is LZMA2 compressed with ARM thumb filter applied

    This indicates that the loaded firmware update has been compressed using LZMA2 and had the ARM thumb filter applied.
    This offers the best compression ratio and is recommended for general usage.

LZMA2 only
    The following output is logged:

    .. code-block:: console

       Secondary slot image is LZMA2 compressed

    This indicates that the loaded firmware update has been compressed using LZMZ2.
    This offers a good compression ratio but is not optimal.

No LZMA2
    The following output is logged:

    .. code-block:: console

       Secondary slot image is uncompressed

    This indicates that the loaded firmware update has not been compressed at all.
    This is a suboptimal and incorrect update.

For information about how these compression types are configured in the :ref:`nrf_compression` library, see :ref:`nrf_compression_config_compression_types` in its documentation.

Dependencies
************

The sample uses the following Zephyr library:

* :ref:`zephyr:mcu_mgr`

It also uses the following |NCS| library:

* :ref:`partition_manager`
* :ref:`nrf_compression`

.. _mcuboot_compressed_update:

MCUboot Compressed Update
#########################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to enable and use image compression within MCUboot which allows for smaller application updates to be loaded to a device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Note the following additional requirements:

* MCUboot configured in upgrade-only mode
* Single update image

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf_compress/mcuboot_update`
.. include:: /includes/build_and_run.txt

.. parsed-literal::
   :class: highlight

   west build -b *board_target*

The |NCS| build system will automatically generate the update binary files with compressed image data and flags set.

To upload MCUboot and a bundle of images to the device, use the ``west flash`` command.

Testing
=======

An update needs to be generated which differs from the base image so that the update can be applied.
This application has a Kconfig that can be enabled which will output a message at run-time which allows for checking that the image has been updated successfully.
To enable this, either rebuild the application with the Kconfig enabled or do a pristine build with the Kconfig enabled ``CONFIG_OUTPUT_BOOT_MESSAGE=y``, the firmware version should also be changed by setting ``-DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"2.0.0\"``.
Once an update image has been generated, follow the steps on :ref:`ug_nrf53_developing_ble_fota` to load an firmware update to the device via Bluetooth.

Checking the type of the update
-------------------------------

After a firmware update has been loaded, the UART will output the information on the image which will be one of the following:

.. code-block:: console

    Secondary slot image is LZMA2 compressed with ARM thumb filter applied

This indicates that the loaded firmware update has been compressed using LZMA2 and had the ARM thumb filter applied, this offers the best compression ratio and is recommended for general usage.

.. code-block:: console

    Secondary slot image is LZMA2 compressed

This indicates that the loaded firmware update has been compressed using LZMZ2, this offers a good compression ratio but is not optimal.

.. code-block:: console

    Secondary slot image is uncompressed

This indicates that the loaded firmware update has not been compressed at all, this is an unoptimal/incorrect update.

After the device is rebooted, the compressed firmware update will be applied to the device and it will boot into it, note that at this point the compressed flags will not be present in the running application image, as the firmware update has been decompressed whilst copying to the primary slot.

Dependencies
************

The sample uses the following Zephyr library:

* :ref:`zephyr:mcu_mgr`

It also uses the following |NCS| library:

* :ref:`partition_manager`
* :ref:`nrf_compression`

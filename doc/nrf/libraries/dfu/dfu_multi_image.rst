.. _lib_dfu_multi_image:

DFU multi-image
###############

.. contents::
   :local:
   :depth: 2

The DFU multi-image library provides an API for writing a DFU multi-image package that can be downloaded in chunks of arbitrary size.
The DFU multi-image package is a simple archive file that consists of a CBOR-based header that describes contents of the package, followed by a number of updates images, such as firmware images for different MCU cores.

Images included in a DFU multi-image package are identified by numeric identifiers assigned by the user.
The library provides a way for the user to register custom functions for writing a single image with a given identifier.

Because the library makes no assumptions about the formats of images included in a written package, it serves as a general-purpose solution for device firmware upgrades.
For example, it can be used to upgrade the nRF5340 firmware.

Configuration
*************

To enable the DFU multi-image library, set the :kconfig:option:`CONFIG_DFU_MULTI_IMAGE` Kconfig option.

To configure the maximum number of images that the DFU multi-image library is able to process, use the :kconfig:option:`CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT` Kconfig option.

To enable building the DFU multi-image package that contains commonly used update images, such as the application core firmware, the network core firmware, or MCUboot images, set the :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD` Kconfig option.
The following options control which images are included:

+-------------------------------------------------------------------+-----------------------------------------+
| Kconfig                                                           | Description                             |
+===================================================================+=========================================+
| :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_APP`           | Include application update.             |
+-------------------------------------------------------------------+-----------------------------------------+
| :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_NET`           | Include network core image update.      |
+-------------------------------------------------------------------+-----------------------------------------+
| :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_MCUBOOT`       | Include MCUboot update.                 |
+-------------------------------------------------------------------+-----------------------------------------+
| :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH` | Include nRF700x Wi-Fi® firmware patches.|
+-------------------------------------------------------------------+-----------------------------------------+

In addition to the built-in set of images, you can also register the extra images that will be included in the multi-image DFU package.
See :ref:`lib_dfu_extra` for details on how to configure and assign IDs for extra DFU images.

Allowing to restore progress after power failure
================================================

To enable restoring the write progress after a power failure or device reset, set the :kconfig:option:`CONFIG_DFU_MULTI_IMAGE_SAVE_PROGRESS` Kconfig option.
Currently, restoring the progress is only supported when using MCUboot as the DFU target.
To use this option, you must also set the following Kconfig options:

* :kconfig:option:`CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS` - Allows DFU multi-image to calculate its current progress based on offsets stored by the underlying DFU targets.
* :kconfig:option:`CONFIG_SETTINGS` - Allows the use of a settings area to store progress information.
* :kconfig:option:`CONFIG_NVS` (the nRF52 and nRF53 Series) or :kconfig:option:`CONFIG_ZMS` (the nRF54L and nRF54H Series) - Enables the settings backend to store data in NVM.

.. note::
  Enabling this option uses space in the settings area in NVM to store the progress information.
  Data is stored on every call to :c:func:`dfu_multi_image_write`.
  Make sure that the settings area is large enough to accommodate this additional data.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* `zcbor`_

API documentation
*****************

| Header file: :file:`include/dfu/dfu_multi_image.h`
| Source files: :file:`subsys/dfu/dfu_multi_image/src/`

.. doxygengroup:: dfu_multi_image

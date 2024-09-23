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
For example, it can be used to upgrade the :ref:`nRF5340 <ug_nrf5340_multi_image>` firmware.

Configuration
*************

To enable the DFU multi-image library, set the :kconfig:option:`CONFIG_DFU_MULTI_IMAGE` Kconfig option.

To configure the maximum number of images that the DFU multi-image library is able to process, use the :kconfig:option:`CONFIG_DFU_MULTI_IMAGE_MAX_IMAGE_COUNT` Kconfig option.

To enable building the DFU multi-image package that contains commonly used update images, such as the application core firmware, the network core firmware, or MCUboot images, set the ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD`` Kconfig option.
The following options control which images are included:

+-------------------------------------------------------------------+----------------------------------------+
| Kconfig                                                           | Description                            |
+===================================================================+========================================+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_APP``           | Include application update.            |
+-------------------------------------------------------------------+----------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_NET``           | Include network core image update.     |
+-------------------------------------------------------------------+----------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_MCUBOOT``       | Include MCUboot update.                |
+-------------------------------------------------------------------+----------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH`` | Include nRF700x Wi-Fi firmware patches.|
+-------------------------------------------------------------------+----------------------------------------+
|               ``SB_CONFIG_SUIT_MULTI_IMAGE_PACKAGE_BUILD``        | Include SUIT envelope and cache images.|
+-------------------------------------------------------------------+----------------------------------------+

.. _lib_dfu_multi_image_suit_multi_image_package:

SUIT multi-image package
========================

The DFU multi-image library supports building a SUIT multi-image package that includes a SUIT envelope and cache images.
The SUIT envelope is always included in the package as image 0, while SUIT cache images are included as subsequent images starting from the image 2.

The SUIT multi-image processing requires the SUIT system to support cache processing.
To enable it, set one of the following Kconfig options to ``y``:

* :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_FULL`
* :kconfig:option:`SUIT_DFU_CANDIDATE_PROCESSING_PUSH_TO_CACHE`

The build system merges all application images into a single :file:`dfu_cache_partition_1.bin` partition file and places its content into the multi-image image 2.
This allows all application images to be stored in a single DFU multi-image, as they will be processed by SUIT.

The :kconfig:option:`SB_CONFIG_SUIT_MULTI_IMAGE_PACKAGE_BUILD` Kconfig option enables building the SUIT multi-image package.
As a result, the multi-image package will contain:

* Image 0:
   - SUIT envelope that contains manifests only.

* Image 2:
   - Application core image.
   - Radio core image, if applicable.
   - Additional images, if applicable.

You can add more data to be processed by SUIT to the following images starting from image 3.
This operation will require an additional binary file and the proper :file:`dfu_cache_partition_X` definition in a devicetree configuration file, where ``X`` is the image number minus 1.
So for the image 3, you would need :file:`dfu_cache_partition_2`.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* `zcbor`_

API documentation
*****************

| Header file: :file:`include/dfu/dfu_multi_image.h`
| Source files: :file:`subsys/dfu/dfu_multi_image/src/`

.. doxygengroup:: dfu_multi_image

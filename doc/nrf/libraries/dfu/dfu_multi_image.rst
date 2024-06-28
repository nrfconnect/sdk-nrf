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

+-------------------------------------------------------------------+---------------------------------------+
| Kconfig                                                           | Description                           |
+===================================================================+=======================================+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_APP``           | Include application update.           |
+-------------------------------------------------------------------+---------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_NET``           | Include network core image update.    |
+-------------------------------------------------------------------+---------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_MCUBOOT``       | Include MCUboot update.               |
+-------------------------------------------------------------------+---------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH`` | Include nRF700x WiFi firmware patches.|
+-------------------------------------------------------------------+---------------------------------------+

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* `zcbor`_

API documentation
*****************

| Header file: :file:`include/dfu/dfu_multi_image.h`
| Source files: :file:`subsys/dfu/dfu_multi_image/src/`

.. doxygengroup:: dfu_multi_image
   :project: nrf
   :members:

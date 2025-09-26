.. _lib_dfu_extra:

DFU extra image extensions
##########################

.. contents::
   :local:
   :depth: 2

The DFU extra image module provides a flexible mechanism for including extra firmware images in the DFU packages.
This allows applications to extend the built-in DFU functionality with additional extra firmware images beyond ones supported natively in the nRF Connect SDK, for example firmware for external device.

This CMake extension supports the following DFU delivery mechanisms:

* **Multi-image binary** (``dfu_multi_image.bin``)

  Multi-image binary is used by applications that require downloading the DFU package containing multiple images as a single blob. The application is then responsible for handling the images. Refer to :ref:`lib_dfu_multi_image` to see how to provide firmware implementation.

* **ZIP packages** (``dfu_application.zip``)

  ZIP packages are used for updates via SMP protocol. The firmware responsible for handling the images uses SMP image management which automatically handles DFU of up to 3 images (MCUboot slots). In case addition of yet another image is needed, user needs to adjust the ``zephyr_img_mgmt.c`` file. For more information about SMP, see :ref:`Device Management <zephyr:device_mgmt>` documentation.

Configuration
*************

To enable extra image support, set the :kconfig:option:`SB_CONFIG_DFU_EXTRA_BINARIES` Kconfig option.

Usage Examples
**************

Add extra binaries in your application's **sysbuild.cmake** file:

.. code-block:: cmake

   if(SB_CONFIG_DFU_EXTRA_BINARIES)
      # Include the extra DFU system
      include(${ZEPHYR_NRF_MODULE_DIR}/cmake/dfu_extra.cmake)

      # Define path to external firmware
      set(ext_fw "${APP_DIR}/ext_fw.signed.bin")

      # Create build target for the external binary
      add_custom_target(ext_fw_target
         DEPENDS ${ext_fw}
         COMMENT "External firmware dependency"
      )

      # Add extra binary to DFU packages
      dfu_extra_add_binary(
          IMAGE_ID 1
          BINARY_PATH ${ext_fw}
          DEPENDS ext_fw_target
       )
   endif()

In case additional binary needs to be handled by MCUboot as updatable slots, adjust the :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES` and make sure MCUboot partitions are defined correctly.

.. note::
   This example shows configuration for IMAGE_ID 1. Adjust ID for your use case and make sure it doesn't conflict with built-in image IDs.
   For MCUboot-based systems, consider the 3-slot limitation.

Function reference
******************

dfu_extra_add_binary()
=======================

Adds an extra binary to DFU multi-image binary and/or ZIP packages.

**Syntax:**

.. code-block:: cmake

   dfu_extra_add_binary(
     IMAGE_ID <id>
     BINARY_PATH <path>
     [IMAGE_NAME <name>]
     [PACKAGE_TYPE <type>]
     [DEPENDS <target1> [<target2> ...]]
   )

**Parameters:**

* ``IMAGE_ID``    - Image identifier. Used for both multi-image packaging and ZIP packages.
                    Must be unique and should not conflict with built-in IDs, see :ref:`sysbuild_assigned_images_ids`.

* ``BINARY_PATH`` - Path to the binary file to include in the package. The path can be absolute
                    or relative to the build directory.

* ``IMAGE_NAME``  - Optional name for the binary file in packages. Defaults to the basename of
                    ``BINARY_PATH`` if not specified.

* ``PACKAGE_TYPE`` - Optional package type selection: ``"zip"``, ``"multi"``, or ``"all"`` (default).
                     Controls which package types include this binary:

                     * ``"zip"`` - Include only in ZIP packages
                     * ``"multi"`` - Include only in multi-image binary
                     * ``"all"`` - Include in both package types

* ``DEPENDS``     - Optional list of CMake targets that must be built before this extra binary
                    is available. This ensures proper build ordering.

See also
********

* :ref:`lib_dfu_multi_image`
* :ref:`sysbuild_assigned_images_ids`

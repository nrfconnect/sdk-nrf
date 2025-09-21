.. _lib_dfu_extra:

DFU extra image extensions
##########################

.. contents::
   :local:
   :depth: 2

The DFU extra image module provides a flexible mechanism for including extra firmware images in the DFU packages.
This flexibility enables applications to support firmware updates of additional images extending beyond the native capabilities of the |NCS|, for example, for external devices.

This CMake extension supports the following DFU delivery mechanisms:

* Multi-image binary (:file:`dfu_multi_image.bin`) - Used by applications that require downloading the DFU package containing multiple images as a single blob.
  The application is then responsible for handling the images.
  Refer to the :ref:`lib_dfu_multi_image` documentation page to see how to provide firmware implementation.

* ZIP packages (:file:`dfu_application.zip`) - Used for updates through Simple Management Protocol (SMP).
  The firmware responsible for handling the images uses SMP image management, which automatically handles DFU for up to three images (MCUboot slots).
  In case you need to add another image, you must adjust the :file:`zephyr_img_mgmt.c` file.
  For more information about SMP, see the :ref:`Device Management <zephyr:device_mgmt>` documentation.

Configuration
*************

To enable extra image support, set the :kconfig:option:`SB_CONFIG_MCUBOOT_EXTRA_IMAGES` Kconfig option to a value greater than ``0``.

Usage examples
**************

Add extra binaries in your application's :file:`sysbuild.cmake` file:

.. code-block:: cmake

   if(SB_CONFIG_MCUBOOT_EXTRA_IMAGES)
      # Include the extra DFU system
      include(${ZEPHYR_NRF_MODULE_DIR}/cmake/dfu_extra.cmake)

      # Define path to extra image
      set(ext_fw "${APP_DIR}/ext_fw.bin")

      # Create build target for the extra binary
      add_custom_target(ext_fw_target DEPENDS ${ext_fw})

      # Add extra binary to DFU packages
      dfu_extra_add_binary(
          BINARY_PATH ${ext_fw}
          NAME "ext_fw"
          DEPENDS ext_fw_target
       )
   endif()

If an additional binary needs to be handled by MCUboot as updatable slots, adjust the :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES` Kconfig option, and make sure MCUboot partitions are defined correctly.

To automatically include the extra binary in the final :file:`merged.hex` output, define a partition with the same name as provided in the ``NAME`` parameter within your partition manager configuration.

API documentation
*****************

See a list of specific CMake functions used within the system.

dfu_extra_add_binary()
======================

Adds an extra binary to DFU multi-image binary and ZIP packages.

Refer to the syntax of the binary:

.. code-block:: cmake

   dfu_extra_add_binary(
     BINARY_PATH <path>
     [NAME <name>]
     [VERSION <version>]
     [PACKAGE_TYPE <type>]
     [DEPENDS <target1> [<target2> ...]]
   )

You can adjust the following parameters:

* ``BINARY_PATH`` - Path to the binary file that will be included in the package.
  The path can be absolute or relative to the build directory.

* ``NAME`` - Optional name for the binary file in packages.
  If not provided, it defaults to the basename of ``BINARY_PATH``.

* ``VERSION`` - Optional image version string (for signing).

* ``PACKAGE_TYPE`` - Optional package type selection: ``"zip"``, ``"multi"``, or ``"all"`` (default).
  It determines which package types include this binary:

  * ``"zip"`` - Included only in ZIP packages
  * ``"multi"`` - Included only in multi-image binary
  * ``"all"`` - Included in both package types

* ``DEPENDS`` - Optional list of CMake targets that must be built before this extra binary is available.
  This ensures the correct build sequence.

Image IDs are automatically assigned based on the order binaries are added.
For more information about image ID assignment, see :ref:`sysbuild_assigned_images_ids`.

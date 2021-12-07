.. _app_build_system:

Build and configuration system
##############################

.. contents::
   :local:
   :depth: 2

The |NCS| build and configuration system is based on the one from Zephyr, with some additions.

Zephyr's build and configuration system
***************************************

Zephyr's build and configuration system uses the following building blocks as a foundation:

* CMake, the cross-platform build system generator
* Kconfig, a powerful configuration system also used in the Linux kernel
* Devicetree, a hardware description language that is used to describe the hardware that the |NCS| is to run on

Since the build and configuration system used by the |NCS| comes from Zephyr, references to the original Zephyr documentation are provided here in order to avoid duplication.
See the following links for information about the different building blocks mentioned above:

* :ref:`zephyr:application` is a complete guide to application development with Zephyr, including the build and configuration system.
* :ref:`zephyr:cmake-details` describes in-depth the usage of CMake for Zephyr-based applications.
* :ref:`zephyr:application-kconfig` contains a guide for Kconfig usage in applications.
* :ref:`zephyr:set-devicetree-overlays` explains how to use devicetree and its overlays to customize an application's devicetree.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.

|NCS| additions
***************

The |NCS| adds some functionality on top of the Zephyr build and configuration system.
Those additions are automatically included into the Zephyr build system using a :ref:`cmake_build_config_package`.

You must be aware of these additions when you start writing your own applications based on this SDK.

* The |NCS| provides an additional :file:`boilerplate.cmake` that is automatically included when using the Zephyr CMake package in the :file:`CMakeLists.txt` file of your application::

    find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

* The |NCS| allows you to :ref:`create custom build type files <gs_modifying_build_types>` instead of using a single :file:`prj.conf` file.
* The |NCS| build system extends Zephyr's with support for multi-image builds.
  You can find out more about these in the :ref:`ug_multi_image` section.
* The |NCS| adds a partition manager, responsible for partitioning the available flash memory.
* The |NCS| build system generates zip files containing binary images and a manifest for use with nRF Cloud FOTA.

.. _app_build_fota:

Building FOTA images
********************

The |NCS| build system places output images in the :file:`<build folder>/zephyr` folder.

If :kconfig:`CONFIG_BOOTLOADER_MCUBOOT` is set, the build system creates the :file:`dfu_application.zip` file containing files :file:`app_update.bin` and :file:`manifest.json`.
If you have also set the options :kconfig:`CONFIG_IMG_MANAGER` and :kconfig:`CONFIG_MCUBOOT_IMG_MANAGER`, the application will be able to process FOTA updates.
If you have set the options :kconfig:`CONFIG_SECURE_BOOT` and :kconfig:`CONFIG_BUILD_S1_VARIANT`, a similar file :file:`dfu_mcuboot.zip` will also be created.
You can use this file to perform FOTA updates of MCUboot itself.

The :file:`app_update.bin` file is a signed version of your application.
The signature matches to what MCUboot expects and allows this file to be used as an update.
The build system creates a :file:`manifest.json` file using information in the :file:`zephyr.meta` output file.
This includes the Zephyr and |NCS| git hashes for the commits used to build the application.
If your working tree contains uncommitted changes, the build system adds the suffix ``-dirty`` to the relevant version field.

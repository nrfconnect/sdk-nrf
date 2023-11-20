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

* CMake, the cross-platform build system generator.
* Kconfig, a powerful configuration system also used in the Linux kernel.
* Devicetree, a hardware description language that is used to describe the hardware that the |NCS| is meant to run on.

Since the build and configuration system used by the |NCS| comes from Zephyr, references to the original Zephyr documentation are provided here in order to avoid duplication.
See the following links for information about the different building blocks mentioned above:

* :ref:`zephyr:application` is a complete guide to application development with Zephyr, including the build and configuration system.
* :ref:`zephyr:cmake-details` describes in-depth the usage of CMake for Zephyr-based applications.
* :ref:`zephyr:application-kconfig` contains a guide for Kconfig usage in applications.
* :ref:`zephyr:set-devicetree-overlays` explains how to use devicetree and its overlays to customize an application's devicetree.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.

.. _app_build_additions:

|NCS| additions
***************

The |NCS| adds some functionality on top of the Zephyr build and configuration system.
Those additions are automatically included into the Zephyr build system using a :ref:`cmake_build_config_package`.

You must be aware of these additions when you start writing your own applications based on this SDK.

* The Kconfig option :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` is enabled by default.
  It gives warnings at CMake configure time if any experimental feature is enabled.

  For example, when building a sample that enables :kconfig:option:`CONFIG_BT_EXT_ADV`, the following warning is printed at CMake configure time:

  .. code-block:: shell

     warning: Experimental symbol BT_EXT_ADV is enabled.

  For more information, see :ref:`software_maturity`.
* The |NCS| provides an additional :file:`boilerplate.cmake` file that is automatically included when using the Zephyr CMake package in the :file:`CMakeLists.txt` file of your application::

    find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

* The |NCS| allows you to :ref:`create custom build type files <modifying_build_types>` instead of using a single :file:`prj.conf` file.
* The |NCS| build system extends Zephyr's with support for multi-image builds.
  You can find out more about these in the :ref:`ug_multi_image` section.
* The |NCS| adds a :ref:`partition_manager`, responsible for partitioning the available flash memory.
* The |NCS| build system generates zip files containing binary images and a manifest for use with nRF Cloud FOTA.

.. _app_build_additions_tools:

|NCS| configuration tools
=========================

The |nRFVSC| provides the following configuration tools for the build system components:

* For CMake, the `build configuration management <How to work with build configurations_>`_.
* For Devicetree, the `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_.
* For Kconfig, the `Kconfig GUI <Configuring with nRF Kconfig_>`_.

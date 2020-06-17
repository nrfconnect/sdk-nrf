.. _ncs-app-dev:

Application development
#######################

This user guide provides fundamental information on developing a new application based on the |NCS|.

Build and Configuration System
******************************

The |NCS| build and configuration system is based on the one from Zephyr, with some additions.

Zephyr's build and configuration system
=======================================

Zephyr's build and configuration system uses the following building blocks as a foundation:

* CMake, the cross-platform build system generator,
* Kconfig, a powerful configuration system also used in the Linux kernel,
* Device Tree, a hardware description language that is used to describe the hardware that the |NCS| is to run on.

Since the build and configuration system used by the |NCS| comes from Zephyr, references to the original Zephyr documentation are provided here in order to avoid duplication.
See the following links for information about the different building blocks mentioned above:

* :ref:`zephyr:application` is a complete guide to application development with Zephyr, including the build and configuration system.
* :ref:`zephyr:cmake-details` describes in-depth the usage of CMake for Zephyr-based applications.
* :ref:`zephyr:application-kconfig` contains a guide for Kconfig usage in applications.
* :ref:`zephyr:set-devicetree-overlays` explains how to use Device Tree and its overlays to customize an application's Device Tree.

|NCS| additions
===============

The |NCS| adds some functionality on top of the Zephyr build and configuration system.
Those additions are automatically included into the Zephyr build system using a :ref:`cmake_build_config_package`.

You must be aware of these additions when you start writing your own applications based on this SDK.

* The |NCS| provides an additional :file:`boilerplate.cmake` that is automatically included when using the Zephyr CMake package in the :file:`CMakeLists.txt` file of your application::

    find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

* The |NCS| allows you to :ref:`create custom build type files <gs_modifying_build_types>` instead of using a single :file:`prj.conf` file.
* The |NCS| build system extends Zephyr's with support for multi-image builds.
  You can find out more about these in the :ref:`ug_multi_image` section.
* The |NCS| adds a partition manager, responsible for partitioning the available flash memory.

Custom boards
*************

Defining your own board is a very common step in application development, since applications are typically designed to run on boards that are not directly supported by the |NCS|, given that they are typically custom designs and not available publicly.
In order to define your own board, you can use the following Zephyr guides as reference, since boards are defined in the |NCS| just as they are in Zephyr:

* :ref:`custom_board_definition` is a guide to adding your own custom board to the Zephyr build system.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.

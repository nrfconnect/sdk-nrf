.. _app_build_system:

Build and configuration system
##############################

.. contents::
   :local:
   :depth: 2

The |NCS| build and configuration system is based on the one from Zephyr, with some additions.

.. _configuration_system_overview:

Overview of build and configuration system
******************************************

The build and configuration system in Zephyr and the |NCS| uses the following building blocks as a foundation:

.. list-table:: Configuration system overview
   :header-rows: 1

   * - Source
     - File types
     - Usage
     - Available dedicated editing tools
     - Additional information
   * - :ref:`CMake <zephyr:build_overview>`
     - :file:`CMakeLists.txt`, :file:`.cmake`
     - Build system generator.
     - n/a
     - CMake is used to build your application together with the Zephyr kernel.
   * - :ref:`Devicetree <zephyr:dt-guide>`
     - :file:`.dts`, :file:`.dtsi`, :file:`.overlay`
     - Hardware description language.
     - `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_
     - Devicetree Visual Editor is part of the |nRFVSC|. You still need to be familiar with the devicetree language to use it.
   * - :ref:`Kconfig <zephyr:application-kconfig>`
     - :file:`Kconfig`, :file:`prj.conf`, :file:`.config`
     - Software configuration system also used in the Linux kernel.
     - `Kconfig GUI <Configuring with nRF Kconfig_>`_, :ref:`menuconfig and guiconfig <zephyr:menuconfig>`
     - | Kconfig GUI is part of the |nRFVSC|.
       | The :ref:`Kconfig Reference <configuration_options>` provides the documentation for each configuration option.
   * - :ref:`partition_manager`
     - :file:`pm.yml`, :file:`pm_static.yml`
     - Memory layout configuration system.
     - :ref:`partition_manager` script
     - Partition Manager is an |NCS| configuration system that is not available in Zephyr.

Each of these systems comes with a specialized syntax and purpose.
See the following sections for more information.
To read more about Zephyr's configuration system and its role in the application development, see :ref:`zephyr:build_overview` and :ref:`zephyr:application` in the Zephyr documentation.

When you :ref:`create_application`, the configuration files for each of these systems are created in the :ref:`application directory <create_application_structure>`: :file:`CMakeLists.txt` for CMake, :file:`app.overlay` for devicetree, :file:`prj.conf` for Kconfig, and :file:`partitions.yml` for Partition Manager (if enabled).
You can then edit them according to your needs (see :ref:`building`).

When you start building, a CMake build is executed in two stages: configuration phase and building phase.

.. figure:: ../images/ncs-toolchain.svg
   :alt: nRF Connect SDK tools and configuration

   |NCS| tools and configuration methods

.. _configuration_system_overview_config:

Configuration phase
===================

During this phase, CMake executes build scripts from :file:`CMakeLists.txt` and gathers configuration from different sources, for example :ref:`app_build_additions_build_types`, to generate the final build scripts and create a model of the build for the specified build target.
The result of this process is a :term:`build configuration`, a set of files that will drive the build process.

For more information about this phase, see the respective sections on Zephyr's :ref:`zephyr:cmake-details` page, which describes in-depth the usage of CMake for Zephyr-based applications.

Role of CMakeLists.txt
----------------------

In Zephyr and the |NCS|, the application is a CMake project.
As such, the application controls the configuration and build process of itself, Zephyr, and sourced libraries.
The application's :file:`CMakeLists.txt` file is the main CMake project file and the source of the build process configuration.

Zephyr provides a CMake package that must be loaded by the application into its :file:`CMakeLists.txt` file.
When loaded, the application can reference items provided by both Zephyr and the |NCS|.

Loading Zephyr's `CMake <CMake documentation_>`_ package creates the ``app`` CMake target.
You can add application source files to this target from the application :file:`CMakeLists.txt` file.
See :ref:`modifying_files_compiler` for detailed information.

.. _configure_application_hw:

Hardware-related configuration
------------------------------

.. ncs-include:: build/cmake/index.rst
   :docset: zephyr
   :dedent: 3
   :start-after: Devicetree
   :end-before: The preprocessed devicetree sources

The preprocessed devicetree sources are parsed by the :file:`zephyr/scripts/dts/gen_defines.py` script to generate a :file:`devicetree_unfixed.h` header file with preprocessor macros.

The :file:`zephyr.dts` file contains the entire hardware-related configuration of the system in the devicetree format.
The header file contains the same kind of information, but with defines usable by source code.

For more information, see :ref:`configuring_devicetree` and Zephyr's :ref:`zephyr:dt-guide`.
In particular, :ref:`zephyr:set-devicetree-overlays` explains how to use devicetree and its overlays to customize an application's devicetree.

.. _configure_application_sw:

Software-related configuration
------------------------------

.. ncs-include:: build/cmake/index.rst
   :docset: zephyr
   :dedent: 3
   :start-after: Kconfig
   :end-before: Information from devicetree is available to Kconfig,

Information from devicetree is available to Kconfig, through the functions defined in :file:`zephyr/scripts/kconfig/kconfigfunctions.py`.

The :file:`.config` file in the :file:`<build_dir>/zephyr/` directory describes most of the software configuration of the constructed binary.
Some subsystems can use their own configuration files.

For more information, see :ref:`configure_application` and Zephyr's :ref:`zephyr:application-kconfig`.
The :ref:`Kconfig Reference <configuration_options>` provides the documentation for each configuration option in the |NCS|.

Memory layout configuration
---------------------------

The memory layout configuration is provided by the :ref:`partition_manager` script, specific to the |NCS|.

The script must be enabled to provide the memory layout configuration.
It is impacted by various elements, such as Kconfig configuration options or the presence of child images.
Partition Manager ensures that all required partitions are in the correct place and have the correct size.

If enabled, the memory layout can be controlled in the following ways:

* Dynamically (default) - In this scenario, the layout is impacted by various elements, such as Kconfig configuration options or the presence of child images.
  Partition Manager ensures that all required partitions are in the correct place and have the correct size.
* Statically - In this scenario, you need to provide the static configuration.
  See :ref:`ug_pm_static` for information about how to do this.

After CMake has run, a :file:`partitions.yml` file with the memory layout will have been created in the :file:`build` directory.
This process also creates a set of header files that provides defines, which can be used to refer to memory layout elements.

Child images
------------

The |NCS| build system allows the application project to become a root for the sub-applications known in the |NCS| as child images.
Examples of child images are bootloader images, network core images, or security-related images.
Each child image is a separate application.

For more information, see :ref:`ug_multi_image`.

.. _configuration_system_overview_build:

Building phase
==============

During this phase, the final build scripts are executed.
The build phase begins when the user invokes ``make`` or `ninja <Ninja documentation_>`_.
The compiler (for example, `GCC compiler`_) then creates object files used to create the final executables.
You can customize this stage by :ref:`cmake_options` and using :ref:`compiler_settings`.

The result of this process is a complete application in a format suitable for flashing on the desired target board.
See :ref:`output build files <app_build_output_files>` for details.

The build phase can be broken down, conceptually, into four stages: the pre-build, first-pass binary, final binary, and post-processing.
To read about each of these stages, see :ref:`zephyr:cmake-details` in the Zephyr documentation.

.. _app_build_additions:

Additions specific to the |NCS|
*******************************

The |NCS| adds some functionality on top of the Zephyr build and configuration system.
Those additions are automatically included into the Zephyr build system using a :ref:`cmake_build_config_package`.

You must be aware of these additions when you start writing your own applications based on this SDK.

.. _app_build_additions_experimental:

Experimental option enabled by default
======================================

Unlike in Zephyr, the Kconfig option :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` is enabled by default in the |NCS|.
It gives warnings at CMake configure time if any :ref:`experimental <software_maturity>` feature is enabled.

For example, when building a sample that enables :kconfig:option:`CONFIG_BT_EXT_ADV`, the following warning is printed at CMake configure time:

.. code-block:: shell

   warning: Experimental symbol BT_EXT_ADV is enabled.

To disable these warnings, disable the :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` Kconfig option.

.. _app_build_additions_build_types:

Custom build types
==================

A build type is a feature that defines the way in which the configuration files are to be handled.
For example, selecting a build type lets you generate different build configurations for *release* and *debug* versions of the application.

In the |NCS|, the build type is controlled using the configuration files, whose names can be suffixed to define specific build types.
When you select a build type for the :ref:`configuration phase <configuration_system_overview_config>`, the compiler will use a specific set of files to create a specific build configuration for the application.

The :file:`prj.conf` file is the application-specific default, but many applications and samples include source files for generating the build configuration differently, for example :file:`prj_release.conf` or :file:`prj_debug.conf`.
Similarly, the build type can be included in file names for board configuration, Partition Manager's static configuration, child image Kconfig configuration, and others.
In this way, these files are made dependent on the build type and will only be used when the corresponding build type is invoked.
For example, if an application uses :file:`pm_static_release.yml` to define Partition Manager's static configuration, this file will only be used when the application's :file:`prj_release.conf` file is used to select the release build type.

Many applications and samples in the |NCS| use build types to define more detailed build configurations.
The most common build types are ``release`` and ``debug``, which correspond to CMake defaults, but other names can be defined as well.
For example, nRF Desktop features a ``wwcb`` build type, while Matter weather station features the ``factory_data`` build type.
See the application's Configuration section for information if it includes any build types.

For more information about how to invoke and create build types, see :ref:`modifying_build_types`.

.. _app_build_additions_multi_image:

Multi-image builds
==================

The |NCS| build system extends Zephyr's with support for multi-image builds.
You can find out more about these in the :ref:`ug_multi_image` section.

The |NCS| allows you to :ref:`create custom build type files <modifying_build_types>` instead of using a single :file:`prj.conf` file.

Boilerplate CMake file
======================

The |NCS| provides an additional :file:`boilerplate.cmake` file that is automatically included when using the Zephyr CMake package in the :file:`CMakeLists.txt` file of your application:

.. code-block::

   find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

This file checks if the selected board is supported and, when available, if the selected :ref:`build type <app_build_additions_build_types>` is supported.

Partition Manager
=================

The |NCS| adds the :ref:`partition_manager` script, responsible for partitioning the available flash memory and creating the `Memory layout configuration`_.

Binaries and images for nRF Cloud FOTA
======================================

The |NCS| build system generates :ref:`output zip files <app_build_output_files>` containing binary images and a manifest for use with `nRF Cloud FOTA`_.
An example of such a file is :file:`dfu_mcuboot.zip`.

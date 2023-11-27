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
     - Kconfig GUI is part of the |nRFVSC|.
   * - :ref:`partition_manager`
     - :file:`pm.yml`, :file:`pm_static.yml`
     - Memory layout configuration system.
     - :ref:`partition_manager` script
     - Partition Manager is an |NCS| configuration system that is not available in Zephyr.

Each of these systems comes with a specialized syntax and purpose.
See the following sections for more information.
To read more about Zephyr's configuration system and its role in the application development, see :ref:`zephyr:build_overview` and :ref:`zephyr:application` in the Zephyr documentation.

When you :ref:`create_application`, the configuration files for each of these systems are created in the :ref:`application directory <create_application_structure>`: :file:`CMakeLists.txt` for CMake, :file:`app.overlay` for Devicetree, :file:`prj.conf` for Kconfig, and :file:`partitions.yml` for Partition Manager (if enabled).
You can then edit them according to your needs (see :ref:`modifying`).

When you start building, a CMake build is executed in two stages: configuration phase and building phase.

.. _configuration_system_overview_config:

Configuration phase
===================

During this phase, CMake executes build scripts from :file:`CMakeLists.txt` and gathers configuration from different sources to generate the final build scripts and create a model of the build for the specified build target.
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

For more information, see :ref:`configure_application` and Zephyr's :ref:`zephyr:dt-guide`.
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

Memory layout configuration
---------------------------

The Partition Manager is specific to the |NCS|.
If enabled, it provides the memory layout configuration.
The layout is impacted by various elements, such as Kconfig configuration options or the presence of child images.
Partition Manager ensures that all required partitions are in the correct place and have the correct size.

If enabled, the memory layout can be controlled in the following ways:

* Dynamically (default) - In this scenario, the layout is impacted by various elements, such as Kconfig configuration options or the presence of child images.
  Partition Manager ensures that all required partitions are in the correct place and have the correct size.
* Statically - In this scenario, you need to provide the static configuration.
  See :ref:`ug_pm_static` for information about how to do this.

After CMake has run, a :file:`partitions.yml` file with the memory layout will have been created in the :file:`build` directory.
This process also creates a set of header files that provides defines, which can be used to refer to memory layout elements.

For more information, see :ref:`partition_manager`.

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
The build phase begins when the user invokes ``make`` or ``ninja``.
You can customize this stage by :ref:`cmake_options`.

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

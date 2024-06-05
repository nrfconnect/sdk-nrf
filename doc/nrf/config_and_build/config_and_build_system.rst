.. _app_build_system:

Build and configuration system
##############################

.. contents::
   :local:
   :depth: 3

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

During this phase, CMake executes build scripts from :file:`CMakeLists.txt` and gathers configuration from different sources, for example :ref:`app_build_file_suffixes`, to generate the final build scripts and create a model of the build for the specified board target.
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
In particular, :ref:`zephyr:set-devicetree-overlays` explains how the base devicetree files are selected.

In the |NCS|, you can use the |nRFVSC| to `create the devicetree files <How to create devicetree files_>`_ and work with them using the dedicated `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_.
You can also select the devicetree files when :ref:`cmake_options`.

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

For more information, see Zephyr's :ref:`zephyr:application-kconfig`.
In particular, :ref:`zephyr:initial-conf` explains how the base configuration files are selected.

In the |NCS|, just as in Zephyr, you can :ref:`configure Kconfig temporarily or permanently <configuring_kconfig>`.
You can also select the Kconfig options and files when :ref:`cmake_options`.

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

.. important::
    |sysbuild_related_deprecation_note|

The |NCS| build system allows the application project to become a root for the sub-applications known in the |NCS| as child images.
Examples of child images are bootloader images, network core images, or security-related images.
Each child image is a separate application.

For more information, see :ref:`ug_multi_image`.

.. _app_build_file_suffixes:

Custom configurations
---------------------

Zephyr provides the :ref:`zephyr:application-file-suffixes` feature for applications that require a single code base with multiple configurations for different product or build variants (or both).
When you select a given file suffix for the :ref:`configuration phase <configuration_system_overview_config>`, the build system will use a specific set of files to create a specific build configuration for the application.
If it does not find files that match the provided suffix, the build system will fall back to the default files without suffix.

The file suffix can be any string, but many applications and samples in the |NCS| use ``release``.
This suffix can be included in the :file:`prj.conf` file name (for example, :file:`prj_release.conf`), and also in file names for board configurations, child image Kconfig configurations, and others.
In this way, these files are made dependent on the given configuration and are only used when that build configuration is generated.
For example, if an application uses a custom :file:`nrf5340dk_nrf5340_cpuapp_release.overlay` overlay file, this file will be used together with the application's :file:`prj_release.conf` when you set :makevar:`FILE_SUFFIX` to ``release`` (``-DFILE_SUFFIX=release``).

Many applications and samples in the |NCS| define even more detailed build configurations.
For example, the :ref:`Zigbee light switch <zigbee_light_switch_sample>` sample features the ``fota`` configuration.
See the Configuration section of the given application or sample's documentation for information on if it includes any custom configurations.

.. important::
    The file suffixes feature is replacing the :ref:`app_build_additions_build_types` that used the :makevar:`CONF_FILE` variable.
    File suffixes are backward compatible with this variable.
    Some applications might still require using the :makevar:`CONF_FILE` variable during the deprecation period of the build types.

For information about how to provide file suffixes when building an application, see :ref:`cmake_options`.

.. _app_build_snippets:

Snippets
--------

Snippets are a Zephyr mechanism for defining portable build system overrides that could be applied to any application.
Read Zephyr's :ref:`zephyr:snippets` documentation for more information.

.. important::
  When using :ref:`configuration_system_overview_sysbuild`, the snippet is applied to all images, unless the image is specified explicitly (``-D<image_name>_SNIPPET="<your_snippet>"``).

You can set snippets for use with your application when you :ref:`set up your build configuration <building>` by :ref:`providing them as CMake options <cmake_options>`.

Usage of snippets is optional.

.. _configuration_system_overview_build:

Building phase
==============

During this phase, the final build scripts are executed.
The build phase begins when the user invokes ``make`` or `ninja <Ninja documentation_>`_.
The compiler (for example, `GCC compiler`_) then creates object files used to create the final executables.
You can customize this stage by :ref:`cmake_options` and using :ref:`compiler_settings`.

The result of this process is a complete application in a format suitable for flashing on the desired board target.
See :ref:`output build files <app_build_output_files>` for details.

The build phase can be broken down, conceptually, into four stages: the pre-build, first-pass binary, final binary, and post-processing.
To read about each of these stages, see :ref:`zephyr:cmake-details` in the Zephyr documentation.

.. _configuration_system_overview_sysbuild:

Sysbuild
========

The |NCS| supports Zephyr's System Build (sysbuild).

.. ncs-include:: build/sysbuild/index.rst
   :docset: zephyr
   :start-after: #######################
   :end-before: Definitions

To distinguish CMake variables and Kconfig options specific to the underlying build systems, :ref:`sysbuild uses namespacing <zephyr:sysbuild_kconfig_namespacing>`.
For example, sysbuild-specific Kconfig options are preceded by `SB_` before `CONFIG` and application-specific CMake options are preceded by the application name.

Sysbuild is integrated with west.
The sysbuild build configuration is generated using the sysbuild's :file:`CMakeLists.txt` file (which provides information about each underlying build system and CMake variables) and the sysbuild's Kconfig options (which are gathered in the :file:`sysbuild.conf` file).

.. note::
    In the |NCS|, building with sysbuild is :ref:`enabled by default <sysbuild_enabled_ncs>`.

For more information about sysbuild, see the following pages:

* :ref:`Sysbuild documentation in Zephyr <zephyr:sysbuild>`
* :ref:`sysbuild_images`
* :ref:`zephyr_samples_sysbuild`
* :ref:`sysbuild_forced_options`

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

.. _sysbuild_enabled_ncs:

Sysbuild enabled by default
===========================

When building :ref:`workspace applications <create_application_types_workspace>` copied from :ref:`nRF repositories <dm_repo_types>`, using the :ref:`standard building procedure <building>` automatically includes :ref:`configuration_system_overview_sysbuild` (the ``--sysbuild`` parameter).
For this reason, unlike in Zephyr, ``--sysbuild`` does not have to be explicitly mentioned in the command prompt when building the application.

This rule does not apply if you work with out-of-tree :ref:`freestanding applications <create_application_types_freestanding>`, for which you need to manually pass ``--sysbuild`` to build commands in every case.

You can disable building with sysbuild by using the ``--no-sysbuild`` parameter in the build command.

.. _app_build_additions_build_types:
.. _gs_modifying_build_types:
.. _modifying_build_types:

Custom build types
==================

.. important::
    |file_suffix_related_deprecation_note|
    It is still required for some applications that use build types with :ref:`multiple images <ug_multi_image>`.
    Check the application and sample documentation pages for which variable to use.

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

The following software components can be made dependent on the build type:

* The Partition Manager's :ref:`static configuration <ug_pm_static>`.
  When the build type has been inferred, the file :file:`pm_static_<buildtype>.yml` will have precedence over :file:`pm_static.yml`.
* The :ref:`child image Kconfig configuration <ug_multi_image_permanent_changes>`.
  Certain child image configuration files located in the :file:`child_image/` directory can be defined per build type.

The devicetree configuration is not affected by the build type.

For more information about how to invoke build types, see :ref:`cmake_options`.

.. _app_build_additions_multi_image:

Multi-image builds
==================

.. important::
    |sysbuild_related_deprecation_note|

The |NCS| build system extends Zephyr's with support for multi-image builds.
You can find out more about these in the :ref:`ug_multi_image` section.

The |NCS| allows you to :ref:`build types <app_build_additions_build_types>` instead of using a single :file:`prj.conf` file.

Boilerplate CMake file
======================

The |NCS| provides an additional :file:`boilerplate.cmake` file that is automatically included when using the Zephyr CMake package in the :file:`CMakeLists.txt` file of your application:

.. code-block::

   find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

This file checks if the selected board is supported and, when available, if the selected :ref:`file suffix <app_build_file_suffixes>` or :ref:`build type <app_build_additions_build_types>` is supported.

Partition Manager
=================

The |NCS| adds the :ref:`partition_manager` script, responsible for partitioning the available flash memory and creating the `Memory layout configuration`_.

Binaries and images for nRF Cloud FOTA
======================================

The |NCS| build system generates :ref:`output zip files <app_build_output_files>` containing binary images and a manifest for use with `nRF Cloud FOTA`_.
An example of such a file is :file:`dfu_mcuboot.zip`.

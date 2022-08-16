.. _gs_modifying:

Modifying an application
########################

.. contents::
   :local:
   :depth: 2

|application_sample_definition|

After programming and testing an application, you probably want to make some modifications to the application, for example, add your own files with additional functionality, change compilation options, or update the default configuration.

Adding files and changing compiler settings
*******************************************

The |NCS| build system is based on Zephyr, whose build system is based on `CMake <CMake documentation_>`_.
For more information about how the build system works in Zephyr, see :ref:`zephyr:build_overview` and :ref:`zephyr:application` in the Zephyr documentation.

In the |NCS|, the application is a CMake project.
As such, the application controls the configuration and build process of itself, Zephyr, and sourced libraries.
The application's :file:`CMakeLists.txt` file is the main CMake project file and the source of the build process configuration.

Zephyr provides a CMake package that must be loaded by the application into its :file:`CMakeLists.txt` file.
When loaded, the application can reference items provided by both Zephyr and the |NCS|.

Loading Zephyr's `CMake <CMake documentation_>`_ package creates the ``app`` CMake target.
You can add application source files to this target from the application :file:`CMakeLists.txt` file.

To update the :file:`CMakeLists.txt` file, either edit it directly or use the |nRFVSC| to maintain it.

Editing CMakeLists.txt
======================

You can add source files to the ``app`` CMake target with the :c:func:`target_sources` function provided by CMake.

Pay attention to the following configuration options:

* If your application is complex, you can split it into subdirectories.
  These subdirectories can provide their own :file:`CMakeLists.txt` files.
* The build system searches for header files in include directories.
  Add additional include directories for your application with the :c:func:`target_include_directories` function provided by CMake.
  For example, if you want to include an :file:`inc` directory, the code would look like the following:

  .. code-block:: c

     target_include_directories(app PRIVATE inc)

See the `CMake documentation`_ and :ref:`zephyr:cmake-details` in the Zephyr documentation for more information about how to edit :file:`CMakeLists.txt`.

Advanced compiler settings
==========================

The application has full control over the build process.

Using Zephyr's configuration options is the standard way of controlling how the system is built.
These options can be found under Zephyr's menuconfig **Build and Link Features** > **Compiler Options**.
For example, to turn off optimizations, select :kconfig:option:`CONFIG_NO_OPTIMIZATIONS`.

Compiler options not controlled by the Zephyr build system can be controlled through the :kconfig:option:`CONFIG_COMPILER_OPT` Kconfig option.

|VSC| compiler settings
=======================

.. modify_vsc_compiler_options_start

The |nRFVSC| lets you build and program with custom options.
For more information, read about the advanced `Custom launch and debug configurations`_ and `Application-specific flash options`_ in the extension documentation.

.. modify_vsc_compiler_options_end

.. _configure_application:

Configuring your application
****************************

You might want to change the default options of the application.
There are different ways of doing this, but not all will store your configuration permanently.

.. _configuration_system_overview:

Configuration system overview
=============================

Zephyr and the |NCS| use several configuration systems, each system with a specialized syntax and purpose.

The |NCS| consists of the following configuration sources:

* Devicetree source (DTS) files for hardware-related options.
* Kconfig files for software-related options.
* Partition Manager files for memory layout configuration.
  This is an |NCS| configuration system that is not available in Zephyr.

To read more about Zephyr's configuration system, see :ref:`zephyr:build_overview` in the Zephyr documentation.

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

For more information, see Zephyr's :ref:`zephyr:dt-guide`.

.. _configure_application_sw:

Software-related configuration
------------------------------

.. ncs-include:: build/cmake/index.rst
   :docset: zephyr
   :dedent: 3
   :start-after: Kconfig
   :end-before: Information from devicetree is available to Kconfig,

Information from devicetree is available to Kconfig, through the functions defined in :file:`zephyr/scripts/kconfig/kconfigfunctions.py`.

The single :file:`.config` file in the :file:`<build_dir>/zephyr/` directory describes the entire software configuration of the constructed binary.

For more information, see Zephyr's :ref:`zephyr:application-kconfig`.

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

After CMake has run, a single :file:`partitions.yml` file with the complete memory layout will have been created in the :file:`build` directory.
This process also creates a set of header files that provides defines, which can be used to refer to memory layout elements.

For more information, see :ref:`partition_manager`.

Child images
------------

The |NCS| build system allows the application project to become a root for the sub-applications known in the |NCS| as child images.
Examples of child images are bootloader images, network core images, or security-related images.
Each child image is a separate application.

For more information, see :ref:`ug_multi_image`.

Changing the configuration temporarily
======================================

When building your application, the different :file:`.config`, :file:`*_defconfig` files and the :file:`prj.conf` file are merged together and then processed by Kconfig.
The resulting configuration is written to the :file:`zephyr/.config` file in your :file:`build` directory.
This means that this file is available when building the application, but it is deleted when you clean the build directory with the ``pristine`` target (see Zephyr's :ref:`zephyr:application_rebuild` for more information).

To quickly test different configuration options, or to build your application in different variants, you can update the :file:`.config` file in the build directory.
Changes are picked up immediately.

While it is possible to edit the :file:`.config` file directly, you should use the nRF Kconfig GUI in the |nRFVSC| or a tool like menuconfig or guiconfig to update it.
These tools present all available options and allow you to select the ones that you need.

The nRF Kconfig GUI in the |nRFVSC| organizes the Kconfig options in a hierarchical list and lets you select the desired options.
To save the changes made using the nRF Kconfig GUI, click the :guilabel:`Save` button.
Read the `nRF Kconfig`_ documentation for more information.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run menuconfig or guiconfig.

To locate a specific configuration option, use the filter (**Search modules** field in the nRF Kconfig GUI or **Jump to** in menuconfig and guiconfig).
The documentation for each :ref:`configuration option <configuration_options>` also lists the menu path where the option can be found.

Changing the configuration permanently
======================================

To configure your application and maintain the configuration when you clean the build directory pristinely, you need to specify the configuration in one of the permanent configuration files.
This will be either the default :file:`prj.conf` file of the application or an extra Kconfig fragment.
In these files, you can specify different values for configuration options that are defined by a library or board, and you can add configuration options that are specific to your application.

See :ref:`zephyr:setting_configuration_values` in the Zephyr documentation for information on how to change the configuration permanently.

.. tip::
   Reconfiguring through menuconfig only changes the specific setting and the invisible options that are calculated from it.
   It does not adjust visible symbols that have already defaulted to a value even if this default calculation is supposed to be dependent on the changed setting.
   This may result in a bloated configuration compared to changing the setting directly in :file:`prj.conf`.
   See the section Stuck symbols in menuconfig and guiconfig on the :ref:`kconfig_tips_and_tricks` in the Zephyr documentation for more information.

If you work with |VSC|, you can use one of the following options:

* Select an extra Kconfig fragment file when `Building an application`_.
* Edit the Kconfig options using the nRF Kconfig GUI and save changes permanently to an existing or new :file:`prj.conf` file.
  See the extension's documentation for more information.

The :file:`prj.conf` file is read when you open a project.
The file will be reloaded when CMake re-runs.
This will happen automatically when the application is rebuilt.

.. _cmake_options:

Providing CMake options
***********************

You can provide additional options for building your application to the CMake process, which can be useful, for example, to switch between different build scenarios.
These options are specified when CMake is run, thus not during the actual build, but when configuring the build.

If you work with the |nRFVSC|, you can specify project-specific CMake options when you add the build configuration for a new |NCS| project.
See `Building an application`_ in the |nRFVSC| documentation.

If you work on the command line, pass the additional options to the ``west build`` command.
The options must be added after a ``--`` at the end of the command.
See :ref:`zephyr:west-building-cmake-args` for more information.

.. _gs_modifying_build_types:

Configuring build types
***********************

.. build_types_overview_start

When the ``CONF_FILE`` variable contains a single file and this file follows the naming pattern :file:`prj_<buildtype>.conf`, then the build type will be inferred to be *<buildtype>*.
The build type cannot be set explicitly.
The *<buildtype>* can be any string, but it is common to use ``release`` and ``debug``.

For information about how to set variables, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

The Partition Manager's :ref:`static configuration <ug_pm_static>` can also be made dependent on the build type.
When the build type has been inferred, the file :file:`pm_static_<buildtype>.yml` will have precedence over :file:`pm_static.yml`.

The child image Kconfig configuration can also be made dependent on the build type.
The child image Kconfig file is named :file:`<child_image>.conf` instead of :file:`prj.conf`, but otherwise follows the same pattern as the parent Kconfig.

.. build_types_overview_end

The Devicetree configuration is not affected by the build type.

.. note::
    For an example of an application that is using build types, see the :ref:`nrf_desktop` application (:ref:`nrf_desktop_requirements_build_types`) or the :ref:`nrf_machine_learning_app` application (:ref:`nrf_machine_learning_app_requirements_build_types`).

Selecting a build type in |VSC|
===============================

.. build_types_selection_vsc_start

To select the build type in the |nRFVSC|:

1. When `Building an application`_ as described in the |nRFVSC| documentation, follow the steps for setting up the build configuration.
#. In the **Add Build Configuration** screen, select the desired :file:`.conf` file from the :guilabel:`Configuration` drop-down menu.
#. Fill in other configuration options, if applicable, and click :guilabel:`Build Configuration`.

.. build_types_selection_vsc_end

Selecting a build type from command line
========================================

.. build_types_selection_cmd_start

To select the build type when building the application from command line, specify the build type by adding the following parameter to the ``west build`` command:

.. parsed-literal::
   :class: highlight

   -- -DCONF_FILE=prj_\ *selected_build_type*\.conf

For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf52840dk_nrf52840`` by running the following command in the project directory:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -d build_nrf52840dk_nrf52840 -- -DCONF_FILE=prj_release.conf

The ``build_nrf52840dk_nrf52840`` parameter specifies the output directory for the build files.

.. build_types_selection_cmd_end

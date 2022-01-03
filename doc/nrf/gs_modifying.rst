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

The |NCS| build system is based on Zephyr, whose build system is based on `CMake`_.
For more information about how the build system works in Zephyr, see :ref:`zephyr:build_overview` and :ref:`zephyr:application` in the Zephyr documentation.

In the |NCS|, the application is a CMake project.
As such, the application controls the configuration and build process of itself, Zephyr, and sourced libraries.
The application's :file:`CMakeLists.txt` file is the main CMake project file and the source of the build process configuration.

Zephyr provides a CMake package that must be loaded by the application into its :file:`CMakeLists.txt` file.
When loaded, the application can reference items provided by both Zephyr and the |NCS|.

Loading Zephyr's `CMake`_ package creates the ``app`` CMake target.
You can add application source files to this target from the application :file:`CMakeLists.txt` file.

To update the :file:`CMakeLists.txt` file, either edit it directly or use |SES| (SES) to maintain it.

Editing CMakeLists.txt directly
===============================

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

Maintaining CMakeLists.txt in SES
=================================

You must tag the :file:`CMakeLists.txt` files properly before adding them to a project in SES.
Projects in the :file:`sdk-nrf` repository already have these tags, but projects from Zephyr and other repositories do not.
Follow the steps in :ref:`ses_tags_in_CMakeLists` to manually add tags when using :file:`CMakeLists.txt` files that are not located in the :file:`sdk-nrf` repository.

To add a file in SES, right-click :guilabel:`Project 'app/libapp.a'` in the Project Explorer.
Select either :guilabel:`Add new file to CMakeLists.txt` to create a file and add it or :guilabel:`Add existing file to CMakeLists.txt` to add a file that already exists.

.. figure:: images/ses_add_files.png
   :alt: Adding files in SES

   Adding files in SES

To edit compilation options in SES, right-click :guilabel:`Project 'app/libapp.a'` in the Project Explorer and select :guilabel:`Edit Compile Options in CMakeLists.txt`.

In the window that is displayed, you can define compilation options for the project.

.. figure:: images/ses_compile_options.png
   :alt:

   Setting compiler defines, includes, and options in SES

.. note::
   These compilation options apply to the application project only.
   To manage Zephyr and other subsystems, go to :guilabel:`Project` > :guilabel:`Configure nRF Connect SDK Project`.

.. _ses_tags_in_CMakeLists:

SES tags in :file:`CMakeLists.txt`
----------------------------------

To be able to manage :file:`CMakeLists.txt` with SES, the CMake commands that are specific to the |NCS| application must be marked so SES can identify them.
Therefore, they must be surrounded by ``# NORDIC SDK APP START`` and ``# NORDIC SDK APP END`` tags.

The following CMake commands can be managed by SES, if they target the ``app`` in CMake:

* ``target_sources``
* ``target_compile_definitions``
* ``target_include_directories``
* ``target_compile_options``

The :file:`CMakeLists.txt` files for the sample applications in the :file:`sdk-nrf` repository already have the required tags.
Therefore, if you always use SES to maintain them, you do not need to worry about tagging.
Typically, the :file:`CMakeLists.txt` files include at least the :file:`main.c` file as source:

.. code-block:: c

   # NORDIC SDK APP START
   target_sources(app PRIVATE src/main.c)
   # NORDIC SDK APP END

Advanced compiler settings
==========================

The application has full control over the build process.

Using Zephyr's configuration options is the standard way of controlling how the system is built.
These options can be found under Zephyr's menuconfig :guilabel:`Build and Link Features` > :guilabel:`Compiler Options`.
For example, to turn off optimizations, select :kconfig:`CONFIG_NO_OPTIMIZATIONS`.

Compiler options not controlled by the Zephyr build system can be controlled through the :kconfig:`CONFIG_COMPILER_OPT` Kconfig option.

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

.. ncs-include:: guides/build/index.rst
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

.. ncs-include:: guides/build/index.rst
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
Changes are picked up immediately, and you do not need to re-open the project in SES.

While it is possible to edit the :file:`.config` file directly, you should use SES or a tool like menuconfig or guiconfig to update it.
These tools present all available options and allow you to select the ones that you need.

To edit the file in SES, select :guilabel:`Project` > :guilabel:`Configure nRF Connect SDK Project`.
If your application contains more than one image (see :ref:`ug_multi_image`), you must select the correct target.
To configure the parent image (the main application), select :guilabel:`menuconfig`.
The other options allow you to configure the child images.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run menuconfig or guiconfig.

To locate a specific configuration option, use the filter (:guilabel:`Jump to` in menuconfig and guiconfig).
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

If you work with SES, the :file:`prj.conf` file is read when you open a project.
The file will be reloaded when CMake re-runs.
This will happen automatically when the application is rebuilt, but can also be requested manually by using the :guilabel:`Project` > :guilabel:`Run CMake...` option.

.. _configuring_vsc:

Configuring in the VS Code extension
====================================

The |VSC| extension lets you modify your build configuration for custom boards, add additional CMake build arguments, select Kconfig fragments, and more.
For detailed instructions, see the `nRF Connect for Visual Studio Code`_ documentation site.

.. _cmake_options:

Providing CMake options
***********************

You can provide additional options for building your application to the CMake process, which can be useful, for example, to switch between different build scenarios.
These options are specified when CMake is run, thus not during the actual build, but when configuring the build.

If you work with SES, you can specify global CMake options that are used for all projects, and you can modify these options when you open a project:

* Specify global CMake options in the SES options before opening a project.
  Click :guilabel:`Tools` > :guilabel:`Options`, select the :guilabel:`nRF Connect` tab, and specify a value for :guilabel:`Additional CMake options`.
* Specify project-specific CMake options when opening the |NCS| project.
  Click :guilabel:`File` > :guilabel:`Open nRF Connect SDK project`, select :guilabel:`Extended Settings`, and specify the options in the :guilabel:`Extra CMake Build Options` field.
  This field is prepopulated with the global CMake options, and you can modify them, remove them, or add to them for the current project.

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
    For an example of an application that is using build types, see the :ref:`nrf_desktop` application (:ref:`nrf_desktop_requirements_build_types`), the :ref:`nrf_machine_learning_app` application (:ref:`nrf_machine_learning_app_requirements_build_types`), or the :ref:`pelion_client` application (:ref:`pelion_client_reqs_build_types`).

Selecting a build type in the VS Code extension
===============================================

.. build_types_selection_vsc_start

To select the build type in the |VSC| extension:

1. When `Building an application`_ as described in the |VSC| extension documentation, follow the steps for setting up the build configuration.
#. In the :guilabel:`Add Build Configuration` screen, select the desired :file:`.conf` file from the :guilabel:`Configuration` drop-down menu.
#. Fill in other configuration options, if applicable, and click :guilabel:`Build Configuration`.

.. build_types_selection_vsc_end

Selecting a build type in SES
=============================

.. build_types_selection_ses_start

To select the build type in SEGGER Embedded Studio:

1. Go to :guilabel:`File` > :guilabel:`Open nRF Connect SDK project`, select the current project, and specify the board name and build directory.
#. Select :guilabel:`Extended Settings`.
#. In the :guilabel:`Extra CMake Build Options` field, specify ``-DCONF_FILE=prj_<buildtype>.conf``, where *<buildtype>* in the file name corresponds to the desired build type.
   For example, for a build type named ``release``, set the following value: ``-DCONF_FILE=prj_release.conf``.
#. Do not select :guilabel:`Clean Build Directory`.
#. Click :guilabel:`OK` to re-open the project.


.. note::
   You can also specify the build type in the :guilabel:`Additional CMake Options` field in :guilabel:`Tools` > :guilabel:`Options` > :guilabel:`nRF Connect`.
   However, the changes will only be applied after re-opening the project.
   Reloading the project is not sufficient.

.. build_types_selection_ses_end

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

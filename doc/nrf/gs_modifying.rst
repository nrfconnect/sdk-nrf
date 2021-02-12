.. _gs_modifying:

Modifying a sample application
##############################

.. contents::
   :local:
   :depth: 2

After programming and testing a sample application, you probably want to make some modifications to the application, for example, add your own files with additional functionality, change compilation options, or update the default configuration.


Adding files and changing compiler settings
*******************************************

All files that your application uses must be specified in the :file:`CMakeLists.txt` file.
By default, most samples include only the main application file :file:`src/main.c`.
This means that you must add all other files that you are using.

You can also configure compiler options, application defines, or include directories, or set :ref:`build types <gs_modifying_build_types>` in :file:`CMakeLists.txt`.

To update the :file:`CMakeLists.txt` file, either edit it directly or use |SES| (SES) to maintain it.


Editing CMakeLists.txt directly
===============================

Add all files that your application uses to the ``target_sources`` function in :file:`CMakeLists.txt`.
To include several files, it can be useful to specify them with a wildcard.
For example, to include all :file:`.c` files from the :file:`src` folder, add the following lines to your :file:`CMakeLists.txt`::

   FILE(GLOB app_sources src/*.c)
   target_sources(app PRIVATE ${app_sources})

Instead of specifying each file (explicitly or with a wildcard), you can include all files from a folder by adding that folder as include folder::

   target_include_directories(app PRIVATE src)

See the `CMake documentation`_ and :ref:`zephyr:cmake-details` in the Zephyr documentation for more information about how to edit :file:`CMakeLists.txt`.


Maintaining CMakeLists.txt in SES
=================================

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
   To manage Zephyr and other subsystems, go to :guilabel:`Project` -> :guilabel:`Configure nRF Connect SDK Project`.


SES tags in :file:`CMakeLists.txt`
----------------------------------

To be able to manage :file:`CMakeLists.txt` with SES, the CMake commands that are specific to the |NCS| application must be marked so SES can identify them.
Therefore, they must be surrounded by ``# NORDIC SDK APP START`` and ``# NORDIC SDK APP END`` tags.

The following CMake commands can be managed by SES, if they target the ``app`` library:

* ``target_sources``
* ``target_compile_definitions``
* ``target_include_directories``
* ``target_compile_options``

The :file:`CMakeLists.txt` files for the sample applications in the |NCS| are tagged as required.
Therefore, if you always use SES to maintain them, you do not need to worry about tagging.
Typically, the :file:`CMakeLists.txt` files include at least the :file:`main.c` file as source::

   # NORDIC SDK APP START
   target_sources(app PRIVATE src/main.c)
   # NORDIC SDK APP END

.. _configure_application:

Configuring your application
****************************

If your application uses a provided library or targets a specific board, you might want to change the default configuration of the library or board.
There are different ways of doing this, but not all will store your configuration permanently.

The default configuration for a library is specified in its :file:`Kconfig` file.
Similarly, the default configuration for a board is specified in its :file:`*_defconfig` file (and its :file:`Kconfig.defconfig` file, see :ref:`zephyr:default_board_configuration` in the Zephyr documentation for more information).
The configuration for your application, which might override some default options of the libraries or the board, is specified in a :file:`prj.conf` file in the application directory.

For detailed information about configuration options, see :ref:`zephyr:application-kconfig` in the Zephyr documentation.


Changing the configuration permanently
======================================

To configure your application and maintain the configuration when you clean the build directory, add your changes to the :file:`prj.conf` file in your application directory.
In this file, you can specify different values for configuration options that are defined by a library or board, and you can add configuration options that are specific to your application.

See :ref:`zephyr:setting_configuration_values` in the Zephyr documentation for information on how to edit the :file:`prj.conf` file.

If you work with SES, the :file:`prj.conf` file is read when you open a project.
This means that after you edit this file, you must re-open your project.

.. note::
   It is possible to change the default configuration for a library by changing the :file:`Kconfig` file of the library.
   However, best practice is to override the configuration in the application configuration file :file:`prj.conf`.


Changing the configuration temporarily
======================================

When building your application, the different :file:`Kconfig` and :file:`*_defconfig` files and the :file:`prj.conf` file are merged together.
The combined configuration is saved in a :file:`zephyr/.config` file in your build directory.
This means that this file is available when building the application, but it is deleted when you clean the build directory.

To quickly test different configuration options, or to build your application in different variants, you can update the :file:`.config` file in the build directory.
Changes are picked up immediately, and you do not need to re-open the project in SES.

While it is possible to edit the :file:`.config` file directly, you should use SES or a tool like menuconfig or guiconfig to update it.
These tools present all available options and allow you to select the ones that you need.

To edit the file in SES, select :guilabel:`Project` -> :guilabel:`Configure nRF Connect SDK Project`.
If your application contains more than one image (see :ref:`ug_multi_image`), you must select the correct target.
To configure the parent image (the main application), select :guilabel:`menuconfig`.
The other options allow you to configure the child images.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run menuconfig or guiconfig.

To locate a specific configuration option, use the filter (:guilabel:`Jump to` in menuconfig and guiconfig).
The documentation for each :ref:`configuration option <configuration_options>` also lists the menu path where the option can be found.

.. important::
   All changes to the :file:`.config` file are lost when you clean your build directory.
   You can save it to another location, but you must then manually copy it back to your build directory.

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

Build types enable you to use different sets of configuration options for each board.
You can create several build type :file:`.conf` files per board and select one of them when building the application.
This means that you do not have to use one :file:`prj.conf` file for your project and modify it each time to fit your needs.

.. build_types_overview_end

.. note::
    Creating build types and selecting them is optional.
    This is a feature specific to the :ref:`application development in nRF Connect SDK <app_build_system>`.

.. _gs_modifying_build_types_creating:

Creating build type files
=========================

To create custom build type files for your application instead of using a single :file:`prj.conf` file, complete the following steps:

1. During :ref:`application development <zephyr:application>`, follow the procedure for creating the application until after the step where you create the :file:`CMakeLists.txt` file.
#. In the :file:`CMakeLists.txt` file, define the file name pattern for configuration files.
   For example::

    set(CONF_FILE "app_${CMAKE_BUILD_TYPE}.conf")

   In this define, ``CMAKE_BUILD_TYPE`` will be used for selecting the build type.
#. Optionally, include an if statement that checks for the presence of the selected build type configuration files.
   For an example, see :file:`applications/nrf_desktop/CMakeLists.txt`.
#. Continue the application creation procedure by setting the Kconfig configuration options.
#. Save the :file:`.conf` file in the application directory with a name that matches the file name pattern defined in CMakeLists.
   For example, :file:`app_ZRelease.conf`.
   In this file name, ``ZRelease`` is the build type name.

You can now select build types in SES or from command line.

Selecting a build type in SES
=============================

.. build_types_selection_ses_start

To select the build type in SEGGER Embedded Studio:

1. Go to :guilabel:`File` > :guilabel:`Open nRF Connect SDK project`, select the current project, and specify the board name and build directory.
#. Select :guilabel:`Extended Settings`.
#. In the :guilabel:`Extra CMake Build Options` field, specify ``-DCMAKE_BUILD_TYPE=selected_build_type``.
   For example, for ``ZRelease`` set the following value: ``-DCMAKE_BUILD_TYPE=ZRelease``.
#. Do not select :guilabel:`Clean Build Directory`.
#. Click :guilabel:`OK` to re-open the project.


.. note::
   You can also specify the build type in the :guilabel:`Additional CMake Options` field in :guilabel:`Tools` -> :guilabel:`Options` -> :guilabel:`nRF Connect`.
   However, the changes will only be applied after re-opening the project.
   Reloading the project is not sufficient.

.. build_types_selection_ses_end

Selecting a build type from command line
========================================

.. build_types_selection_cmd_start

To select the build type when building the application from command line, specify the build type by adding the following parameter to the ``west build`` command:

.. parsed-literal::
   :class: highlight

   -- -DCMAKE_BUILD_TYPE=\ *selected_build_type*\

For example, you can replace the *selected_build_type* variable to build the ``ZRelease`` firmware for PCA20041 by running the following command in the project directory:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840_pca20041 -d build_pca20041 -- -DCMAKE_BUILD_TYPE=ZRelease

The ``build_pca20041`` parameter specifies the output directory for the build files.

.. build_types_selection_cmd_end

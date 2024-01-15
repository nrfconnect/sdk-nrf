.. _configuring_cmake:

Adding files and configuring CMake
##################################

.. contents::
   :local:
   :depth: 2

As described in more detail in :ref:`app_build_system`, the Zephyr and |NCS| build systems are based on `CMake <CMake documentation_>`_.
For this reason, every application in Zephyr and the |NCS| must have a :file:`CMakeLists.txt` file.
This file is the entry point of the build system as it specifies the application source files for the compiler to include in the :ref:`configuration phase <app_build_system>`.

Maintaining CMakeLists.txt
**************************

The recommended method to maintain and update the :file:`CMakeLists.txt` file is to use the |nRFVSC|.
The extension provides support for the `source control with west`_ and `CMake build system`_, including `build configuration management <How to work with build configurations_>`_ and `source and config files overview <Details View_>`_.

.. _modifying_files_compiler:

Adding source files to CMakeLists.txt
*************************************

You can add source files to the ``app`` CMake target with the :c:func:`target_sources` function provided by CMake.

Pay attention to the following configuration options:

* If your application is complex, you can split it into subdirectories.
  These subdirectories can provide their own :file:`CMakeLists.txt` files.
  (The main :file:`CMakeLists.txt` file needs to include those.)
* The build system searches for header files in include directories.
  Add additional include directories for your application with the :c:func:`target_include_directories` function provided by CMake.
  For example, if you want to include an :file:`inc` directory, the code would look like the following:

  .. code-block:: c

     target_include_directories(app PRIVATE inc)

See :ref:`zephyr:zephyr-app-cmakelists` in the Zephyr documentation for more information about how to edit :file:`CMakeLists.txt`.

.. note::
    You can also read the `CMake Tutorial`_ in the CMake documentation for a better understanding of how :file:`CMakeLists.txt` are used in a CMake project.
    This tutorial however differs from Zephyr and |NCS| project configurations, so use it only as reference.

.. _cmake_options:

Providing CMake options
***********************

You can provide additional options for building your application to the CMake process, which can be useful, for example, to switch between different build scenarios.
These options are specified when CMake is run, thus not during the actual build, but when configuring the build.

For information about what variables can be set and how to add these variables in your project, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      If you work with the |nRFVSC|, you can specify project-specific CMake options when you add the :term:`build configuration` for a new |NCS| project.
      See `How to build an application`_ in the |nRFVSC| documentation.

   .. group-tab:: Command line

      If you work on the command line, pass the additional options to the ``west build`` command when :ref:`building`.
      The options must be added after a ``--`` at the end of the command.
      See :ref:`zephyr:west-building-cmake-args` for more information.

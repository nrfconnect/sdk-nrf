.. _configuring_kconfig:

Configuring Kconfig
###################

.. contents::
   :local:
   :depth: 2

You can change the software-related configuration temporarily or permanently.
The temporary build files are deleted when you clean the build directory with the ``pristine`` target (see Zephyr's :ref:`zephyr:application_rebuild` for more information).

The :ref:`Kconfig Reference <configuration_options>` provides the documentation for each configuration option and lists the menu path where the option can be found.

.. _configuration_temporary_change:

Temporary Kconfig changes
*************************

When building your application, the different :file:`.config`, :file:`*_defconfig` files, and the :file:`prj.conf` file are merged together and then processed by Kconfig.
The resulting configuration is written to the :file:`zephyr/.config` file in your :file:`build` directory.
This means that this file is available when building the application until you clean the build directory pristinely.

.. note::
    While it is possible to edit the :file:`.config` file directly, you should use the nRF Kconfig GUI in the |nRFVSC| or a tool like menuconfig or guiconfig to update it.
    These tools present all available options and allow you to select the ones that you need.
    They also show the dependencies between the options and their limitations.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      Use the nRF Kconfig GUI in the |nRFVSC| to select the desired options.
      The GUI organizes the Kconfig options in a hierarchical list and lets you view and manage your selection.

      To locate a specific configuration option, use the **Search modules** field.
      Read the `Configuring with nRF Kconfig`_ page in the |nRFVSC| documentation for more information.

      Alternatively, you can configure your application in the |nRFVSC| using menuconfig.
      Open the **More actions..** menu next to `Kconfig action in the Actions View`_ to start menuconfig in the extension.

   .. group-tab:: Command line

      To quickly test different configuration options, or to build your application in different variants, you can update the :file:`.config` file in the build directory.
      Changes are picked up immediately.

      Alternatively, you can configure your application using menuconfig.
      For this purpose, run the following command when :ref:`programming_cmd`.

      .. code-block:: console

         west build -t menuconfig

      See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run menuconfig or guiconfig.
      To locate a specific configuration option, use the **Jump to** field.

.. _configuration_temporary_change_single_build:

Temporary Kconfig changes for a single build
============================================

You can also apply temporary Kconfig changes to a single build by :ref:`providing the value of a chosen Kconfig option as a CMake option <cmake_options>`.
The Kconfig option configuration will be available until you clean the build directory pristinely.

.. _configuration_permanent_change:

Permanent Kconfig changes
*************************

To configure your application and maintain the configuration after you clean the build directory pristinely, you need to specify the configuration in one of the permanent configuration files.
In most of the cases, this means editing the default :file:`prj.conf` file of the application, but you can also use an extra Kconfig fragment file.

The Kconfig fragment files are configuration files used for building an application image with or without software support that is enabled by specific Kconfig options.
In these files, you can specify different values for configuration options that are defined by a library or board, and you can add configuration options that are specific to your application.
Examples include whether to add networking support or which drivers are needed by the application.
Kconfig fragments are applied on top of the default :file:`prj.conf` file and use the :file:`.conf` file extension.
When they are board-specific, they are placed in the :file:`boards` folder, are named :file:`<board>.conf`, and they are applied on top of the default Kconfig file for the specified board.

See :ref:`zephyr:setting_configuration_values` in the Zephyr documentation for information on how to change the configuration permanently and :ref:`how the additional files are applied <zephyr:initial-conf>`.

.. tip::
   Reconfiguring through menuconfig only changes the specific setting and the invisible options that are calculated from it.
   It does not adjust visible symbols that have already defaulted to a value even if this default calculation is supposed to be dependent on the changed setting.
   This may result in a bloated configuration compared to changing the setting directly in :file:`prj.conf`.
   See the section Stuck symbols in menuconfig and guiconfig on the :ref:`kconfig_tips_and_tricks` in the Zephyr documentation for more information.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      If you work with |nRFVSC|, you can use one of the following options:

      * Edit the :file:`prj.conf` directly in |VSC|.
      * Select an extra Kconfig fragment file when you `build an application <How to build an application_>`_.
      * Edit the Kconfig options in :file:`prj.conf` using the nRF Kconfig GUI and save changes permanently to an existing or new :file:`prj.conf` file.

      See the `extension's documentation about Kconfig <Configuring with nRF Kconfig_>`_ for more information.

   .. group-tab:: Command line

      If you work on the command line, use one of the following options:

      * Edit the :file:`prj.conf` directly and run the standard ``west build`` command.
      * Pass the additional options to the ``west build`` command by adding them after a ``--`` at the end of the command.

        .. ncs-include:: develop/west/build-flash-debug.rst
           :docset: zephyr
           :start-after: will take effect.
           :end-before: To enable :makevar:`CMAKE_VERBOSE_MAKEFILE`,

        See :ref:`zephyr:west-building-cmake-config` for more information.

The configuration changes in :file:`prj.conf` are automatically picked up by the build system when you rebuild the application.

.. _kconfig_override_warnings:

Override Kconfig warnings
*************************

Kconfig options often depend on each other, and because of this the build system can override any Kconfig changes you make.
If that happens, a warning will be printed in the build log, but the build may still complete successfully.

Here is an example for such a warning:

.. code-block:: console

   warning: UART_CONSOLE (defined at drivers/console/Kconfig:43) was assigned the value 'y' but got the
   value 'n'. Check these unsatisfied dependencies: SERIAL (=n), SERIAL_HAS_DRIVER (=n). See
   http://docs.zephyrproject.org/latest/kconfig.html#CONFIG_UART_CONSOLE and/or look up UART_CONSOLE in
   the menuconfig/guiconfig interface. The Application Development Primer, Setting Configuration
   Values, and Kconfig - Tips and Best Practices sections of the manual might be helpful too.

Look for these warnings to make sure no Kconfig options are overridden unexpectedly.

These warnings might be more frequent if you edit the Kconfig files manually.
The Kconfig GUI and other Kconfig tools give you an overview over dependencies, which allows you to see which Kconfig options have been overridden and why before you build the project.

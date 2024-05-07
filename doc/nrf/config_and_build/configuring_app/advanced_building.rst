.. _building_advanced:

Advanced building procedures
############################

.. contents::
   :local:
   :depth: 2

You can customize the basic :ref:`building <building>` procedures in a variety of ways, depending on the configuration of your project.

.. _compiler_settings:

Advanced compiler settings
**************************

The application has full control over the build process.

Using Zephyr's configuration options is the standard way of controlling how the system is built.
These options can be found under Zephyr's menuconfig **Build and Link Features** > **Compiler Options**.
For example, to turn off optimizations, select :kconfig:option:`CONFIG_NO_OPTIMIZATIONS`.

Compiler options not controlled by the Zephyr build system can be controlled through the :kconfig:option:`CONFIG_COMPILER_OPT` Kconfig option.

Optional build parameters
*************************

Here are some of the possible options you can use:

* You can provide :ref:`custom CMake options <cmake_options>` to the build command.
* You can include the *directory_name* parameter to build from a directory other than the current directory.
* You can :ref:`start menuconfig with the west command <configuration_temporary_change>` to configure your application.
* You can :ref:`reuse an existing build directory <zephyr:west-building-pristine>` for building another application for another board or board target by passing ``-p=auto`` to ``west build``.
* You can :ref:`run unit tests with the west command <running_unit_tests>` with the ``-t run`` parameter from the unit test directory.

For more information on other optional build parameters, run the ``west build -h`` help text command.

.. |output_files_note| replace:: For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

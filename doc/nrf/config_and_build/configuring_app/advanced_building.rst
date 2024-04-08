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
* You can use the *build_target@board_revision* parameter to get extra devicetree overlays with new features available for a board version.
  The *board_revision* is printed on the label of your DK, just below the PCA number.
  For example, if you run the west build command with an additional parameter ``@1.0.0`` for nRF9160 build target, it adds the external flash on the nRF9160 DK that was available since :ref:`board version v0.14.0 <nrf9160_board_revisions>`.
* You can :ref:`start menuconfig with the west command <configuration_temporary_change>` to configure your application.
* You can :ref:`reuse an existing build directory <zephyr:west-building-pristine>` for building another application for another board or build target by passing ``-p=auto`` to ``west build``.

For more information on other optional build parameters, run the ``west build -h`` help text command.

.. |output_files_note| replace:: For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

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

.. _common_sample_components:

Common sample components for development
****************************************

|common_sample_components_desc|

To learn more about how to use the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option, see the respective device guides for :ref:`nRF52 Series <ug_nrf52_developing_ble_fota>` and the :ref:`nRF5340 DK <ug_nrf53_developing_ble_fota>`.

.. _optional_build_parameters:

Optional build parameters
*************************

Here are some of the possible options you can use:

* You can provide :ref:`custom CMake options <cmake_options>` to the build command.
* You can pass ``--no-sysbuild`` to ``west build`` to build without :ref:`configuration_system_overview_sysbuild`.
  (In the |NCS|, :ref:`building with sysbuild is enabled by default <sysbuild_enabled_ncs>`.)
* You can include the *directory_name* parameter to build from a directory other than the current directory.
* You can specify a *destination_directory_name* parameter to choose where the build files are generated.
  If not specified, the build files are automatically generated in :file:`build/zephyr/`.
* You can :ref:`start menuconfig with the west command <configuration_temporary_change>` to configure your application.
* You can :ref:`reuse an existing build directory <zephyr:west-building-pristine>` for building another application for another board or board target by passing ``-p=auto`` to ``west build``.
* You can :ref:`run unit tests with the west command <running_unit_tests>` with the ``-t run`` parameter from the unit test directory.
* You can use the ``--domain`` parameter to :ref:`build for a single domain <zephyr:west-multi-domain-builds>`.
  This parameter can also be used for :ref:`programming <zephyr:west-multi-domain-flashing>` and :ref:`debugging <zephyr:west-multi-domain-debugging>` multiple domains.

For more information on other optional build parameters, run the ``west build -h`` help text command.

.. |output_files_note| replace:: For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

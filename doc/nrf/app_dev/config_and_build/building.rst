.. _building:

Building an application
#######################

.. contents::
   :local:
   :depth: 2

|application_sample_definition|

After you have :ref:`created an application <create_application>`, you need to build it in order to be able to program it.
Just as for creating the application, you can build the application using either the |nRFVSC| or the command line.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      To build with the |nRFVSC|, you first need to create a build configuration.
      For instructions, see `How to build an application`_ in the extension documentation.

      By default, the extension runs both stages of the CMake build (:ref:`configuration phase and building phase <app_build_system>`).
      If you want to only set up the build configuration without building it, make sure the :guilabel:`Build after generating configuration` is not selected.

      To build with :ref:`configuration_system_overview_sysbuild`, :ref:`keep the default setting selected <sysbuild_enabled_ncs>` or select the :guilabel:`Use sysbuild` radio button.

      If you want to build with custom options or scripts, read about `Binding custom tasks to actions`_ in the extension documentation.

      .. note::
          |ncs_oot_sample_note|

      |output_files_note|

   .. group-tab:: Command line

      Complete the following steps to build on the command line:

      1. |open_terminal_window_with_environment|
      #. Go to the specific application directory.

         For example, if you want to build the :ref:`at_client_sample` sample, run the following command to navigate to its directory:

         .. code-block:: console

            cd nrf/samples/cellular/at_client

      #. Build the application by using the following west command with the *board_target* specified:

         .. parsed-literal::
            :class: highlight

            west build -b *board_target*

         See :ref:`programming_board_names` for more information on the supported boards and board targets.
         The board targets supported for a given application are always listed in its requirements section.

         |sysbuild_autoenabled_ncs|

         The command can be expanded with :ref:`optional_build_parameters`, such as :ref:`custom CMake options <cmake_options>` or the ``--no-sysbuild`` parameter that disables building with sysbuild.
         |parameters_override_west_config|

       After running the ``west build`` command, the build files can be found in the main build directory or in the application-named sub-directories in the main build directory (or both, depending on your project structure).
       |output_files_note|
       For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.

       For more information on building using the command line, see :ref:`Building <zephyr:west-building>` in the Zephyr documentation.

.. note::
    |application_sample_long_path_windows|

.. _building_advanced:

Advanced building procedures
****************************

You can customize the basic building procedures in a variety of ways, depending on the configuration of your project.

.. _compiler_settings:

Advanced compiler settings
==========================

The application has full control over the build process.

Using Zephyr's configuration options is the standard way of controlling how the system is built.
These options can be found under Zephyr's menuconfig **Build and Link Features** > **Compiler Options**.
For example, to turn off optimizations, select :kconfig:option:`CONFIG_NO_OPTIMIZATIONS`.

Compiler options not controlled by the Zephyr build system can be controlled through the :kconfig:option:`CONFIG_COMPILER_OPT` Kconfig option.

.. _common_sample_components:

Common sample components for development
========================================

|common_sample_components_desc|

To learn more about how to use the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option, see the respective device guides for :ref:`nRF52 Series <ug_nrf52_developing_ble_fota>` and the :ref:`nRF5340 DK <ug_nrf53_developing_ble_fota>`.

.. _optional_build_parameters:

Optional build parameters
=========================

You can customize the basic ``west build`` command in a variety of ways.
The following table contains some of the commonly used parameters in the |NCS|.

For more options, see Zephyr's :ref:`zephyr:west-building` or run the ``west --help`` and ``west build --help`` commands.

|parameters_override_west_config|

.. list-table:: Optional build parameters (selection)
   :header-rows: 1

   * - Parameter
     - Usage
     - Example
   * - :ref:`Custom CMake options <cmake_options>`
     - Provide additional options for building your application to the CMake process.
     - See the :ref:`dedicated section <cmake_options>`
   * - ``--no-sysbuild``
     - Explicitly build without :ref:`configuration_system_overview_sysbuild`.
       (In the |NCS|, :ref:`building with sysbuild is enabled by default <sysbuild_enabled_ncs>`.)
     - ``west build -b nrf52840dk/nrf52840 --no-sysbuild``
   * - ``-vvv``
     - Enable a detailed :ref:`zephyr:west-building-verbose` log, which includes the full commands used by the build system to generate the :ref:`app_build_output_files`.
     - ``west build -b nrf52840dk/nrf52840 -vvv``
   * - *directory_name*
     - Build from a directory other than the current directory.
     - ``west build -b nrf5340dk/nrf5340/cpuapp/ns nrf/samples/tfm/tfm_psa_template``
   * - ``-d``
     - Specify the build directory where the :ref:`app_build_output_files` are to be placed.
       If not specified, the build files are automatically generated in :file:`build/zephyr/`.
     - ``west build -b nrf52840dk/nrf52840 -d local_build``
   * - ``--domain``
     - :ref:`Build for a single domain <zephyr:west-multi-domain-builds>` in a multi-domain build.
       This parameter can also be used for :ref:`programming <zephyr:west-multi-domain-flashing>` and :ref:`debugging <zephyr:west-multi-domain-debugging>` multiple domains.
     - ``west build -b nrf52840dk/nrf52840 --domain hello_world``
   * - ``menuconfig``
     - :ref:`Start menuconfig <configuration_temporary_change>` to configure your application's Kconfig options.
     - ``west build -t menuconfig``
   * - ``-p=auto``
     - :ref:`Reuse an existing build directory <zephyr:west-building-pristine>` for building another application for another board or board target.
     - ``west build -b nrf52840dk/nrf52840 -p=auto``
   * - ``test``
     - :ref:`Run unit tests with the west command <running_unit_tests>` from the unit test directory (with the :file:`testcase.yaml` file).
     - ``west build -b native_sim -t run``

.. |output_files_note| replace:: For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

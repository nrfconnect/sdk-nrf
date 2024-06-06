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
.. _building_overlay_files:

Providing CMake options
***********************

You can provide additional options for building your application to the CMake process, which can be useful, for example, to switch between different build scenarios.
These options are specified when CMake is run, thus not during the actual build, but when configuring the build.

The |NCS| uses the same CMake build variables as Zephyr and they are compatible with both CMake and west.

For the complete list of build variables in Zephyr and more information about them, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.
The following table lists the most common ones used in the |NCS|:

.. list-table:: Common build system variables in the |NCS|
   :header-rows: 1

   * - Variable
     - Purpose
     - CMake argument to use
   * - Name of the Kconfig option
     - | Set the given Kconfig option to a specific value :ref:`for a single build <configuration_temporary_change_single_build>`.
       | The Kconfig option name can be subject to :ref:`variable namespacing <zephyr:sysbuild_kconfig_namespacing>` and :ref:`sysbuild Kconfig namespacing <zephyr:sysbuild_kconfig_namespacing>`.
     - ``-D<name_of_Kconfig_option>=<value>``
   * - :makevar:`CONF_FILE`
     - | Select the base Kconfig configuration file to be used for your application and override the :ref:`autoselection process <zephyr:initial-conf>`.
       | This variable has also been used to select one of the available :ref:`build types <modifying_build_types>`, if the application or sample supports any.
       | Using this variable for build type selection is deprecated and is being gradually replaced by :makevar:`FILE_SUFFIX`, but :ref:`still required for some applications <modifying_build_types>`.
     - | ``-DCONF_FILE=<file_name>.conf``
       | ``-DCONF_FILE=prj_<build_type_name>.conf``
   * - :makevar:`SB_CONF_FILE`
     - Select the base :ref:`sysbuild <configuration_system_overview_sysbuild>` Kconfig configuration file to be used for your application and override the :ref:`autoselection process <zephyr:initial-conf>`.
     - ``-DSB_CONF_FILE=<file_name>.conf``
   * - :makevar:`EXTRA_CONF_FILE`
     - Provide additional :ref:`Kconfig fragment files <configuration_permanent_change>` to be "mixed in" with the base configuration file.
     - ``-DEXTRA_CONF_FILE=<file_name>.conf``
   * - :makevar:`DTC_OVERLAY_FILE`
     - Select the base :ref:`devicetree overlay files <configuring_devicetree>` to be used for your application and override the :ref:`autoselection process <zephyr:set-devicetree-overlays>`.
     - ``-DDTC_OVERLAY_FILE=<file_name>.overlay``
   * - :makevar:`EXTRA_DTC_OVERLAY_FILE`
     - Provide additional, custom :ref:`devicetree overlay files <configuring_devicetree>` to be "mixed in" with the base devicetree overlay file.
     - ``-DEXTRA_DTC_OVERLAY_FILE=<file_name>.overlay``
   * - :makevar:`SHIELD`
     - Select one of the supported :ref:`shields <shield_names_nrf>` for building the firmware.
     - ``-DSHIELD=<shield>`` (``-D<image_name>_SHIELD`` for images)
   * - :makevar:`FILE_SUFFIX`
     - | Select one of the available :ref:`suffixed configurations <zephyr:application-file-suffixes>`, if the application or sample supports any.
       | See :ref:`app_build_file_suffixes` for more information about their usage and limitations in the |NCS|.
       | This variable is gradually replacing :makevar:`CONF_FILE` for selecting build types.
     - ``-DFILE_SUFFIX=<configuration_suffix>`` (``-D<image_name>_FILE_SUFFIX`` for images)
   * - ``-S`` (west) or :makevar:`SNIPPET` (CMake)
     - Select one of the :ref:`zephyr:snippets` to add to the application firmware during the build.
     - | ``-S <name_of_snippet>`` (applies the snippet to all images)
       | ``-DSNIPPET=<name_of_snippet>`` (``-D<image_name>_SNIPPET=<name_of_snippet>`` for images)
   * - :makevar:`PM_STATIC_YML_FILE`
     - | Select a :ref:`static configuration file <ug_pm_static>` for the Partition Manager script.
       | For applications that *do not* use multiple images, the static configuration can be selected with :makevar:`FILE_SUFFIX` (see above).
     - ``-DPM_STATIC_YML_FILE=pm_static_<suffix>.yml``

You can use these parameters in both the |nRFVSC| and the command line.

The build variables are applied one after another, based on the order you provide them.
This is how you can specify them:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      See `How to build an application`_ in the |nRFVSC| documentation.
      You can specify the additional configuration variables when `setting up a build configuration <How to build an application_>`_:

      * :makevar:`FILE_SUFFIX` (and :makevar:`CONF_FILE`) - Select the configuration in the :guilabel:`Configuration` menu.
      * :makevar:`EXTRA_CONF_FILE` - Add the Kconfig fragment file in the :guilabel:`Kconfig fragments` menu.
      * :makevar:`EXTRA_DTC_OVERLAY_FILE` - Add the devicetree overlays in the :guilabel:`Devicetree overlays` menu.
      * Other variables - Provide CMake arguments in the :guilabel:`Extra CMake arguments` field, preceded by ``--``.

      For example, to build the :ref:`location_sample` sample for the nRF9161 DK with the nRF7002 EK Wi-Fi support, select ``nrf9161dk/nrf9161/ns`` in the :guilabel:`Board` menu, :file:`overlay-nrf7002ek-wifi-scan-only.conf` in the :guilabel:`Kconfig fragments` menu, and provide ``-- -DSHIELD=nrf7002ek`` in the :guilabel:`Extra CMake arguments` field.

   .. group-tab:: Command line

      Pass the additional options to the ``west build`` command when :ref:`building`.
      The CMake arguments must be added after a ``--`` at the end of the command.

      For example, to build the :ref:`location_sample` sample for the nRF9161 DK with the nRF7002 EK Wi-Fi support, the command would look like follows:

      .. code-block::

         west build -p -b nrf9161dk/nrf9161/ns -- -DSHIELD=nrf7002ek -DEXTRA_CONF_FILE=overlay-nrf7002ek-wifi-scan-only.conf

See :ref:`configuration_permanent_change` and Zephyr's :ref:`zephyr:west-building-cmake-args` for more information.

Examples of commands
--------------------

**Providing a CMake variable for build types**

  .. toggle::

     .. tabs::

        .. group-tab:: nRF Connect for VS Code

            To select the build type in the |nRFVSC|:

            1. When `building an application <How to build an application_>`_ as described in the |nRFVSC| documentation, follow the steps for setting up the build configuration.
            #. In the **Add Build Configuration** screen, select the desired :file:`.conf` file from the :guilabel:`Configuration` drop-down menu.
            #. Fill in other configuration options, if applicable, and click :guilabel:`Build Configuration`.

        .. group-tab:: Command line

            To select the build type when building the application from command line, specify the build type by adding the following parameter to the ``west build`` command:

            .. parsed-literal::
              :class: highlight

              -- -DCONF_FILE=prj_\ *selected_build_type*\.conf

            For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf52840dk/nrf52840`` by running the following command in the project directory:

            .. parsed-literal::
              :class: highlight

              west build -b nrf52840dk/nrf52840 -d build_nrf52840dk_nrf52840 -- -DCONF_FILE=prj_release.conf

            The ``build_nrf52840dk_nrf52840`` parameter specifies the output directory for the build files.
        ..

     ..

     If the selected board does not support the selected build type, the build is interrupted.
     For example, for the :ref:`nrf_machine_learning_app` application, if the ``nus`` build type is not supported by the selected board, the following notification appears:

     .. code-block:: console

        Configuration file for build type ``nus`` is missing.

  ..

.. _cmake_options_images:

Providing CMake options for specific images
===========================================

You can prefix the build variable names with the image name if you want the variable to be applied only to a specific image: ``-D<prefix>_<build_variable>=<file_name>``.
For example, ``-DEXTRA_CONF_FILE=external_crypto.conf`` will be applied to the default image for which you are building (most often, the application image), while ``-Dmcuboot_EXTRA_CONF_FILE=external_crypto.conf`` will be applied to the MCUboot image.

This feature is not available for setting Kconfig options.

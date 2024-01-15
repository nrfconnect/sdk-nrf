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

.. _gs_modifying_build_types:
.. _modifying_build_types:

Configuring build types
***********************

When the ``CONF_FILE`` variable contains a single file and this file name follows the naming pattern :file:`prj_<buildtype>.conf`, then the build type will be inferred to be *<buildtype>*.
The build type cannot be set explicitly.
The *<buildtype>* can be any string, but it is common to use ``release`` and ``debug``.

For information about how to set variables, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

The following software components can be made dependent on the build type:

* The Partition Manager's :ref:`static configuration <ug_pm_static>`.
  When the build type has been inferred, the file :file:`pm_static_<buildtype>.yml` will have precedence over :file:`pm_static.yml`.
* The :ref:`child image Kconfig configuration <ug_multi_image_permanent_changes>`.
  Certain child image configuration files located in the :file:`child_image/` directory can be defined per build type.

The devicetree configuration is not affected by the build type.

.. note::
    For an example of an application that uses build types, see the :ref:`nrf_desktop` application (:ref:`nrf_desktop_requirements_build_types`) or the :ref:`nrf_machine_learning_app` application (:ref:`nrf_machine_learning_app_requirements_build_types`).

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

      For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf52840dk_nrf52840`` by running the following command in the project directory:

      .. parsed-literal::
         :class: highlight

         west build -b nrf52840dk_nrf52840 -d build_nrf52840dk_nrf52840 -- -DCONF_FILE=prj_release.conf

      The ``build_nrf52840dk_nrf52840`` parameter specifies the output directory for the build files.

If the selected board does not support the selected build type, the build is interrupted.
For example, for the :ref:`nrf_machine_learning_app` application, if the ``nus`` build type is not supported by the selected board, the following notification appears:

.. code-block:: console

   Configuration file for build type ``nus`` is missing.

Optional build parameters
*************************

Here are some of the possible options you can use:

* Some applications contain configuration overlay files that enable specific features.
  These can be added to the ``west build`` command as follows:

  .. parsed-literal::
     :class: highlight

     west build -b *build_target* -- -DOVERLAY_CONFIG="overlay-feature1.conf;overlay-feature2.conf"

  See :ref:`configuration_permanent_change` and Zephyr's :ref:`zephyr:west-building-cmake-args` for more information.
* You can include the *directory_name* parameter to build from a directory other than the current directory.
* You can use the *build_target@board_revision* parameter to get extra devicetree overlays with new features available for a board version.
  The *board_revision* is printed on the label of your DK, just below the PCA number.
  For example, if you run the west build command with an additional parameter ``@1.0.0`` for nRF9160 build target, it adds the external flash on the nRF9160 DK that was available since :ref:`board version v0.14.0 <nrf9160_board_revisions>`.
* You can :ref:`start menuconfig with the west command <configuration_temporary_change>` to configure your application.
* You can :ref:`reuse an existing build directory <zephyr:west-building-pristine>` for building another application for another board or build target by passing ``-p=auto`` to ``west build``.

For more information on other optional build parameters, run the ``west build -h`` help text command.

.. |output_files_note| replace:: For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

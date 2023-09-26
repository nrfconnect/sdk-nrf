.. _gs_programming:
.. _programming:

Building and programming an application
#######################################

.. contents::
   :local:
   :depth: 2

|application_sample_definition|

For additional information, check the user guide for the hardware platform that you are using.
These user guides contain platform-specific instructions for building and programming.
For example, see :ref:`ug_nrf5340_building` in the :ref:`ug_nrf5340` user guide for information about programming an nRF5340 DK, or :ref:`Programming precompiled firmware images <programming_thingy>` and :ref:`building_pgming` for information about programming a Thingy:91.

.. note::
   |application_sample_long_path_windows|

.. _programming_vsc:

Building with |VSC|
*******************

For instructions about building with the |nRFVSC|, see `How to build an application`_.
If you want to build and program with custom options, read about the advanced `Custom launch and debug configurations`_.

For information about how to flash and erase in the |nRFVSC|, read about the `Application-specific flash options`_.

.. note::
   |ncs_oot_sample_note|

|output_files_note|

.. _programming_cmd:

Building on the command line
****************************

After completing the :ref:`command-line build setup <install_ncs>`, use the following steps to build |NCS| projects on the command line.

1.    Open a terminal window.

      If you have installed the |NCS| using the :ref:`legacy Toolchain Manager <gs_assistant>`, click the down arrow next to the version you installed and select :guilabel:`Open bash`.

      .. figure:: ../installation/images/gs-assistant_tm_dropdown.png
         :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

         The Toolchain Manager dropdown menu options

#.    Go to the specific application directory.

      For example, if you want to build the :ref:`at_client_sample` sample, run the following command to navigate to its directory:

      .. code-block:: console

         cd nrf/samples/cellular/at_client

#.    Build the application using the west command.
      The build target is specified by the parameter *build_target* in the west command as follows:

      .. parsed-literal::
         :class: highlight

         west build -b *build_target*

      See :ref:`programming_board_names` for more information on the supported boards and build targets.
      The board targets supported for a given application are always listed in its requirements section.

      After running the ``west build`` command, the build files can be found in :file:`build/zephyr`.
      |output_files_note|
      For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.

      .. important::
         If you are working with an nRF9160 DK, make sure to select the correct controller before you program the application to your development kit.

         Set the **SW10** switch (marked debug/prog) in the **NRF91** position to program the main controller, or in the **NRF52** position to program the board controller.
         In nRF9160 DK v0.9.0 and earlier, the switch is called **SW5**.
         See the `Device programming section in the nRF9160 DK User Guide`_ for more information.

#.    Connect the development kit to your PC using a USB cable.

      .. note::
         To program the nRF52840 Dongle instead of a development kit, skip the following instructions and follow the programming instructions in :ref:`zephyr:nrf52840dongle_nrf52840`.

#.    Power on the development kit.
#.    Program the application to the kit using the following command:

      .. code-block:: console

         west flash --erase

      This command erases the full flash memory before programming, which is the recommended approach.
      If the application depends on other flash memory areas (for example, if it uses the :ref:`zephyr:settings_api` partition where bonding information is stored), erasing the full kit before programming ensures that these areas are updated with the new content.

      As an alternative, you can also clear only those flash memory pages that are to be overwritten with the new application.
      With such approach, the old data in other areas will be retained.

      To erase only the areas of flash memory that are required for programming the new application, use the following command:

      .. code-block:: console

         west flash

      The ``west flash`` command automatically resets the kit and starts the application.

For more information on building and programming using the command line, see the Zephyr documentation on :ref:`zephyr:west-build-flash-debug`.

Optional build parameters
=========================

You can customize the basic ``west build`` command in a variety of ways.
Here are some of the possible options you can use:

* Some applications contain configuration overlay files that enable specific features.
  These can be added to the ``west build`` command as follows:

  .. parsed-literal::
     :class: highlight

     west build -b *build_target* -- -DOVERLAY_CONFIG="overlay-feature1.conf;overlay-feature2.conf"

  See :ref:`configuration_permanent_change` and Zephyr's :ref:`zephyr:west-building-cmake-args` for more information.
* You can include the *directory_name* parameter to build from a directory other than the application directory.
* You can use the *build_target@board_revision* parameter to get extra devicetree overlays with new features available for a board version.
  The *board_revision* is printed on the label of your DK, just below the PCA number.
  For example, if you run the west build command with an additional parameter ``@1.0.0`` for nRF9160 build target, it adds the external flash on the nRF9160 DK that was available since board version 0.14.0.
* You can :ref:`start menuconfig with the west command <configuration_temporary_change>` to configure your application.
* You can reuse an existing build directory for building another application for another board or build target by passing ``-p=auto`` to ``west build``.

For more information on other optional build parameters, run the ``west build -h`` help text command.

.. |output_files_note| replace:: For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

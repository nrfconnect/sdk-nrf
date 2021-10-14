.. _gs_programming:

Building and programming an application
#######################################

.. contents::
   :local:
   :depth: 2

|application_sample_definition|

For additional information, check the user guide for the hardware platform that you are using.
These user guides contain platform-specific instructions for building and programming.
For additional information, check the user guide for the hardware platform that you are using.
For example, see :ref:`ug_nrf5340_building` in the :ref:`ug_nrf5340` user guide for information about programming an nRF5340 DK, or :ref:`precompiled_fw` and :ref:`building_pgming` for information about programming a Thingy:91.

.. _gs_programming_vsc:

Building with the VS Code extension
***********************************

|vsc_extension_instructions|

.. _gs_programming_ses:

Building with SES
*****************

.. build_SES_projimport_open_start

Complete the following steps to build |NCS| projects with SES after :ref:`installing SEGGER Embedded Studio <installing_ses>` and :ref:`completing the first time setup <build_environment>`:

1. Start SEGGER Embedded Studio.

   If you have installed the |NCS| using the :ref:`gs_app_tcm`, click the :guilabel:`Open Segger Embedded Studio` button next to the version you installed to start SES.
   If you have installed SES manually, run the :file:`emStudio` executable file from the :file:`bin` directory.

   .. figure:: images/gs-assistant_tm_installed.png
      :alt: The Toolchain Manager options after installing the nRF Connect SDK version, cropped

      The Toolchain Manager options after installing the |NCS| version

#. Select :guilabel:`File` > :guilabel:`Open nRF Connect SDK Project`.

    .. figure:: images/ses_open.png
       :alt: Open nRF Connect SDK Project menu

       Open nRF Connect SDK Project menu

#. To add a project to SES, you must specify the following information:

   * :guilabel:`nRF Connect SDK Release` - Select the |NCS| version that you want to work with.

     The drop-down list contains the current version of all |NCS| installation directories that SES knows about.
     To add a missing |NCS| installation directory to that list, run ``west zephyr-export`` in the installation repository or define the Zephyr base to point to the directory (see :ref:`setting_up_SES`).
   * :guilabel:`nRF Connect Toolchain Version` - If you used the Toolchain Manager to install the |NCS|, select the version of the toolchain that works with the selected |NCS| version.
     Otherwise, select NONE and make sure that your SES environment is configured correctly (see :ref:`setting_up_SES`).

     .. note::
        The drop-down list contains only toolchain versions that are compatible with the selected |NCS| version.

   * :guilabel:`Projects` - Select the project that you want to work with.

     The drop-down list contains a selection of applications from the sdk-nrf and sdk-zephyr repositories.
     Select any of the checkboxes underneath to add the applications from that area to the drop-down list.
     To add projects to the drop-down list, for example, your own custom projects, click :guilabel:`...` and select the folder that contains the projects that you want to add.
   * :guilabel:`Board Name` - Select the board that you want to work with.

     The drop-down list contains the build targets for all Nordic Semiconductor boards that are defined in the sdk-nrf and sdk-zephyr repositories.
     Select any of the checkboxes underneath to add the build targets from that area to the drop-down list.
     To add build targets to the drop-down list, for example, targets for your own custom board, click :guilabel:`...` and select the folder that contains the board definitions.
   * :guilabel:`Build Directory` - Select the folder in which to run the build.
     The field is filled automatically based on the selected board name, but you can specify a different directory.
   * :guilabel:`Clean Build Directory` - Select this option to ensure that you are not building with an outdated build cache.
   * :guilabel:`Extended Settings` - Select this option to display a field where you can specify additional CMake options to be used for building.
     See :ref:`cmake_options`.

   .. build_SES_projimport_open_end

   The following figure shows an example configuration for the Asset Tracker application built for the ``nrf9160dk_nrf9160_ns`` build target:

   .. figure:: images/ses_config.png
      :alt: Opening the Asset Tracker project

      Opening the Asset Tracker project

   .. build_SES_projimport_start

4. Click :guilabel:`OK` to add the project to SES.
   You can now work with the project in the IDE.

   .. build_SES_projimport_note_start

   .. note::

      At this stage, you might get an error indicating a project load failure. For example::

        Can't load project file
        The project file <filepath> is invalid.
        The reported error is 'solution load command failed (1)'

      This issue might be caused by a variety of problems, such as incorrectly specified project file paths.
      SES helps you to identify the source of the issue by providing a text output with detailed information about the error.
      Make sure to click :guilabel:`OK` on the error pop-up message and then inspect the text output in SES.

   .. build_SES_projimport_note_end

5. Build and program your project.

   The required steps differ depending on if you build a single application or a multi-image project (such as the nRF9160 samples, which include :ref:`SPM <secure_partition_manager>`).

   .. imp_note_nrf91_start

   .. important::
      If you are working with an nRF9160 DK, make sure to select the correct controller before you program the application to your development kit.

      Set the **SW10** switch (marked debug/prog) in the **NRF91** position to program the main controller, or in the **NRF52** position to program the board controller.
      In nRF9160 DK v0.9.0 and earlier, the switch is called **SW5**.
      See the `Device programming section in the nRF9160 DK User Guide`_ for more information.

   .. imp_note_nrf91_end

   To build and program an application:

   a. Select your project in the Project Explorer.
      The project name displays in bold when it is selected.
   #. From the menu, select :guilabel:`Build` > :guilabel:`Build Solution`.
      Alternatively, if you are working with a single-image application, you can choose the :guilabel:`Build and Debug` option that builds the application and programs it to a connected development kit when the build has completed.
   #. When the build completes, you can program the application to a connected development kit:

      * For a single-image application, select :guilabel:`Target` > :guilabel:`Download zephyr/zephyr.elf`.
      * For a multi-image application, depending on your build target:

        * If you are programming a SoC from the nRF53 Series and you also need to update the network core, you must add the network core project in |SES| and complete the additional steps, as described in the :ref:`ug_nrf5340_ses_multi_image` section of :ref:`ug_nrf5340`.
          This is because programming the :file:`merged.hex` file at this stage updates only the application core.
        * If you are not programming an nRF53 Series SoC or you do not need to update the network core, select :guilabel:`Target` > :guilabel:`Download zephyr/merged.hex`.

   If a "Project out-of-date" warning appears, click :guilabel:`No` to ignore it and leave the option to show the dialog again selected:

   .. figure:: images/ses_nrf5340_netcore_download.png
      :alt: Ignore any 'Project out-of-date' warnings

      Ignore any 'Project out-of-date' warnings

   .. caution::
      If you click :guilabel:`Yes` and disable the option to show the dialog again, you will enter a loop because of a "no input files" error.
      To restore the default settings, select :guilabel:`Tools` > :guilabel:`Options` > :guilabel:`Building` and set :guilabel:`Confirm Automatically Build Before Debug` to ``Yes``.

#. To inspect the details of the code that was programmed and the memory usage, click :guilabel:`Debug` > :guilabel:`Go`.

   .. note::
   	In a multi-image build, this allows you to debug the source code of your application only.

If you get an error that a tool or command cannot be found, first make sure that the tool is installed.
If it is installed, verify that its location is correct in the :envvar:`PATH` variable or, if applicable, in the SES settings.

.. _gs_programming_cmd:

Building on the command line
****************************

After completing the :ref:`manual <build_environment_cli>` or :ref:`automatic <gs_app_installing-ncs-tcm>` command-line build setup, use the following steps to build |NCS| projects on the command line.

1.    Open a terminal window.

      If you have installed the |NCS| using the :ref:`gs_app_tcm`, click the down arrow next to the version you installed and select :guilabel:`Open bash`.

      .. figure:: images/gs-assistant_tm_dropdown.png
         :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

         The Toolchain Manager dropdown menu options

#.    Go to the specific application directory.
      For example, to build the :ref:`at_client_sample` sample, run the following command to navigate to its directory:

      .. code-block:: console

         cd nrf/samples/nRF9160/at_client


#.    Build the application using the west command.
      The build target is specified by the parameter *build_target* in the west command as follows:

      .. parsed-literal::
         :class: highlight

         west build -b *build_target*

      .. note::

	     To build from a directory other than the application directory, run the west build command with an additional parameter *directory_name*,  specifying the application directory.

      See :ref:`gs_programming_board_names` for more information on the supported boards and build targets.
      To reuse an existing build directory for building another application for another board or build target, pass ``-p=auto`` to ``west build``.

      If you want to configure your application, run the following west command:

      .. code-block:: console

         west build -t menuconfig

      See :ref:`configure_application` for additional information about configuring an application.

      After running the ``west build`` command, the build files can be found in :file:`build/zephyr`.
      For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents`.

      .. include:: gs_programming.rst
         :start-after: .. imp_note_nrf91_start
         :end-before: .. imp_note_nrf91_end

#.    Connect the development kit to your PC using a USB cable.

      .. note::
         To program the nRF52840 Dongle instead of a development kit, skip the following instructions and follow the programming instructions in :ref:`zephyr:nrf52840dongle_nrf52840`.

#.    Power on the development kit.
#.    Program the application to the kit using the following command:

      .. code-block:: console

         west flash --erase

      This command clears the full flash memory before programming, which is the recommended approach.
      If the application depends on other flash memory areas (for example, if it uses the :ref:`zephyr:settings_api` partition where bonding information is stored), erasing the full kit before programming ensures that these areas are updated with the new content.

      As an alternative, you can also clear only those flash memory pages that are to be overwritten with the new application.
      With such approach, the old data in other areas will be retained.

      To erase only the areas of flash memory that are required for programming the new application, use the following command:

      .. code-block:: console

         west flash

      The ``west flash`` command automatically resets the kit and starts the application.

For more information on building and programming using the command line, see the Zephyr documentation on :ref:`zephyr:west-build-flash-debug`.

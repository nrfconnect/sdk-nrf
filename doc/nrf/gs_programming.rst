.. _gs_programming:

Building and programming a sample application
#############################################

.. contents::
   :local:
   :depth: 2

The recommended way of building and programming an |NCS| sample is to use the Nordic Edition of the SEGGER Embedded Studio (SES) IDE.


.. note::

   See :ref:`precompiled_fw` and :ref:`building_pgming` for information about programming a Thingy:91.

.. _gs_programming_ses:

Building with SES
*****************

.. build_SES_projimport_open_start

Complete the following steps to build |NCS| projects with SES after :ref:`installing SEGGER Embedded Studio <installing_ses>` and :ref:`completing the first time setup <build_environment>`:

1. Start SEGGER Embedded Studio.

   If you have installed the |NCS| using the :ref:`gs_app_tcm`, click :guilabel:`Open IDE` next to the version you installed to start SES.
   If you have installed SES manually, run :file:`bin/emStudio`.

#. Select :guilabel:`File` -> :guilabel:`Open nRF Connect SDK Project`.

    .. figure:: images/ses_open.png
       :alt: Open nRF Connect SDK Project menu

       Open nRF Connect SDK Project menu

#. To import a project into SES, you must specify the following information:

   * :guilabel:`nRF Connect SDK Release` - Select the |NCS| version that you want to work with.

     The drop-down list contains the current version of all |NCS| installation directories that SES knows about.
     To add a missing |NCS| installation directory to that list, run ``west zephyr-export`` in the installation repository or define the Zephyr base to point to the directory (see :ref:`setting_up_SES`).
   * :guilabel:`nRF Connect Toolchain Version` - If you used the Toolchain manager to install the |NCS|, select the version of the toolchain that works with the selected |NCS| version.
     Otherwise, select NONE and make sure that your SES environment is configured correctly (see :ref:`setting_up_SES`).

     .. note::
        The drop-down list contains only toolchain versions that are compatible with the selected |NCS| version.

   * :guilabel:`Projects` - Select the project that you want to work with.

     The drop-down list contains a selection of samples and applications from the sdk-nrf and sdk-zephyr repositories.
     Select any of the checkboxes underneath to add the samples from that area to the drop-down list.
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

   The following figure shows an example configuration for the Asset Tracker application built for the ``nrf9160dk_nrf9160ns`` build target:

   .. figure:: images/ses_config.png
      :alt: Opening the Asset Tracker project

      Opening the Asset Tracker project

   .. build_SES_projimport_start

4. Click :guilabel:`OK` to import the project into SES. You can now work with the
   project in the IDE.

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

      Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller, or in the **NRF52** position to program the board controller.
      See the `Device programming section in the nRF9160 DK User Guide`_ for more information.

   .. imp_note_nrf91_end

   To build and program an application:

      a. Select your project in the Project Explorer.
      #. From the menu, select :guilabel:`Build` -> :guilabel:`Build Solution`.
      #. When the build completes, you can program the sample to a connected development kit:

         * For a single-image application, select :guilabel:`Target` -> :guilabel:`Download zephyr/zephyr.elf`.
         * For a multi-image application, select :guilabel:`Target` -> :guilabel:`Download zephyr/merged.hex`.

      .. note::
	   Alternatively, choose the :guilabel:`Build and Debug` option.
	   :guilabel:`Build and Debug` will build the application and program it when
	   the build completes.

#. To inspect the details of the code that was programmed and the memory usage, click :guilabel:`Debug` -> :guilabel:`Go`.

   .. note::
   	In a multi-image build, this allows you to debug the source code of your application only.

If you get an error that a tool or command cannot be found, first make sure that the tool is installed.
If it is installed, verify that its location is correct in the PATH variable or, if applicable, in the SES settings.

.. _gs_programming_cmd:

Building on the command line
****************************

Complete the following steps to build |NCS| projects on the command line after completing the :ref:`command-line build setup <build_environment_cli>`.

1.    Open a terminal window.

      If you have installed the |NCS| using the :ref:`gs_app_tcm`, click the down arrow next to the version you installed and select :guilabel:`Open bash`.

#.    Go to the specific sample or application directory.
      For example, to build the :ref:`at_client_sample` sample, run the following command to navigate to the sample directory:

      .. code-block:: console

         cd nrf/samples/nRF9160/at_client


#.    Build the sample or application using the west command.
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

      After running the ``west build`` command, the build files can be found in ``build/zephyr``.
      For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents`.

      .. include:: gs_programming.rst
         :start-after: .. imp_note_nrf91_start
         :end-before: .. imp_note_nrf91_end

#.    Connect the development kit to your PC using a USB cable.
#.    Power on the development kit.
#.    Program the sample or application to the kit using the following command:

      .. code-block:: console

         west flash

      This command clears only the flash memory pages that are overwritten with the new application.
      If the application depends on other flash areas (for example, if it uses the :ref:`zephyr:settings_api` partition), erase the full kit before programming to ensure that these areas are updated with the new content.
      If you do not fully erase the kit, the old data in these areas will be retained.

      To fully erase the kit before programming the new application, use the following command:

      .. code-block:: console

         west flash --erase

      The ``west flash`` command automatically resets the kit and starts the application.

For more information on building and programming using the command line, see the Zephyr documentation on :ref:`zephyr:west-build-flash-debug`.

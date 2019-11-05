.. _gs_programming:

Building and programming a sample application
#############################################

The recommended way of building and programming an |NCS| sample is to use
the Nordic Edition of the SEGGER Embedded Studio (SES) IDE.


.. note::
	If you prefer to build your applications from the command line,
	see :ref:`zephyr:getting_started_run_sample`.

.. _gs_programming_ses:

Building with SES
*****************

Complete the following steps to build |NCS| projects with SES after :ref:`installing SEGGER Embedded Studio <installing_ses_win>` and :ref:`completing the first time setup <build_environment_win>`:

1. Run the file :file:`bin/emStudio`.

#. Select **File -> Open nRF Connect SDK Project**.

    .. figure:: images/ses_open.png
       :alt: Open nRF Connect SDK Project menu

       Open nRF Connect SDK Project menu

#. To import a project into SES, you must specify the following information:

	- **CMakeLists.txt** - the location of the :file:`CMakeLists.txt` project file of the sample that you want to work with
	- **Board Directory** - the location of the board description of the board for which to build the project
	- **Board Name** - the board name (select from the list that is populated based on the board directory)
	- **Build Directory** - the folder in which to run the build (automatically filled based on the board name, but you can specify a different directory)
	- **Delete Existing CMakeCache.txt** - select this option to ensure that you are not building with an outdated build cache

   The following figure shows an example configuration for the Asset Tracker application built for the ``nrf9160_pca10090ns`` board target:

   .. figure:: images/ses_config.png
      :alt: Opening the Asset Tracker project

      Opening the Asset Tracker project

#. Click **OK** to import the project into SES. You can now work with the
   project in the IDE.

   .. note::

      At this stage, you might get an error indicating a project load failure. For example::

        Can't load project file
        The project file <filepath> is invalid.
        The reported error is 'solution load command failed (1)'

      This issue might be caused by a variety of problems, such as incorrectly specified project file paths.
      SES helps you to identify the source of the issue by providing a text output with detailed information about the error.
      Make sure to click :guilabel:`OK` on the error pop-up message and then inspect the text output in SES.

#. Build and program your project.

   The required steps differ depending on if you build a single application or a multi-image project (such as the nRF9160 samples, which include :ref:`SPM <secure_partition_manager>`).

   .. important::
      If you are working with an nRF9160 DK, make sure to select the correct controller before you program the application to your board.

      Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller, or in the **NRF52** position to program the board controller.
      See the `Device programming section in the nRF9160 DK User Guide`_ for more information.

   To build and program an application:

      a. Select your project in the Project Explorer.
      #. From the menu, select **Build -> Build Solution**.
      #. When the build completes, you can program the sample to a connected board:

         * For a single-image application, select **Target -> Download zephyr/zephyr.elf**.
         * For a multi-image application, select **Target -> Download zephyr/merged.hex**.

      .. note::
	   Alternatively, choose the **Build and Debug** option.
	   **Build and Debug** will build the application and program it when
	   the build completes.

7. To inspect the details of the code that was programmed and the memory usage, click **Debug -> Go**.

   .. note::
   	In a multi-image build, this allows you to debug the source code of your application only.

If you get an error that a tool or command cannot be found, first make sure that the tool is installed.
If it is installed, verify that its location is correct in the PATH variable or, if applicable, in the SES settings.
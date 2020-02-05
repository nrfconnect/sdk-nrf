.. _gs_programming:

Building and programming a sample application
#############################################

The recommended way of building and programming an |NCS| sample is to use
the Nordic Edition of the SEGGER Embedded Studio (SES) IDE.


.. note::

   See :ref:`precompiled_fw` and :ref:`building_pgming` for information about programming a Thingy:91.

.. _gs_programming_ses:

Building with SES
*****************

.. build_SES_projimport_open_start

Complete the following steps to build |NCS| projects with SES after :ref:`installing SEGGER Embedded Studio <installing_ses>` and :ref:`completing the first time setup <build_environment>`:

1. Start SEGGER Embedded Studio by running :file:`bin/emStudio`.

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

.. build_SES_projimport_open_end

   The following figure shows an example configuration for the Asset Tracker application built for the ``nrf9160_pca10090ns`` board target:

   .. figure:: images/ses_config.png
      :alt: Opening the Asset Tracker project

      Opening the Asset Tracker project

.. build_SES_projimport_start

4. Click **OK** to import the project into SES. You can now work with the
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
      If you are working with an nRF9160 DK, make sure to select the correct controller before you program the application to your board.

      Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller, or in the **NRF52** position to program the board controller.
      See the `Device programming section in the nRF9160 DK User Guide`_ for more information.

   .. imp_note_nrf91_end

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

#. To inspect the details of the code that was programmed and the memory usage, click **Debug -> Go**.

   .. note::
   	In a multi-image build, this allows you to debug the source code of your application only.

If you get an error that a tool or command cannot be found, first make sure that the tool is installed.
If it is installed, verify that its location is correct in the PATH variable or, if applicable, in the SES settings.

.. _gs_programming_cmd:

Building on the command line
****************************

Complete the following steps to build |NCS| projects on the command line after completing the :ref:`command-line build setup <build_environment_cli>`.

1.    Open a terminal window.

#.    Go to the specific sample or application directory.
      For example, to build the :ref:`at_client_sample` sample, run the following command to navigate to the sample directory:

      .. code-block:: console

         cd nrf/samples/nRF9160/at_client


#.    Build the sample or application using the west command.
      The development board is specified by the parameter *board_name* in the west command as follows:

      .. parsed-literal::
         :class: highlight

         west build -b *board_name*

      .. note::

	     To build from a directory other than the sample or application directory, run the west build command with an additional parameter *directory_name*,  specifying the sample or application directory.

      See `Board names <Board names_>`_ for more information on the development boards.
      To reuse an existing build directory for building another sample or application for another board, pass ``-p=auto`` to ``west build``.

      If you want to configure your application, run the following west command:

      .. code-block:: console

         west build -t menuconfig

      See :ref:`configure_application` for additional information about configuring an application.

      After running the ``west build`` command, the build files can be found in ``build/zephyr``.
      For more information on the contents of the build directory, see the *Build Directory Contents* section in the Zephyr documentation on :ref:`zephyr:build_an_application`.

      .. include:: gs_programming.rst
         :start-after: .. imp_note_nrf91_start
         :end-before: .. imp_note_nrf91_end

#.    Connect the development board to your PC using a USB cable.
#.    Power on the development board.
#.    Program the sample or application to the board using the following command:

      .. code-block:: console

         west flash

      To fully erase the board before programming the new sample or application, use the command:

      .. code-block:: console

         west flash --erase

      The ``west flash`` command automatically resets the board and starts the sample or application.

For more information on building and programming using the command line, see the Zephyr documentation on :ref:`zephyr:west-build-flash-debug`.

.. _gs_programming_board_names:

Board names
***********

You can find the board names for the different development boards in the :ref:`zephyr:boards` section in the Zephyr documentation.
For your convenience, the following table lists the board names for Nordic Semiconductor's development kits.

.. _table:

+--------------------------------------------------------+-------------------------------------------------------+
| Development kits                                       | Board names                                           |
+========================================================+=======================================================+
| :ref:`nRF51 DK board (PCA10028)<nrf51_pca10028>`       | nrf51_pca10028                                        |
+--------------------------------------------------------+-------------------------------------------------------+
| :ref:`nRF52 DK board (PCA10040)<nrf52_pca10040>`       | nrf52_pca10040                                        |
+--------------------------------------------------------+-------------------------------------------------------+
| :ref:`nRF52840 DK board (PCA10056)<nrf52840_pca10056>` | nrf52840_pca10056                                     |
+--------------------------------------------------------+-------------------------------------------------------+
| :ref:`nRF5340 PDK board (PCA10095)<nrf5340_dk_nrf5340>`| nrf5340_dk_nrf5340_cpunet (for the network sample)    |
+                                                        +                                                       +
|                                                        | nrf5340_dk_nrf5340_cpuapp (for the application sample)|
+--------------------------------------------------------+-------------------------------------------------------+
| :ref:`nRF9160 DK board (PCA10090)<nrf9160_pca10090>`   | nrf9160_pca10090 (for the secure version)             |
+                                                        +                                                       +
|                                                        | nrf9160_pca10090ns (for the non-secure version)       |
+--------------------------------------------------------+-------------------------------------------------------+

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

1. Start SEGGER Embedded Studio.

   If you have installed the |NCS| using the :ref:`gs_app_tcm`, click :guilabel:`Open IDE` next to the version you installed to start SES.
   If you have installed SES manually, run :file:`bin/emStudio`.

#. Select **File -> Open nRF Connect SDK Project**.

    .. figure:: images/ses_open.png
       :alt: Open nRF Connect SDK Project menu

       Open nRF Connect SDK Project menu

#. To import a project into SES, you must specify the following information:

   * **CMakeLists.txt** - the location of the :file:`CMakeLists.txt` project file of the sample that you want to work with
   *  **Board Directory** - the location of the board description of the board for which to build the project
   *  **Board Name** - the board name (select from the list that is populated based on the board directory)
   * **Build Directory** - the folder in which to run the build (automatically filled based on the board name, but you can specify a different directory)
   * **Delete Existing CMakeCache.txt** - select this option to ensure that you are not building with an outdated build cache

.. build_SES_projimport_open_end

   The following figure shows an example configuration for the Asset Tracker application built for the ``nrf9160dk_nrf9160ns`` build target:

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

      If you have installed the |NCS| using the :ref:`gs_app_tcm`, click the down arrow next to the version you installed and select :guilabel:`Open bash`.

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

The following tables lists all boards and build targets for Nordic Semiconductor's hardware platforms.

Boards included in sdk-zephyr
=============================

The following boards are defined in the :file:`zephyr/boards/arm/` folder.
Also see the :ref:`zephyr:boards` section in the Zephyr documentation.

.. _table:

+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| Hardware platform | PCA number | Board name                                                      | Build target                          |
+===================+============+=================================================================+=======================================+
| nRF52 DK          | PCA10040   | :ref:`nrf52dk_nrf52832 <zephyr:nrf52dk_nrf52832>`               | ``nrf52dk_nrf52832``                  |
| (nRF52832)        |            +-----------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52dk_nrf52810 <zephyr:nrf52dk_nrf52810>`               | ``nrf52dk_nrf52810``                  |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF52833 DK       | PCA10100   | :ref:`nrf52833dk_nrf52833 <zephyr:nrf52833dk_nrf52833>`         | ``nrf52833dk_nrf52833``               |
|                   |            +-----------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52833dk_nrf52820 <zephyr:nrf52833dk_nrf52820>`         | ``nrf52833dk_nrf52820``               |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF52840 DK       | PCA10056   | :ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`         | ``nrf52840dk_nrf52840``               |
|                   |            +-----------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf52840dk_nrf52811 <zephyr:nrf52840dk_nrf52811>`         | ``nrf52840dk_nrf52811``               |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF52840 Dongle   | PCA10059   | :ref:`nrf52840dongle_nrf52840 <zephyr:nrf52840dongle_nrf52840>` | ``nrf52840dongle_nrf52840``           |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| Thingy:52         | PCA20020   | :ref:`thingy52_nrf52832 <zephyr:thingy52_nrf52832>`             | ``thingy52_nrf52832``                 |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF5340 PDK       | PCA10095   | :ref:`nrf5340pdk_nrf5340 <zephyr:nrf5340pdk_nrf5340>`           | ``nrf5340pdk_nrf5340_cpunet``         |
|                   |            |                                                                 |                                       |
|                   |            |                                                                 | ``nrf5340pdk_nrf5340_cpuapp``         |
|                   |            |                                                                 |                                       |
|                   |            |                                                                 | ``nrf5340pdk_nrf5340_cpuappns``       |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+
| nRF9160 DK        | PCA10090   | :ref:`nrf9160dk_nrf9160 <zephyr:nrf9160dk_nrf9160>`             | ``nrf9160dk_nrf9160``                 |
|                   |            |                                                                 |                                       |
|                   |            |                                                                 | ``nrf9160dk_nrf9160ns``               |
|                   |            +-----------------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`nrf9160dk_nrf52840 <zephyr:nrf9160dk_nrf52840>`           | ``nrf9160dk_nrf52840``                |
+-------------------+------------+-----------------------------------------------------------------+---------------------------------------+


Boards included in sdk-nrf
==========================

The following boards are defined in the :file:`nrf/boards/arm/` folder.

+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| Hardware platform | PCA number | Board name                                               | Build target                          |
+===================+============+==========================================================+=======================================+
| nRF Desktop       | PCA20041   | :ref:`nrf52840gmouse_nrf52840 <nrf_desktop>`             | ``nrf52840gmouse_nrf52840``           |
| Gaming Mouse      |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA20044   | :ref:`nrf52dmouse_nrf52832 <nrf_desktop>`                | ``nrf52dmouse_nrf52832``              |
| Mouse             |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA20045   | :ref:`nrf52810dmouse_nrf52810 <nrf_desktop>`             | ``nrf52810dmouse_nrf52810``           |
| Mouse             |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA20037   | :ref:`nrf52kbd_nrf52832 <nrf_desktop>`                   | ``nrf52kbd_nrf52832``                 |
| Keyboard          |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA10111   | :ref:`nrf52833dongle_nrf52833 <nrf_desktop>`             | ``nrf52833dongle_nrf52833``           |
| Dongle            |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| nRF Desktop       | PCA10114   | :ref:`nrf52820dongle_nrf52820 <nrf_desktop>`             | ``nrf52820dongle_nrf52820``           |
| Dongle            |            |                                                          |                                       |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+
| Thingy:91         | PCA20035   | :ref:`thingy91_nrf9160 <ug_thingy91>`                    | ``thingy91_nrf9160``                  |
|                   |            |                                                          |                                       |
|                   |            |                                                          | ``thingy91_nrf9160ns``                |
|                   |            +----------------------------------------------------------+---------------------------------------+
|                   |            | :ref:`thingy91_nrf52840 <ug_thingy91>`                   | ``thingy91_nrf52840``                 |
+-------------------+------------+----------------------------------------------------------+---------------------------------------+

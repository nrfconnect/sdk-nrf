.. _gs_programming:

Building and programming a sample application
#############################################

The recommended way of building and programming an |NCS| sample is to use
the Nordic Edition of the SEGGER Embedded Studio (SES) IDE.

SEGGER Embedded Studio is free of charge for use with Nordic Semiconductor
devices. After downloading it, you can register and activate a free license.


.. note::
	If you prefer to build your applications from the command line,
	see :ref:`zephyr:getting_started_run_sample`.

Building with SES
*****************

You must install a special version of SEGGER Embedded Studio to be able to open
and compile the projects in the |NCS|.
You can download it from the following links:

* `Windows x86 <http://segger.com/downloads/embedded-studio/embeddedstudio_arm_nordic_win_x86>`_
* `Windows x64 <http://segger.com/downloads/embedded-studio/embeddedstudio_arm_nordic_win_x64>`_
* `Mac OS x64 <http://segger.com/downloads/embedded-studio/embeddedstudio_arm_nordic_macos>`_
* `Linux x86 <http://segger.com/downloads/embedded-studio/embeddedstudio_arm_nordic_linux_x86>`_
* `Linux x64 <http://segger.com/downloads/embedded-studio/embeddedstudio_arm_nordic_linux_x64>`_

1. Extract the downloaded package and run the file :file:`bin/emStudio`.

#. Select **File -> Open nRF Connect SDK Project**.

    .. figure:: images/ses_open.png
       :alt: Open nRF Connect SDK Project menu

       Open nRF Connect SDK Project menu

#. The first time you import an |NCS| project, SES will prompt you to set the paths to the Zephyr Base directory and the GNU ARM Embedded Toolchain.
   Set the Zephyr Base directory to the full path to ``ncs\zephyr``.
   The GNU ARM Embedded Toolchain directory is the directory where you installed the toolchain (for example, ``c:\gnuarmemb``).

    .. figure:: images/ses_notset.png
       :alt: Zephyr Base Not Set prompt

       Zephyr Base Not Set prompt

   If you want to change these settings later, click **Tools -> Options** and select the **nRF Connect** tab (see :ref:`ses_options_figure`).

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

#. Build and flash your project.
   The required steps differ depending on if you build a single application or a multi-image project (such as the nRF9160 samples, which include :ref:`SPM <secure_partition_manager>`).

   .. important::
      If you are working with an nRF9160 DK, make sure to select the correct controller before you flash the application to your board.

      Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller, or in the **NRF52** position to program the board controller.
      See the `Device programming section in the nRF9160 DK User Guide`_ for more information.

   To build and flash a single application:

      a. Select your project in the Project Explorer.
      #. From the menu, select **Build -> Build zephyr/zephyr.elf**.
      #. When the build completes, you can flash the sample to a connected board.
         To do this, select **Target -> Download zephyr/zephyr.elf**.

      .. note::
	   Alternatively, choose the **Build and Debug** option.
	   **Build and Debug** will build the application and flash it when
	   the build completes.

   To build and flash a multi-image project:

      a. Select your project in the Project Explorer.
      #. From the menu, select **Build -> Build Solution**.
      #. When the multi-image build completes, you can flash the sample to a connected board.
         To do this, select **Target -> Download File -> Download Intel Hex File**.
         Navigate to the ``zephyr`` folder in your build directory and choose ``merged.hex``.

7. To inspect the details of the flashed code and the memory usage, click **Debug -> Go**.

   .. note::
   	In a multi-image build, this allows you to debug the source code of your application only.

.. _gs_programming_ts:

Troublehooting SES
******************

When using SES to build the |NCS| samples,
it might return an error indicating a project load failure. For example::

	Can't load project file
	The project file <filepath> is invalid.
	The reported error is 'solution load command failed (1)'

This issue might be caused by a variety of problems, such as incorrectly specified project file paths.
SES helps you to identify the source of the issue by providing a text output with detailed information about the error.
Make sure to click **OK** on the error pop-up message and then inspect the text output in SES.

Missing executables
===================

On Windows and Linux, SES uses the PATH variable to find executables.
If you get an error that a tool or command cannot be found, first make sure that the tool is installed.
If it is installed, add its location to the PATH variable.

For some tools, you can explicitly specify the location under **Tools -> Options** (select the **nRF Connect** tab).

  .. _ses_options_figure:

  .. figure:: images/ses_options.png
     :alt: nRF Connect SDK options in SES

     nRF Connect SDK options in SES

Setup on macOS
==============

On macOS, the global PATH variable is used only if you start SES from the command line.
If you start SES by running the file :file:`bin/emStudio`, the global PATH is not used, and you must specify the path to all executables under **Tools -> Options** (select the **nRF Connect** tab, see :ref:`ses_options_figure`).

In addition, specify the path to the west tool as additional CMake option, replacing *path_to_west* with the path to the west executable (for example, ``/usr/local/bin/west``):

.. parsed-literal::
   :class: highlight

   -DWEST=\ *path_to_west*

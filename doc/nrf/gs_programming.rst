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

#. Select **File -> Open nRF Connect SDK Project.**

    .. figure:: images/ses_open.JPG
       :alt: Open nRF Connect SDK Project menu

       Open nRF Connect SDK Project menu

#. To import a project into SES, you must specify the following paths:

	- **Zephyr Base** - the location of your cloned Zephyr repository
	- **CMakeLists.txt** - the location of the :file:`CMakeLists.txt` project file
	  of the sample that you want to work with
	- **Build Directory** - the folder in which to run the build
	- **Board Directory** - the location of the board description of the board
	  for which to build the project
	- **GNU ARM Embedded Toolchain Directory** - the location of the GNU Arm
	  Embedded Toolchain installation

   The following figure shows an example configuration for the Asset Tracker
   sample built for the ``nrf9160_pca10090`` board target:

   .. figure:: images/ses_config.JPG
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
==================

When using SES to build the |NCS| samples,
it might return an error indicating a project load failure. For example::

	Can't load project file
	The project file <filepath> is invalid.
	The reported error is 'solution load command failed (1)'

This issue might be caused by a variety of problems, such as incorrectly specified
project file paths.
SES helps you to identify the source of the issue by providing a text output
with detailed information about the error. Make sure to click **OK** on the
error pop-up message and then inspect the text output in SES.

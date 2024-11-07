.. _ug_nrf54h20_gs:

Getting started with the nRF54H20 DK
####################################

.. contents::
   :local:
   :depth: 2

This document gets you started with your nRF54H20 Development Kit (DK) using the |NCS|.
It tells you how to install the :zephyr:code-sample:`sysbuild_hello_world` sample and perform a quick test of your DK.

.. _ug_nrf54h20_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF54H20 DK version PCA10175:

  * Engineering B - ranging from v0.8.0 to 0.8.2
  * Engineering C - v0.8.3 and later revisions

  Check the version number on your DK's sticker to verify its compatibility with the |NCS|.

* USB-C cable.

Software
========

On your computer, one of the following operating systems:

.. include:: ../../../../../nrf/installation/recommended_versions.rst
    :start-after: os_table_start
    :end-before: os_table_end

See :ref:`supported_OS` for more information.

|supported OS|

You also need the following:

* `Git`_ or `Git for Windows`_ (on Linux and Mac, or Windows, respectively).
* `curl`_.
* The latest version of the `nRF Command Line Tools`_ package.
  After downloading and installing the tools, add the nrfjprog executable to the system path, on Linux and MacOS, or to the environment variables, on Windows.
  This allows you to run it from anywhere on the system.

  .. note::
     Before running the initial J-Link installation from the `nRF Command Line Tools`_ package, ensure not to have any other J-Link executables on your system.
     If you have other J-Link installations, uninstall them before proceeding.

   .. note::
      |nrf_CLT_deprecation_note|

* On Windows, SEGGER USB Driver for J-Link from `SEGGER J-Link`_ |jlink_ver|.

   .. note::
      To install the SEGGER USB Driver for J-Link on Windows, you must manually reinstall J-Link |jlink_ver| from the command line using the ``-InstUSBDriver=1`` parameter, updating the installation previously run by the `nRF Command Line Tools`_:

      1. Navigate to the download location of the J-Link executable and run one of the following commands:

         * From the Command Prompt::

            JLink_Windows_V794i_x86_64.exe -InstUSBDriver=1

         * From PowerShell::

            .\JLink_Windows_V794i_x86_64.exe -InstUSBDriver=1

      #. In the :guilabel:`Choose optional components` window, select :guilabel:`update existing installation`.
      #. Add the J-Link executable to the system path on Linux and MacOS, or to the environment variables on Windows, to run it from anywhere on the system.

* The latest version of |VSC| for your operating system from the `Visual Studio Code download page`_.
* In |VSC|, the latest version of the `nRF Connect for VS Code Extension Pack`_.
* On Linux, the `nrf-udev`_ module with udev rules required to access USB ports on Nordic Semiconductor devices and program the firmware.

.. _ug_nrf54h20_gs_installing_software:

Installing the required software
********************************

To work with the nRF54H20 DK, follow the instructions in the next sections to install the required tools.

.. _ug_nrf54h20_install_toolchain:

Installing the |NCS| and its toolchain
**************************************

You can install the |NCS| and its toolchain by using Toolchain Manager.

Toolchain Manager is a tool available from `nRF Connect for Desktop`_, a cross-platform tool that provides different applications that simplify installing the |NCS|.
Both the tool and the application are available for Windows, Linux, and MacOS.

To install the toolchain and the SDK using the Toolchain Manager app, complete the following steps:

1. Install Toolchain Manager:

   a. `Download nRF Connect for Desktop`_ for your operating system.
   #. Install and run the tool on your machine.
   #. In the **APPS** section, click :guilabel:`Install` next to Toolchain Manager.

   The app is installed on your machine, and the :guilabel:`Install` button changes to :guilabel:`Open`.

#. Install the |NCS| source code:

   a. Open Toolchain Manager in nRF Connect for Desktop.

      .. figure:: ../../../../../nrf/installation/images/gs-assistant_tm.png
         :alt: The Toolchain Manager window

         The Toolchain Manager window

   #. Click :guilabel:`SETTINGS` in the navigation bar to specify where you want to install the |NCS|.
   #. In :guilabel:`SDK ENVIRONMENTS`, click the :guilabel:`Install` button next to the |NCS| version |release|.

      The |NCS| version |release| is installed on your machine.
      The :guilabel:`Install` button changes to :guilabel:`Open VS Code`.

#. Set up the preferred building method:

   .. tabs::

      .. tab:: nRF Connect for Visual Studio Code

         To build on the |nRFVSC|, complete the following steps:

         a. In Toolchain Manager, click the :guilabel:`Open VS Code` button.

            A notification appears with a list of missing extensions that you need to install, including those from the `nRF Connect for Visual Studio Code`_ extension pack.
         #. Click **Install missing extensions**, then close VS Code.
         #. Once the extensions are installed, click **Open VS Code** button again.

         You can then follow the instructions in :ref:`creating_vsc`.

      .. tab:: Command line

         To build on the command line, complete the following steps:

         1. With admin permissions enabled, download and install the `nRF Command Line Tools`_.
         #. Restart the Toolchain Manager application.
         #. Click the dropdown menu for the installed nRF Connect SDK version.

            .. figure:: ../../../../../nrf/installation/images/gs-assistant_tm_dropdown.png
               :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

               The Toolchain Manager dropdown menu options

         #. Select :guilabel:`Open command prompt`.

         You can then follow the instructions in :ref:`creating_cmd`.

Installing the Terminal application
***********************************

On your computer, install `nRF Connect for Desktop`_.
You must also install a terminal emulator, such as `nRF Connect Serial Terminal`_ (from the nRF Connect for Desktop application) or the nRF Terminal (part of the `nRF Connect for Visual Studio Code`_ extension).
Both of these terminal emulators start the required :ref:`toolchain environment <using_toolchain_environment>`.

Installing nRF Util and its commands
************************************

Using the nRF54H20 DK with the |NCS| version |release| requires the following:

* nRF Util version 7.11.1 or above
* nRF Util ``device`` version 2.7.2
* nrf-regtool version 7.0.0

1. Download the nrfutil executable file from the `nRF Util development tool`_ product page.
#. Add nRF Util to the system path on Linux and MacOS, or environment variables on Windows, to run it from anywhere on the system.
   On Linux and MacOS, use one of the following options:

   * Add nRF Util's directory to the system path.
   * Move the file to a directory in the system path.

#. On MacOS and Linux, give ``nrfutil`` execute permissions by typing ``chmod +x nrfutil`` in a terminal or using a file browser.
   This is typically a checkbox found under file properties.
#. On MacOS, to run the nrfutil executable you need to allow it in the system settings.
#. Verify the version of the nRF Util installation on your machine by running the following command::

      nrfutil --version

#. If your version is below 7.11.1, run the following command to update nRF Util::

      nrfutil self-upgrade

   For more information, consult the `nRF Util`_ documentation.

#. Install the nRF Util ``device`` command version 2.7.2 as follows::

      nrfutil install device=2.7.2 --force

#. Install the nrf-regtool version 7.0.0:

   a. Open nRF Connect for Desktop, navigate to the Toolchain Manager, select the |release| toolchain, and click the :guilabel:`Open terminal` button.
   #. In the terminal window, install ``nrf-regtool`` version 7.0.0 as follows::

         pip install nrf-regtool==7.0.0

.. _ug_nrf54h20_gs_bringup:

nRF54H20 DK bring-up
********************

The following sections describe the steps required for the nRF54H20 bring-up.

.. rst-class:: numbered-step

Programming the BICR
====================

The Board Information Configuration Registers (BICR) are non-volatile memory (NVM) registers that contain information on how the nRF54H20 SoC must interact with other board elements, including the information about the power and clock delivery to the SoC.
To prepare the nRF54H20 DK for first use, you must manually program the values of the BICR using a precompiled BICR binary file (:file:`bicr.hex`).

1. Download the `BICR new binary file`_.
#. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.

.. note::
   On MacOS, connecting the DK might cause a popup containing the message ``“Disk Not Ejected Properly`` to repeatedly appear on screen.
   To disable this, run ``JLinkExe``, then run ``MSDDisable`` in the J-Link Commander interface.

#. List all the connected development kits to see their serial number (matching the one on the DK's sticker)::

      nrfutil device list

#. Move the BICR HEX file to a folder of your choice, then program the BICR by running nRF Util from that folder using the following command::

      nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware bicr.hex --core Application --serial-number <serial_number>

.. rst-class:: numbered-step

.. _ug_nrf54h20_SoC_binaries:

Programming the nRF54H20 SoC binaries
=====================================

After programming the BICR, the nRF54H20 SoC requires the provisioning of the nRF54H20 SoC binaries, a bundle containing the precompiled firmware for the Secure Domain and System Controller.
To program the nRF54H20 SoC binaries to the nRF54H20 DK, do the following:

1. Download the nRF54H20 SoC binaries v0.7.0:

   * `nRF54H20 SoC Binaries v0.7.0 for EngC DKs`_, compatible with the nRF54H20 DK v0.8.3 and later revisions
   * `nRF54H20 SoC Binaries v0.7.0 for EngB DKs`_, compatible with the nRF54H20 DKs ranging from v0.8.0 to v0.8.2.

   .. note::
      On MacOS, ensure that the ZIP file is not unpacked automatically upon download.

#. Move the :file:`.zip` bundle to a folder of your choice, then run nRF Util to program the binaries using the following command::

      nrfutil device x-provision-nrf54h --firmware <path-to_bundle_zip_file> --serial-number <serial_number>

.. rst-class:: numbered-step

Transitioning the nRF54H20 SoC to RoT
=====================================

The current nRF54H20 DK is delivered with its lifecycle state (LCS) set to ``EMPTY``.
To correctly operate, its lifecycle state must be transitioned to Root of Trust (``RoT``).

.. note::
   The forward transition to LCS ``RoT`` is permanent.
   After the transition, it is not possible to transition backward to LCS ``EMPTY``.

To transition the LCS to ``RoT``, do the following:

1. Set the LCS of the nRF54H20 SoC to Root of Trust using the following command::

      nrfutil device x-adac-lcs-change --life-cycle rot --serial-number <serial_number>

#. After the LCS transition, reset the device::

      nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

.. _ug_nrf54h20_gs_sample:

Building and programming the sample
***********************************

The :zephyr:code-sample:`sysbuild_hello_world` sample is a multicore sample running on both the application core (``cpuapp``) and the Peripheral Processor (PPR, ``cpuppr``).
It uses the ``nrf54h20dk/nrf54h20/cpuapp`` board target.

To build and program the sample to the nRF54H20 DK, complete the following steps:

1. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.
#. Open nRF Connect for Desktop, navigate to the Toolchain Manager, select the version |release| toolchain, and click the :guilabel:`Open terminal` button.
#. In the terminal window, navigate to the :file:`zephyr/samples/sysbuild/hello_world` folder containing the sample.
#. Build the sample for application and radio cores by running the following commands:

   * For Engineering C (DKs 0.8.3 and later revisions)::

         west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.sysbuild.hello_world.nrf54h20dk_cpuapp_cpurad .

   * For Engineering B (DKs from v0.8.0 to 0.8.2)::

         west build -p -b nrf54h20dk@0.8.0/nrf54h20/cpuapp -T sample.sysbuild.hello_world.nrf54h20dk_cpuapp_cpurad .

You can now program the sample.
If you have multiple Nordic Semiconductor devices, make sure that only the nRF54H20 DK you want to program is connected.

.. code-block:: console

   west flash

The sample will be automatically built and programmed on both the application core and the Peripheral Processor (PPR) of the nRF54H20.

.. include:: /includes/nRF54H20_erase_UICR.txt

.. _ug_nrf54h20_sample_reading_logs:

Reading the logs
****************

With the :zephyr:code-sample:`sysbuild_hello_world` sample programmed, the nRF54H20 DK outputs logs for the application core and the configured remote processor.
The logs are output over UART.

To read the logs from the :zephyr:code-sample:`sysbuild_hello_world` sample programmed to the nRF54H20 DK, complete the following steps:

1. Connect to the DK with a terminal emulator (for example, `nRF Connect Serial Terminal`_) using the :ref:`default serial port connection settings <test_and_optimize>`.
#. Press the **Reset** button on the PCB to reset the DK.
#. Observe the console output for the application core:

   .. code-block:: console

      *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
      Hello world from nrf54h20dk/nrf54h20/cpuapp

.. note::
   If no output is shown when using nRF Serial Terminal, select a different serial port in the terminal application.

For more information on how logging works in the |NCS|, consult the :ref:`ug_logging` and :ref:`zephyr:logging_api` documentation pages.

Next steps
**********

You are now all set to use the nRF54H20 DK.
See the following links for where to go next:

* :ref:`ug_nrf54h20_architecture` for information about the multicore System on Chip, such as the responsibilities of the cores and their interprocessor interactions, the memory mapping, and the boot sequence.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
* :ref:`configuration_and_build` documentation to learn more about the |NCS| development environment.
* :ref:`ug_nrf54h` documentation for more advanced topics related to the nRF54H20.

If you want to go through an online training course to familiarize yourself with Bluetooth® Low Energy and the development of Bluetooth LE applications, enroll in the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.

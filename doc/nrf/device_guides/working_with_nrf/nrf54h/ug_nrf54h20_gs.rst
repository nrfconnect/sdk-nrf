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

* nRF54H20 DK version PCA10175 v0.7.2 (ES3) or PCA10175 v0.8.0 (ES3.3, ES4).
  These are the only versions of the nRF54H20 DK compatible with the |NCS| v2.7.0.
  Check the version number on your DK's sticker to verify its compatibility with the |NCS| version v2.7.0.
* USB-C cable.

Software
========

On your computer, one of the following operating systems:

.. include:: ../../../../nrf/installation/recommended_versions.rst
    :start-after: os_table_start
    :end-before: os_table_end

See :ref:`supported_OS` for more information about the tier definitions.

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

* On Windows, SEGGER USB Driver for J-Link from SEGGER `J-Link version 7.94e`_.

   .. note::
      To install the SEGGER USB Driver for J-Link on Windows, you must manually reinstall J-Link v7.94e from the command line using the ``-InstUSBDriver=1`` parameter, updating the installation previously run by the `nRF Command Line Tools`_:

      1. Navigate to the download location of the J-Link executable and run one of the following commands:

         * From the Command Prompt::

            JLink_Windows_V794e_x86_64.exe -InstUSBDriver=1

         * From PowerShell::

            .\JLink_Windows_V794e_x86_64.exe -InstUSBDriver=1

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

Installing the |NCS| v2.7.0 and its toolchain
*********************************************

You can install the |NCS| v2.7.0 and its toolchain by using Toolchain Manager.

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

      .. figure:: ../../../../nrf/installation/images/gs-assistant_tm.png
         :alt: The Toolchain Manager window

         The Toolchain Manager window

   #. Click :guilabel:`SETTINGS` in the navigation bar to specify where you want to install the |NCS|.
   #. In :guilabel:`SDK ENVIRONMENTS`, click the :guilabel:`Install` button next to the |NCS| version 2.7.0.

      The |NCS| version 2.7.0 is installed on your machine.
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

            .. figure:: ../../../../nrf/installation/images/gs-assistant_tm_dropdown.png
               :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

               The Toolchain Manager dropdown menu options

         #. Select :guilabel:`Open command prompt`.

         You can then follow the instructions in :ref:`creating_cmd`.

Installing the Terminal application
***********************************

On your computer, install `nRF Connect for Desktop`_.
You must also install a terminal emulator, such as `nRF Connect Serial Terminal`_ (from the nRF Connect for Desktop application) or the nRF Terminal (part of the `nRF Connect for Visual Studio Code`_ extension).

Installing nRF Util and its commands
************************************

Using the nRF54H20 DK with the |NCS| v2.7.0 requires the following:

* nRF Util version 7.11.1 or above
* nRF Util ``device`` version 2.4.0

1. Download the nrfutil executable file from the `nRF Util development tool`_ product page.
#. Add nRF Util to the system path on Linux and MacOS, or environment variables on Windows, to run it from anywhere on the system.
   On Linux and MacOS, use one of the following options:

   * Add nRF Util's directory to the system path.
   * Move the file to a directory in the system path.

#. On MacOS and Linux, give ``nrfutil`` execute permissions by typing ``chmod +x nrfutil`` in a terminal or using a file browser.
   This is typically a checkbox found under file properties.
#. On MacOS, To run the nrfutil executable you need to allow it in the system settings.
#. Verify the version of the nRF Util installation on your machine by running the following command::

      nrfutil --version

#. If your version is below 7.11.1, run the following command to update nRF Util::

      nrfutil self-upgrade

#. Install the nRF Util ``device`` command to version 2.4.0 as follows::

      nrfutil install device=2.4.0 --force

For more information, consult the `nRF Util`_ documentation.

.. _ug_nrf54h20_gs_bringup:

nRF54H20 DK bring-up
********************

The following sections describe the steps required for the nRF54H20 bring-up.

.. rst-class:: numbered-step

Programming the BICR
====================

The Board Information Configuration Registers (BICR) are non-volatile memory (NVM) registers that contain information on how the nRF54H20 SoC must interact with other board elements, including the information about the power and clock delivery to the SoC.
To prepare the nRF54H20 DK for first use, you must manually program the values of the BICR using a precompiled BICR binary file (:file:`bicr_ext_loadcap.hex`).

1. Download the `BICR binary file`_ .
#. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.

.. note::
   On MacOS, connecting the DK might cause a popup containing the message ``“Disk Not Ejected Properly`` to repeatedly appear on screen.
   To disable this, run ``JLinkExe``, then run ``MSDDisable`` in the J-Link Commander interface.

#. List all the connected development kits to see their serial number (matching the one on the DK's sticker)::

      nrfutil device list

#. Move the BICR HEX file to a folder of your choice, then program the BICR by running nRF Util from that folder using the following command::

      nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware bicr_ext_loadcap.hex --core Secure --serial-number <serial_number>

.. rst-class:: numbered-step

Programming the SDFW and SCFW
=============================

After programming the BICR, the nRF54H20 SoC requires the provisioning of a bundle ( :file:`nrf54h20_soc_binaries_v0.5.0.zip`) containing the precompiled firmware for the Secure Domain and System Controller.
To program the Secure Domain Firmware (SDFW, also known as ``urot``) and the System Controller Firmware (SCFW) from the firmware bundle to the nRF54H20 DK, do the following:

1. Download the `nRF54H20 firmware bundle v0.5.0`_.

   .. note::
      On MacOS, ensure that the ZIP file is not unpacked automatically upon download.

#. Move the :file:`.zip` bundle to a folder of your choice, then run nRF Util to program the binaries using the following command::

      nrfutil device x-provision-nrf54h --firmware <path-to_bundle_zip_file> --serial-number <serial_number>

.. rst-class:: numbered-step

Updating the FICR
=================

.. caution::
   This step is required only if your nRF54H20 DK is version PCA10175 v0.7.2 or v0.8.0 ES3.3.
   Jump to the next step if your DK is version ES4, meaning v0.8.0 with no ES markings.

After programming the SDFW and SCFW from the firmware bundle, you must update the Factory Information Configuration Registers (FICR) to correctly configure some trims of the nRF54H20 SoC.
To update the FICR, you must run a J-Link script:

1. Get the Jlink script that updates the FICR::

      curl -LO https://files.nordicsemi.com/artifactory/swtools/external/scripts/nrf54h20es_trim_adjust.jlink

#. Run the script:

   * Linux and Mac OS::

        JLinkExe -CommanderScript nrf54h20es_trim_adjust.jlink

   * Windows::

        jlink.exe -CommanderScript nrf54h20es_trim_adjust.jlink

.. rst-class:: numbered-step

Transitioning the nRF54H20 SoC to RoT
=====================================

The current nRF54H20 DK is delivered with its lifecycle state (LCS) set to ``EMPTY``.
To correctly operate, its lifecycle state must be transitioned to Root of Trust (``RoT``).

.. note::
   The forward transition to LCS ``RoT`` is permanent.
   After the transition, it is not possible to transition backward to LCS ``EMPTY``.

To transition the LCS to ``RoT``, do the following:

1. Verify the current lifecycle state of the nRF54H20::

      nrfutil device x-adac-discovery --serial-number <serial_number>

   The output will look similar to the following::

      *serial_number*
      adac_auth_version     1.0
      vendor_id             Nordic VLSI ASA
      soc_class             0x00005420
      soc_id                [e6, 6f, 21, b6, dc, be, 11, ee, e5, 03, 6f, fe, 4d, 7b, 2e, 07]
      hw_permissions_fixed  [00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
      hw_permissions_mask   [01, 00, 00, 00, 87, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
      psa_lifecycle         LIFECYCLE_EMPTY (0x1000)
      sda_id                0x01
      secrom_revision       0xad3b3cd0
      sysrom_revision       0xebc8f190
      token_formats         [TokenAdac]
      cert_formats          [CertAdac]
      cryptosystems         [Ed25519Sha512]
      Additional TLVs:
      TargetIdentity: [ff, ff, ff, ff, ff, ff, ff, ff]

#. If the lifecycle state (``psa_lifecycle``) shown is ``RoT`` (``LIFECYCLE_ROT (0x2000)``), no LCS transition is required.
   If the lifecycle state (``psa_lifecycle``) shown is not ``RoT`` (``LIFECYCLE_EMPTY (0x1000)`` means the LCS is set to ``EMPTY``), set it to Root of Trust using the following command::

      nrfutil device x-adac-lcs-change --life-cycle rot --serial-number <serial_number>

#. Verify again the current lifecycle state of the nRF54H20::

      nrfutil device x-adac-discovery --serial-number <serial_number>

   The output will look similar to the following::

      *serial_number*
      adac_auth_version     1.0
      vendor_id             Nordic VLSI ASA
      soc_class             0x00005420
      soc_id                [e6, 6f, 21, b6, dc, be, 11, ee, e5, 03, 6f, fe, 4d, 7b, 2e, 07]
      hw_permissions_fixed  [00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
      hw_permissions_mask   [01, 00, 00, 00, 87, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
      psa_lifecycle         LIFECYCLE_ROT (0x2000)
      sda_id                0x01
      secrom_revision       0xad3b3cd0
      sysrom_revision       0xebc8f190
      token_formats         [TokenAdac]
      cert_formats          [CertAdac]
      cryptosystems         [Ed25519Sha512]
      Additional TLVs:
      TargetIdentity: [ff, ff, ff, ff, ff, ff, ff, ff]

   The lifecycle state (``psa_lifecycle``) is now correctly set to *Root of Trust* (``LIFECYCLE_ROT (0x2000)``)

#. After the LCS transition, reset the device::

      nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

.. _ug_nrf54h20_gs_sample:

Programming the sample
**********************

The :zephyr:code-sample:`sysbuild_hello_world` sample is a multicore sample running on both the application core (``cpuapp``) and the Peripheral Processor (PPR, ``cpuppr``).
It uses the ``nrf54h20dk/nrf54h20/cpuapp`` board target.

To build and program the sample to the nRF54H20 DK, complete the following steps:

1. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.
#. Open nRF Connect for Desktop, navigate to the Toolchain Manager, select the v2.7.0 toolchain, and click the :guilabel:`Open terminal` button.
#. In the terminal window, navigate to the :file:`zephyr/samples/sysbuild/hello_world` folder containing the sample.
#. Build the sample for application and radio cores by running the following command::

      west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.sysbuild.hello_world.nrf54h20dk_cpuapp_cpurad .

#. Program the sample.
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

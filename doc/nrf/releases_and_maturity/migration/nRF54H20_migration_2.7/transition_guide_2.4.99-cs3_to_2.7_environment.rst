.. _transition_cs3_to_2_7_env:

Transition your development environment to |NCS| v2.7.0 (for v2.4.99-cs3 users)
###############################################################################

.. contents::
   :local:
   :depth: 2

This document describes how to transition your development environment for the nRF54H20 DK from |NCS| v2.4.99-cs3 to |NCS| v2.7.0.

The main development environment changes introduced by v2.7.0 for the nRF54H20 DK are the following:

* The |NCS| toolchain has been updated.
* nRF Util is now used by the build system.
* The Secure Domain Firmware (SDFW) and System Controller Firmware (SCFW) are no longer built from the source during the application build process.
  They are provided as a firmware bundle (v0.5.0) and provisioned to the nRF54H20 during the bring-up steps.
  The nRF54H20 DK must be in lifecycle state ``EMPTY`` to be provisioned with the new firmware bundle.
  For additional details, see :ref:`transition_cs3_to_2_7_env_bringup`.
  See the details in the `nRF54H20 DK bring-up`_ section below.
* To correctly operate the nRF54H20 DK, its lifecycle state must be set to ``RoT`` after the bring-up.

  Also, it is no longer possible to perform an unauthenticated backward LCS transitions.
  This means that once set to ``RoT``, it is no longer possible to revert to LCS state ``EMPTY``.

Minimum requirements
********************

Make sure you have all the required hardware, software, and that your computer has one of the supported operating systems.

Hardware
========

* nRF54H20 DK version PCA10175 v0.7.2 (ES3) or PCA10175 v0.8.0 (ES3.3, ES4).
  These are the only versions of the nRF54H20 DK compatible with |NCS| v2.7.0.
  Check the version number on your DK's sticker to verify its compatibility with |NCS| version v2.7.0.
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

* `Git`_ or `Git for Windows`_ (on Linux and Mac, or Windows, respectively)
* `curl`_
* The latest version of the `nRF Command Line Tools`_ package.
  After downloading and installing the tools, add the nrfjprog executable to the system path, on Linux and MacOS, or to the environment variables, on Windows, to run it from anywhere on the system.

  The CLI tools installation will also trigger the installation of J-Link.
  During that installation, in the :guilabel:`Choose optional components` window, select :guilabel:`update existing installation`.
* On Windows, SEGGER USB Driver for J-Link from SEGGER `J-Link version 7.94e`_.

   .. note::
      To install the SEGGER USB Driver for J-Link on Windows, you must manually reinstall J-Link v7.94e from the command line using the ``-InstUSBDriver=1`` parameter, updating the installation previously run by the `nRF Command Line Tools`_:

      1. Navigate to the download location of the J-Link executable and run one of the following commands:

         * From the Command Prompt::

            JLink_Windows_V794e_x86_64.exe -InstUSBDriver=1

         * From PowerShell::

            .\JLink_Windows_V794e_x86_64.exe -InstUSBDriver=1

      #. In the :guilabel:`Choose optional components` window, select again :guilabel:`update existing installation`.
      #. Add the J-Link executable to the system path, on Linux and MacOS, or to the environment variables, on Windows, to run it from anywhere on the system.

* The latest version of |VSC| for your operating system from the `Visual Studio Code download page`_.
* In |VSC|, the latest version of the `nRF Connect for VS Code Extension Pack`_.
* On Linux, the `nrf-udev`_ module with udev rules required to access USB ports on Nordic Semiconductor devices and program the firmware.

Preliminary step
****************

Before transitioning to the new toolchain, rename your existing ``ncs-lcs`` and ``.west`` folders (for example as ``ncs-lcs_old`` and ``.west_old``) to back up their files.

.. note::
   If you have multiple folders initialized with ``west``, it is suggested to back up all the relevant ``.west`` folders.

Transitioning to the |NCS| v2.7.0
*********************************

You can transition to the |NCS| v2.7.0 by using Toolchain Manager.

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

#. If you have any existing custom applications created for 2.4.99-cs3 that you would like to migrate, move its files from the previous ``ncs-lcs_old`` folder to the newly created |NCS| installation folder.

Updating the Terminal application
*********************************

To update `Serial Terminal from nRF Connect for Desktop`, follow these steps:

1. On your computer, open `nRF Connect for Desktop`_
   If there is an update available, a pop up will notify you of its availability.
#. If available, install the update from the pop up screen.
#. Update `Serial Terminal from nRF Connect for Desktop`.

If you are using the nRF Terminal application part of the `nRF Connect for Visual Studio Code`_ extension, open Visual Studio Code instead and ensure you are running the newest version of both the editor and the extension.

Installing nRF Util and its commands
************************************

Using the nRF54H20 DK with the |NCS| v2.7.0 requires the following:

* nRF Util version 7.11.1 or above
* nRF Util ``device`` version 2.4.0

1. Download the nrfutil executable file from the `nRF Util development tool`_ product page.
#. Add nRF Util to the system path (on Linux and MacOS) or environment variables (on Windows) to run it from anywhere on the system.
   On Linux and MacOS, use one of the following options:

   * Add nRF Util's directory to the system path.
   * Move the file to a directory in the system path.

#. On MacOS and Linux, give ``nrfutil`` execute permissions by typing ``chmod +x nrfutil`` in a terminal or using a file browser.
   This is typically a checkbox found under file properties.
#. Verify the version of the nRF Util installation on your machine by running the following command::

      nrfutil --version

#. If your version is below 7.11.1, run the following command to update nRF Util::

      nrfutil self-upgrade

#. Install the nRF Util ``device`` command to version 2.4.0 as follows::

      nrfutil install device=2.4.0 --force

For more information, consult the `nRF Util`_ documentation.

.. _transition_cs3_to_2_7_env_bringup:

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

.. _transition_cs3_to_2_7_env_lcsrot:

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

Next steps
**********

Your environment is now set to use the |NCS| v2.7.0 with the nRF54H20 DK:

* If you want to modify your existing custom applications previously developed for |NCS| v2.4.99-cs3 to be compatible with v2.7.0, consult the :ref:`migration_cs3_to_to_2_7_app` page.
* If you want to build and program a sample application on your nRF54H20 DK, consult the building and programming section in the :ref:`ug_nrf54h20_gs` page.

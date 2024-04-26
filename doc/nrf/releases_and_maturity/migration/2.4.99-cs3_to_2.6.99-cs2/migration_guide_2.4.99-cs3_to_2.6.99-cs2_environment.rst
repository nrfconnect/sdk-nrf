.. _migration_cs3_to_2_6_99_cs2_env:

Update your development environment for |NCS| v2.6.99_cs2 (for v2.4.99-cs3 users)
#################################################################################

.. contents::
   :local:
   :depth: 2

This document describes how to update your development environment from |NCS| v2.4.99-cs3 to |NCS| v2.6.99-cs2.

The main development environment changes introduced by 2.6.99-cs2 for the nRF54H20 DK are the following:

* The |NCS| toolchain has been updated.
  A bootstrap script specific to the nRF54H20 DK is now available to install and update the toolchain.
* nRF Util has now replaced nRF Command Line Tools.
  The bootstrap script will now install and update nRF Util.
* SDFW and SCFW are now provided as precompiled binaries.
  The Secure Domain Firmware (SDFW) and System Controller Firmware (SCFW) are no longer built from the source during the application build process, but they must be provisioned as binaries from the provided firmware bundle before the DK can be used.
  See the details in the `nRF54H20 DK bring-up`_ section below.
* The nRF54H20 SoC lifecycle state must now be set to Root of Trust (RoT).
  See the details in the `Transitioning the nRF54H20 SoC to RoT`_ section.

Minimum requirements
********************

Make sure you have all the required hardware, software, and that your computer has one of the supported operating systems.

Hardware
========

* nRF54H20 DK version PCA10175 v0.7.x or v0.8.0 (ES3).
  These are the only versions of the nRF54H20 DK compatible with |NCS| 2.6.99-cs2.
  Check the version number on your DK's sticker to verify its compatibility with |NCS| version 2.6.99-cs2.
* USB-C cable.

Software
========

On your computer, one of the following operating systems:

* Microsoft Windows
* Ubuntu Linux
* macOS

|supported OS|

You also need the following:

* `Git`_ or `Git for Windows`_ (on Linux and Mac, or Windows, respectively)
* `curl`_
* On Windows, SEGGER USB Driver for J-Link
* SEGGER `J-Link version 7.94e`_

.. note::
   The SEGGER USB Driver for J-Link was included, on Windows, in the nRF Command Line Tools bundle required by the |NCS| v2.4.99-cs3.
   When updating SEGGER J-Link to version 7.94e on Windows, select :guilabel:`Update existing installation` under :guilabel:`Choose destination` in the :guilabel:`choose optional components` window.

   See the following screenshot:

      .. figure:: images/jlink794e_install.png
         :alt: Optional components in the J-Link installation

Preliminary step
****************

Before updating the toolchain, rename your existing ``ncs-lcs`` and ``.west`` folders (for example as ``ncs-lcs_old`` and ``.west_old``) to backup their files.

Updating the toolchain
**********************

You can update the toolchain for the |NCS| v2.6.99-cs2 by running an installation script.
Follow these steps:

.. tabs::

   .. tab:: Windows

      1. Open Git Bash.
      #. Download and run the :file:`bootstrap-toolchain.sh` installation script file using the following command:

         .. parsed-literal::
            :class: highlight

            curl --proto '=https' --tlsv1.2 -sSf https://files.nordicsemi.com/artifactory/swtools/external/scripts/bootstrap-toolchain.sh | NCS_TOOLCHAIN_VERSION=v2.6.99-cs2 sh

         Depending on your connection, this might take some time.
      #. Open a new terminal window with the |NCS| toolchain environment by running the following command:

         .. parsed-literal::
            :class: highlight

            c:/ncs-lcs/nrfutil.exe toolchain-manager launch --terminal --chdir "c:/ncs-lcs/work-dir" --ncs-version v2.6.99-cs2

         This setup allows you to access west and other development tools.
         Alternatively, you can set up the environment variables manually by running the following command::

            c:/ncs-lcs/nrfutil.exe toolchain-manager env --as-script

         Copy-paste the output into the terminal and execute it to enable the use of west directly in that window.

         .. note::
            When working with west in the customer sampling release, you must always use a terminal window with the |NCS| toolchain environment.

      If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`C:\\ncs-lcs` directory, and start over.

   .. tab:: Linux

      1. Open a terminal window.
      #. Download and run the :file:`bootstrap-toolchain.sh` installation script file using the following command:

         .. parsed-literal::
            :class: highlight

            curl --proto '=https' --tlsv1.2 -sSf https://files.nordicsemi.com/artifactory/swtools/external/scripts/bootstrap-toolchain.sh | NCS_TOOLCHAIN_VERSION=v2.6.99-cs2 sh

         Depending on your connection, this might take some time.
      #. Open a new terminal window with the |NCS| toolchain environment by running the following command:

         .. parsed-literal::
            :class: highlight

            $HOME/ncs-lcs/nrfutil toolchain-manager launch --shell --chdir "$HOME/ncs-lcs/work-dir" --ncs-version v2.6.99-cs2

         .. note::
            When working with west in the customer sampling release, you must always use a shell window with the |NCS| toolchain environment.

      If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`ncs-lcs` directory, and start over.

   .. tab:: macOS

      1. Open a terminal window.
      #. Install `Homebrew`_:

         .. code-block:: bash

            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

      #. Use the ``brew`` command to install the required dependencies:

         .. code-block:: bash

            brew install cmake ninja gperf python3 ccache qemu dtc wget libmagic

         Ensure that these dependencies are installed with their versions as specified in the :ref:`Required tools table <req_tools_table>`.
         To check the installed versions, run the following command:

         .. parsed-literal::
            :class: highlight

             brew list --versions

      #. Download and run the :file:`bootstrap-toolchain.sh` installation script file using the following command:

         .. parsed-literal::
            :class: highlight

            curl --proto '=https' --tlsv1.2 -sSf https://files.nordicsemi.com/artifactory/swtools/external/scripts/bootstrap-toolchain.sh | NCS_TOOLCHAIN_VERSION=v2.6.99-cs2 sh

         Depending on your connection, this might take some time.

         .. note::
            On macOS, the install directory is :file:`/opt/nordic/ncs`.
            This means that creating the directory requires root access.
            You will be prompted to grant the script admin rights for the creation of the folder on the first install.
            The folder will be created with the necessary access rights to the user, so subsequent installs do not require root access.

            Do not run the toolchain-manager installation as root (for example, using `sudo`).
            It restricts access to the root user only, meaning you will need the root access for any subsequent installations.
            If you run the script as root, to fix permissions delete the installation folder and run the script again as a non-root user.

      #. Open a new terminal window with the |NCS| toolchain environment by running the following command:

         .. parsed-literal::
            :class: highlight

            /Users/*yourusername*/ncs-lcs/nrfutil toolchain-manager launch --shell --chdir "/Users/*yourusername*/ncs-lcs/work-dir" --ncs-version v2.6.99-cs2

         .. note::
            When working with west in the customer sampling release, you must always use a shell window with the |NCS| toolchain environment.

      #. Run the following commands in your terminal to install the correct lxml dependency:

         .. parsed-literal::
            :class: highlight

            pip uninstall -y lxml
            pip install lxml

      If you run into errors during the installation process, delete the :file:`.west` folder inside the :file:`ncs-lcs` directory, and start over.

We recommend adding the nRF Util path to your environmental variables.

Updating to the |NCS| 2.6.99-cs2
********************************

After you have updated the toolchain, complete the following steps to get the |NCS| v2.6.99-cs2:

1. In the terminal window opened during the toolchain installation, initialize west with the revision of the |NCS| from the customer sampling:

   .. parsed-literal::
      :class: highlight

      west init -m https://github.com/nrfconnect/sdk-nrf --mr v2.6.99-cs2

#. Enter the following command to clone the project repositories::

      west update

   Depending on your connection, this might take some time.

#. Export a :ref:`Zephyr CMake package <zephyr:cmake_pkg>`.
   This allows CMake to automatically load the boilerplate code required for building |NCS| applications::

      west zephyr-export

   Your directory structure now looks similar to this::

      ncs-lcs/work-dir
      |___ .west
      |___ bootloader
      |___ modules
      |___ nrf
      |___ nrfxlib
      |___ zephyr
      |___ ...

   Note that there are additional folders, and that the structure might change.
   The full set of repositories and folders is defined in the manifest file.

#. if you have any existing custom applications created for 2.4.99-cs3 that you would like to migrate, move its files from the previous ``ncs-lcs_old`` folder to the newly created ``ncs-lcs`` folder.

Updating the Terminal application
*********************************

To update `Serial Terminal from nRF Connect for Desktop`, follow these steps:

1. On your computer, open `nRF Connect for Desktop`_
   If there is an update available, a pop up will notify you of its availability.
#. If available, install the update from the pop up screen.
#. Update `Serial Terminal from nRF Connect for Desktop`.

If you are using the nRF Terminal application part of the `nRF Connect for Visual Studio Code`_ extension, open Visual Studio Code instead and ensure you are running the newest version of both the editor and the extension.

.. _migration_cs3_to_2_6_99_cs2_env_bringup:

nRF54H20 DK bring-up
********************

The following sections describe the steps required for the nRF54H20 bring-up.

.. rst-class:: numbered-step

Programming the BICR
====================

The Board Information Configuration Registers (BICR) are non-volatile memory (NVM) registers that contain information on how the nRF54H20 SoC must interact with other board elements, including the information about the power and clock delivery to the SoC.
To prepare the nRF54H20 DK for first use, you must manually program the values of the BICR using a precompiled BICR binary file (:file:`bicr_ext_loadcap.hex`).

1. Download the `BICR binary file`_ .
#. List all the connected development kits to see their serial number (matching the one on the DK's sticker)::

      nrfutil device list

#. Move the BICR HEX file to a folder of your choice, then program the BICR by running nRF Util from that folder using the following command::

      nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware bicr_ext_loadcap.hex --core Secure --serial-number <serial_number>

.. rst-class:: numbered-step

Programming the SDFW and SCFW
=============================

After programming the BICR, the nRF54H20 SoC requires the provisioning of a bundle ( :file:`nrf54h20_soc_binaries_v0.3.3.zip`) containing the precompiled firmware for the Secure Domain and System Controller.
To program the Secure Domain Firmware (SDFW, also known as ``urot``) and the System Controller Firmware (SCFW) from the firmware bundle to the nRF54H20 DK, do the following:

1. Download the `nRF54H20 firmware bundle`_.
#. Move the :file:`ZIP` bundle to a folder of your choice, then run nRF Util to program the binaries using the following command::

      nrfutil device x-provision-nrf54h --firmware <path-to_bundle_zip_file> --serial-number <serial_number>

.. rst-class:: numbered-step

Updating the FICR
=================

After programming the SDFW and SCFW from the firmware bundle, you must update the Factory Information Configuration Registers (FICR) to correctly configure some trims of the nRF54H20 SoC.
To update the FICR, you must run a J-Link script:

1. Get the Jlink script that updates the FICR::

      curl -LO https://files.nordicsemi.com/artifactory/swtools/external/scripts/nrf54h20es_trim_adjust.jlink

#. Run the script::

      JLinkExe -CommanderScript nrf54h20es_trim_adjust.jlink

.. _migration_cs3_to_2_6_99_cs2_env_lcsrot:

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

Your environment is now set to use the |NCS| v2.6.99-cs2 with the nRF54H20 DK:

* If you want to modify your existing custom applications previously developed for |NCS| v2.4.99-cs3 to be compatible with v2.6.99-cs2, consult :ref:`migration_cs3_to_2_6_99_cs2_app`.
* If you want to build and program a sample application on your nRF54H20 DK, consult the building and programming section in the `nRF54H20 DK getting started guide for the nRF Connect SDK v2.6.99-cs2`_.

.. _ug_nrf54h20_gs:

Getting started with the nRF54H20 DK
####################################

.. contents::
   :local:
   :depth: 2

This section gets you started with your nRF54H20 Development Kit (DK) using the |NCS|.
It tells you how to install the :ref:`multicore_hello_world` sample and perform a quick test of your DK.

If you have already set up your nRF54H20 DK and want to learn more, see the following documentation:

* :ref:`configuration_and_build` documentation to learn more about the |NCS| development environment.
* :ref:`ug_nrf54h` documentation for more advanced topics related to the nRF54H20.

If you want to go through an online training course to familiarize yourself with Bluetooth® Low Energy and the development of Bluetooth LE applications, enroll in the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.

.. _ug_nrf54h20_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF54H20 DK version PCA10175 v0.7.x or v0.8.0 (ES3).
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
* SEGGER `J-Link version 7.94e`_

.. _ug_nrf54h20_gs_installing_software:

Installing the required software
********************************

To work with the nRF54H20 DK, follow the instructions in the next sections to install the required tools.

.. _ug_nrf54h20_install_toolchain:

Installing the toolchain
========================

You can install the toolchain for the customer sampling of the |NCS| by running an installation script.
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
            WWhen working with west in the customer sampling release, you must always use a terminal window with the |NCS| toolchain environment.

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

.. _ug_nrf54h20_install_ncs:

Installing the |NCS|
====================

After you have installed the toolchain, complete the following steps to get the customer sampling version of the |NCS|:

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

Installing the Terminal application
***********************************

On your computer, install `nRF Connect for Desktop`_.
You must also install a terminal emulator, such as `nRF Connect Serial Terminal`_ (from the nRF Connect for Desktop application) or the nRF Terminal (part of the `nRF Connect for Visual Studio Code`_ extension).

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

After programming the SDFW and SCFW from the firmware bundle, you must update the FICR registers to configure correctly some trims of the nRF54H20 SoC.
To update the FICR, run the following commands::

   curl -LO https://files.nordicsemi.com/artifactory/swtools/external/scripts/nrf54h20es_trim_adjust.jlink
   JLinkExe -CommanderScript nrf54h20es_trim_adjust.jlink

These commands will download and run a J-Link script that will correctly configure the trims.

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

The :ref:`multicore_hello_world` sample is a multicore sample running on both the Application Core (``cpuapp``) and the Peripheral Processor (PPR, ``cpuppr``).
It uses the ``nrf54h20dk_nrf54h20_cpuapp`` build target.

To build and program the sample to the nRF54H20 DK, complete the following steps:

1. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.
#. Navigate to the :file:`nrf/samples/multicore/hello_world` folder containing the sample.
#. Build the sample for application and radio cores by running the following command::

      west build -p -b nrf54h20dk_nrf54h20_cpuapp -T sample.multicore.hello_world.nrf54h20dk_cpuapp_cpurad .

#. Alternatively, build the sample for the application and PPR cores by running the following command::

      west build -p -b nrf54h20dk_nrf54h20_cpuapp -T sample.multicore.hello_world.nrf54h20dk_cpuapp_cpuppr .

#. Program the sample.
   If you have multiple Nordic Semiconductor devices, make sure that only the nRF54H20 DK you want to program is connected.

   .. code-block:: console

      west flash

The sample will be automatically built and programmed on both the Application Core and the Peripheral Processor (PPR) of the nRF54H20.

.. include:: /includes/nRF54H20_erase_UICR.txt

.. _ug_nrf54h20_sample_reading_logs:

Reading the logs
****************

With the :ref:`multicore_hello_world` sample programmed, the nRF54H20 DK outputs logs for the Application Core and the configured remote processor.
The logs are output over UART.

To read the logs from the :ref:`multicore_hello_world` sample programmed to the nRF54H20 DK, complete the following steps:

1. Connect to the DK with a terminal emulator (for example, `nRF Connect Serial Terminal`_) using the :ref:`default serial port connection settings <test_and_optimize>`.
#. Press the **Reset** button on the PCB to reset the DK.
#. Observe the console output for both cores:

   * For the Application Core, the output should look like the following:

     .. code-block:: console

        *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
        Hello world from nrf54h20dk_nrf54h20_cpuapp
        Hello world from nrf54h20dk_nrf54h20_cpuapp
        ...

   * For the remote core, like PPR, the output should be as follows:

     .. code-block:: console

        *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
        Hello world from nrf54h20dk_nrf54h20_cpuppr
        Hello world from nrf54h20dk_nrf54h20_cpuppr
        ...

.. note::
   If no output is shown when using nRF Serial Terminal, select a different serial port in the terminal application.

For more information on how logging works in the |NCS|, consult the :ref:`ug_logging` and :ref:`zephyr:logging_api` documentation pages.

Next steps
**********

You are now all set to use the nRF54H20 DK.
See the following links for where to go next:

* :ref:`ug_nrf54h20_architecture` for information about the multicore System on Chip, such as the responsibilities of the cores and their interprocessor interactions, the memory mapping, and the boot sequence.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.

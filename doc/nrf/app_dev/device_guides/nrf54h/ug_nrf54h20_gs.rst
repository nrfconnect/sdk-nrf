.. _ug_nrf54h20_gs:

Getting started with the nRF54H20 DK
####################################

.. contents::
   :local:
   :depth: 2

This document gets you started with your nRF54H20 Development Kit (DK) using the |NCS| for the first time.
It tells you how to install the :zephyr:code-sample:`sysbuild_hello_world` sample and perform a quick test of your DK.

If you are migrating from an earlier version of the |NCS|, see :ref:`migration_guides`.

.. _ug_nrf54h20_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* nRF54H20 DK version PCA10175 Engineering C - v0.9.0 and later revisions.
  Check the version number on your DK's sticker to verify its compatibility with the |NCS|.
* USB-C cable.

Software
========

On your computer, one of the following operating systems:

.. include:: ../../../installation/recommended_versions.rst
    :start-after: os_table_start
    :end-before: os_table_end

See :ref:`supported_OS` for more information.

|supported OS|

You also need the following:

* `Git`_ or `Git for Windows`_ (on Linux and Mac, or Windows, respectively)
* `curl`_
* SEGGER J-Link |jlink_ver| and, on Windows, also the SEGGER USB Driver for J-Link from `SEGGER J-Link`_ |jlink_ver|.
  For information on how to install the USB Driver, see the `nRF Util prerequisites`_ documentation.
* The latest version of |VSC| for your operating system from the `Visual Studio Code download page`_
* In |VSC|, the latest version of the `nRF Connect for VS Code Extension Pack`_
* On Linux, the `nrf-udev`_ module with udev rules required to access USB ports on Nordic Semiconductor devices and program the firmware

.. _ug_nrf54h20_gs_installing_software:

Installing the required software
********************************

To work with the nRF54H20 DK, follow the instructions in the next sections to install the required tools.

Install prerequisites
=====================

.. include:: ../../../installation/install_ncs.rst
   :start-after: .. prerequisites-include-start
   :end-before: .. prerequisites-include-end

Installing nRF Util and its commands
------------------------------------

Using the nRF54H20 DK with the |NCS| version |release| requires the following:

* nRF Util v7.13.0 or higher
* nRF Util ``device`` command v2.8.8
* nRF Util ``trace`` command v3.3.0
* nRF Util ``suit`` command v0.9.0

If you have not already installed nRF Util as part of :ref:`nRF Connect SDK prerequisites <installing_vsc>`, complete the following steps to install it:

1. Complete the steps listed on the `Installing nRF Util`_ page to install nRF Util.
   Follow the default installation procedure from the web repository.
#. Verify the version of the nRF Util installation on your machine:

   a. Run the following command:

      .. code-block::

         nrfutil --version

   b. If your version is below v7.13.0, run the following command to update the core module:

      .. code-block::

         nrfutil self-upgrade

      For more information, consult the `Upgrading nRF Util core module`_ documentation.

#. Install the required versions of nRF Util commands, as listed above, using the command from `Installing specific versions of nRF Util commands`_.
   For example, the following command installs the nRF Util ``device`` command version 2.7.16:

   .. code-block::

      nrfutil install device=2.7.16 --force

.. _ug_nrf54h20_install_toolchain:

Installing the |NCS| toolchain
==============================

.. include:: ../../../installation/install_ncs.rst
   :start-after: .. installncstoolchain-include-start
   :end-before: .. installncstoolchain-include-end

Getting the |NCS| code
======================

.. include:: ../../../installation/install_ncs.rst
   :start-after: .. getncscode-include-start
   :end-before: .. getncscode-include-end

Installing a terminal application
=================================

Install a terminal emulator, such as the `Serial Terminal app`_ (from the nRF Connect for Desktop application) or the nRF Terminal (part of the `nRF Connect for Visual Studio Code`_ extension).
Both of these terminal emulators start the required :ref:`toolchain environment <using_toolchain_environment>`.

.. _ug_nrf54h20_gs_bringup:

nRF54H20 DK bring-up
********************

The following sections describe the steps required for the nRF54H20 bring-up.

.. rst-class:: numbered-step

Programming the BICR
====================

The Board Information Configuration Registers (BICR) are non-volatile memory (NVM) registers that contain information on how the nRF54H20 SoC must interact with other board elements, including information about power and clock delivery to the SoC.
To prepare the nRF54H20 DK for first use, you must manually program the values of the BICR using a precompiled BICR binary file (:file:`bicr.hex`).

1. Download the `BICR new binary file`_.
#. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.

   .. note::
      On MacOS, when connecting the DK, you might see a popup with the message ``Disk Not Ejected Properly`` appearing multiple times.

#. List all the connected development kits to see their serial number (matching the one on the DK's sticker)::

      nrfutil device list

#. Move the BICR HEX file to a folder of your choice, then program the BICR by running nRF Util from that folder using the following command::

      nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware bicr.hex --core Application --serial-number <serial_number>

.. rst-class:: numbered-step

.. _ug_nrf54h20_SoC_binaries:

Programming the nRF54H20 SoC binaries
=====================================

After programming the BICR, program the nRF54H20 SoC with the :ref:`nRF54H20 SoC binaries <abi_compatibility>`.
This bundle contains the precompiled firmware for the :ref:`Secure Domain <ug_nrf54h20_secure_domain>` and :ref:`System Controller <ug_nrf54h20_sys_ctrl>`.
To program the nRF54H20 SoC binaries to the nRF54H20 DK, do the following:

1. Download the `nRF54H20 SoC binaries v0.9.6`_, compatible with the nRF54H20 DK v0.9.0 and later revisions.

   .. note::
      On MacOS, ensure that the ZIP file is not unpacked automatically upon download.

#. Move the :file:`.zip` bundle to a folder of your choice, then run nRF Util to program the binaries using the following command::

      nrfutil device x-provision-nrf54h --firmware <path-to_bundle_zip_file> --serial-number <serial_number>

.. rst-class:: numbered-step

Transitioning the nRF54H20 SoC to RoT
=====================================

The current nRF54H20 DK comes with its lifecycle state (LCS) set to ``EMPTY``.
To operate correctly, you must transition its lifecycle state to Root of Trust (``RoT``).

.. note::
   The forward transition to LCS ``RoT`` is permanent.
   After the transition, it is impossible to transition backward to LCS ``EMPTY``.

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
#. Run the following command to build the sample for application and radio cores::

      west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.sysbuild.hello_world.nrf54h20dk_cpuapp_cpurad .

You can now program the sample.
If you have multiple Nordic Semiconductor devices, ensure that only the nRF54H20 DK you want to program remains connected.

.. code-block:: console

   west flash

Make sure you have the :ref:`nrfutil device <ug_nrf54h20_install_toolchain>` command installed for ``west flash`` to work with the nRF54H20 DK.
This command builds and programs the sample automatically on both the application core and the Peripheral Processor (PPR) of the nRF54H20 SoC.

.. include:: /includes/nRF54H20_erase_UICR.txt

.. _ug_nrf54h20_sample_reading_logs:

Reading the logs
****************

With the :zephyr:code-sample:`sysbuild_hello_world` sample programmed, the nRF54H20 DK outputs logs for the application core and the configured remote processor.
The logs are output over UART.

To read the logs from the :zephyr:code-sample:`sysbuild_hello_world` sample programmed to the nRF54H20 DK, complete the following steps:

1. Connect to the DK with a terminal emulator (for example, the `Serial Terminal app`_) using the :ref:`default serial port connection settings <test_and_optimize>`.
#. Press the **Reset** button on the PCB to reset the DK.
#. Observe the console output for the application core:

   .. code-block:: console

      *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
      Hello world from nrf54h20dk/nrf54h20/cpuapp

.. note::
   If no output appears when using nRF Serial Terminal, select a different serial port in the terminal application.

For more information on how logging works in the |NCS|, consult the :ref:`ug_logging` and :ref:`zephyr:logging_api` documentation pages.

Next steps
**********

You are now all set to use the nRF54H20 DK.
See the following links for where to go next:

* :ref:`ug_nrf54h20_architecture` for information about the multicore System on Chip.
  This includes descriptions of core responsibilities, their interprocessor interactions, memory mapping, and the boot sequence.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
* :ref:`configuration_and_build` documentation to learn more about the |NCS| development environment.
* :ref:`ug_nrf54h` documentation for more advanced topics related to the nRF54H20.

If you want to go through an online training course to familiarize yourself with Bluetooth® Low Energy and the development of Bluetooth LE applications, enroll in the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.

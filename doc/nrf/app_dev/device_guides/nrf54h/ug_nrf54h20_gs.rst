.. _ug_nrf54h20_gs:

Getting started with the nRF54H20 DK
####################################

.. contents::
   :local:
   :depth: 2

.. note::
   If you are migrating from an earlier version of the |NCS|, see :ref:`migration_guides`.

This guide gets you started with your new nRF54H20 Development Kit (DK) using the |NCS| for the first time.
It guides you through the following:

1. Installing the |NCS| and other required software and tools.
#. Preparing the nRF54H20 DK for first use:

   a. Programming the nRF54H20 DK's Board Information Configuration Registers (BICR) using the provided binary file.
   #. Programming the Secure Domain and System Controller of the nRF54H20 SoC using the provided IronSide SE binaries.
   #. Transitioning the SoC's lifecycle state (LCS) to Root of Trust (RoT).

#. Programming the :zephyr:code-sample:`sysbuild_hello_world` sample on the DK.
#. Testing the sample by capturing and reviewing the UART console output.

.. _ug_nrf54h20_gs_requirements:

Hardware requirements
*********************

To follow this guide, make sure you have all the required hardware:

* nRF54H20 DK version PCA10175 Engineering C - v0.9.0 and later DK revisions in lifecycle state (LCS) ``EMPTY`` (new DKs will always be in LCS ``EMPTY``).
  Check the version number on your DK's sticker to verify its compatibility with the |NCS|.
* USB-C cable.

.. rst-class:: numbered-step

Installing the |NCS|
********************

Install the |NCS| following the instructions for both |nRFVSC| and the command line in the :ref:`install_ncs` documentation page.

.. rst-class:: numbered-step

Installing additional nRF Util commands
***************************************

After installing the |NCS| and its toolchain, you get the :ncs-tool-version:`NRFUTIL_VERSION_WIN10` version of nRF Util core module (``nrfutil``).
Using the nRF54H20 DK with the |NCS| version |release| also requires the following nRF Util components:

.. _ug_nrf54h20_nrfutil_comm_ver:

+----------------------------------------------+-------------------------------------+
| nRF Util component                           | Required version                    |
+==============================================+=====================================+
| nRF Util (``nrfutil``)                       | Latest                              |
+----------------------------------------------+-------------------------------------+
| nRF Util ``device`` command                  | version |54H_nrfutil_device_ver|    |
+----------------------------------------------+-------------------------------------+
| nRF Util ``trace`` command                   | version |54H_nrfutil_trace_ver|     |
+----------------------------------------------+-------------------------------------+

To install the required versions of the nRF Util commands, complete the following steps:

1. Update your existing nRF Util installation:

   a. Make sure you have removed the lock on the nRF Util installation to be able to install other nRF Util commands.
      See `Locking nRF Util home directory`_ in the tool documentation for more information.
   #. Run the following command to update the core module to the latest version:

      .. code-block::

         nrfutil self-upgrade

      For more information, consult the `Upgrading nRF Util core module`_ documentation.

#. Install the required versions of the nRF Util commands:

   .. parsed-literal::
      :class: highlight

      nrfutil install device=\ |54H_nrfutil_device_ver|\  --force
      nrfutil install trace=\ |54H_nrfutil_trace_ver|\  --force

   For more information, see the `Installing specific versions of nRF Util commands`_ documentation.

#. Verify the installation of the nRF Util commands:

   .. code-block::

      nrfutil device --version
      nrfutil trace --version

   The output will show the installed versions.
   Ensure they match the required versions.

.. rst-class:: numbered-step

Installing a terminal application
*********************************

Install a terminal emulator, such as the `Serial Terminal app`_ (from the nRF Connect for Desktop application) or the nRF Terminal (part of the `nRF Connect for Visual Studio Code`_ extension).
Both of these terminal emulators start the required :ref:`toolchain environment <using_toolchain_environment>`.

.. _ug_nrf54h20_gs_bringup:
.. _ug_nrf54h20_gs_bringup_bicr:
.. _ug_nrf54h20_gs_bicr:

.. rst-class:: numbered-step

Bring-up step: Programming the BICR
***********************************

The Board Information Configuration Registers (BICR) are non-volatile memory (NVM) registers that contain information on how the nRF54H20 SoC must interact with other board elements, including information about power and clock delivery to the SoC.
To prepare the nRF54H20 DK for its first use, you must manually program the required values into the BICR using a precompiled BICR binary file (:file:`bicr.hex`).

1. Download the `nRF54H20 DK BICR binary file`_.
#. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.

   .. note::
      On MacOS, when connecting the DK, you might see a popup with the message ``Disk Not Ejected Properly`` appearing multiple times.

#. List all the connected development kits to see their serial number (matching the one on the DK's sticker)::

      nrfutil device list

#. Move the BICR HEX file to a folder of your choice, then program the BICR by running nRF Util from that folder using the following command::

      nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware bicr.hex --core Application --serial-number <serial_number>

.. note::
   After you program the BICR, the LFCLK calibrates on first boot.
   Do not expect accurate LFCLK timing for about 3.5–4 seconds.
   If calibration does not complete, the system controller (sysctrl) starts calibration on the next boot.

.. _ug_nrf54h20_SoC_binaries:
.. _ug_nrf54h20_gs_bringup_soc_bin:

.. rst-class:: numbered-step

Bring-up step: Programming the nRF54H20 IronSide SE binaries
************************************************************

.. note::
   To program the nRF54H20 IronSide SE binaries on your nRF54H20 SoC-based device, your device must be in lifecycle state (LCS) ``EMPTY``.
   This means that this is likely the first use of your nRF54H20 DK.

   Devices already provisioned using SUIT-based binaries and in LCS ``RoT`` cannot be transitioned back to LCS ``EMPTY``.
   For more information, see :ref:`abi_compatibility`.

After programming the BICR, program the :ref:`nRF54H20 IronSide SE binaries <abi_compatibility>`.
This bundle contains the precompiled firmware for the :ref:`Secure Domain <ug_nrf54h20_secure_domain>` and :ref:`System Controller <ug_nrf54h20_sys_ctrl>`.
To program the nRF54H20 IronSide SE binaries to the nRF54H20 DK, do the following:

1. Download the `latest nRF54H20 IronSide SE binaries`_.

   .. note::
      On MacOS, ensure that the ZIP file is not unpacked automatically upon download.

#. Move the :file:`.zip` bundle to a folder of your choice, then run nRF Util to program the binaries using the following command::

      nrfutil device x-provision-nrf54h --firmware <path-to_bundle_zip_file> --serial-number <serial_number>

.. _ug_nrf54h20_gs_bringup_transition:

.. rst-class:: numbered-step

Bring-up step: Transitioning the nRF54H20 SoC to RoT
****************************************************

The current nRF54H20 DK comes with its lifecycle state (LCS) set to ``EMPTY``.
To operate correctly, you must transition its lifecycle state to Root of Trust (``RoT``).

.. caution::
   The forward transition to LCS ``RoT`` is permanent.
   After the transition, it is impossible to transition backward to LCS ``EMPTY``.

To transition the LCS to ``RoT``, do the following:

1. Set the LCS of the nRF54H20 SoC to Root of Trust using the following command::

      nrfutil device x-adac-lcs-change --life-cycle rot --serial-number <serial_number>

#. After the LCS transition, reset the device::

      nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

.. _ug_nrf54h20_gs_sample:

.. rst-class:: numbered-step

Building and programming your first sample to the nRF54H20 DK
*************************************************************

The :zephyr:code-sample:`sysbuild_hello_world` sample is a multicore sample running on both the application core (``cpuapp``) and the Peripheral Processor (PPR, ``cpuppr``).
It uses the ``nrf54h20dk/nrf54h20/cpuapp`` board target.

To build and program the sample to the nRF54H20 DK, complete the following steps:

1. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.
#. Start the toolchain environment for your operating system using the following command pattern, with ``--ncs-version`` corresponding to the |NCS| version you have installed:

   .. tabs::

      .. tab:: Windows

         .. parsed-literal::
            :class: highlight

            nrfutil sdk-manager toolchain launch --ncs-version *version* --terminal

         For example:

         .. parsed-literal::
            :class: highlight

            nrfutil sdk-manager toolchain launch --ncs-version |release| --terminal

         This example command starts the toolchain environment for the |NCS| |release|.

      .. tab:: Linux

         .. parsed-literal::
            :class: highlight

            nrfutil sdk-manager toolchain launch --ncs-version *version* --shell

         For example:

         .. parsed-literal::
            :class: highlight

            nrfutil sdk-manager toolchain launch --ncs-version |release| --shell

         This example command starts the toolchain environment for the |NCS| |release|.

      .. tab:: macOS

         .. parsed-literal::
            :class: highlight

            nrfutil sdk-manager toolchain launch --ncs-version *version* --shell

         For example:

         .. parsed-literal::
            :class: highlight

            nrfutil sdk-manager toolchain launch --ncs-version |release| --shell

         This example command starts the toolchain environment for the |NCS| |release|.

#. In the terminal window, navigate to the :file:`zephyr/samples/sysbuild/hello_world` folder containing the sample.
#. Run the following command to build the sample for application and radio cores::

      west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.sysbuild.hello_world.nrf54h20dk_cpuapp_cpurad .

#. Program the sample to the DK.
   If you have multiple Nordic Semiconductor devices, ensure that only the nRF54H20 DK you want to program remains connected.

   .. code-block:: console

      west flash

   This command programs both the application core and the Peripheral Processor (PPR) of the nRF54H20 SoC.

.. _ug_nrf54h20_sample_reading_logs:

.. rst-class:: numbered-step

Testing your first sample on the nRF54H20 DK
********************************************

Now that the :zephyr:code-sample:`sysbuild_hello_world` sample is programmed, verify its operation by capturing the UART logs from both the application core and the peripheral processor:

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
* :ref:`security` documentation for information on security features available in the |NCS|.

If you want to go through an online training course to familiarize yourself with Bluetooth® Low Energy and the development of Bluetooth LE applications, enroll in the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.

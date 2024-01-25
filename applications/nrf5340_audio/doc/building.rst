.. _nrf53_audio_app_building:

Building and running nRF5340 Audio applications
###############################################

.. contents::
   :local:
   :depth: 2

This nRF5340 Audio application source files can be found in their respective folders under :file:`applications/nrf5340_audio` in the nRF Connect SDK folder structure.

.. note::
   Building and programming the nRF5340 Audio applications is different from the :ref:`standard procedure <ug_nrf5340_building>` of building and programming for the nRF5340 DK.
   This is because the nRF5340 Audio applications only build and program the files for the application core.
   |nrf5340_audio_net_core_hex_note|

You can build and program the applications in one of the following ways:

* :ref:`nrf53_audio_app_building_script`.
  This is the suggested method.
  Using this method allows you to build and program multiple development kits at the same time.
* :ref:`nrf53_audio_app_building_standard`.
  Using this method requires building and programming each development kit separately.

.. important::
   Building and programming using the |nRFVSC| is currently not supported.

.. note::
   You might want to check the :ref:`nRF5340 Audio application known issues <known_issues_nrf5340audio>` before building and programming the applications.

.. _nrf53_audio_app_dk_testing_out_of_the_box:

Testing out of the box
**********************

Each development kit comes preprogrammed with basic firmware that indicates if the kit is functional.
Before building the application, you can verify if the kit is working by completing the following steps:

1. Plug the device into the USB port.
#. Turn on the development kit using the On/Off switch.
#. Observe **RGB** (bottom side LEDs around the center opening that illuminate the Nordic Semiconductor logo) turn solid yellow, **OB/EXT** turn solid green, and **LED3** start blinking green.

You can now program the development kit.

.. _nrf53_audio_app_building_script:

Building and programming using script
*************************************

The suggested method for building each of the applications and programming it to the development kit is running the :file:`buildprog.py` Python script, which is located in the :file:`applications/nrf5340_audio/tools/buildprog` directory.
The script automates the process of selecting :ref:`configuration files <nrf53_audio_app_configuration_files>` and building different applications.
This eases the process of building and programming images for multiple development kits.

Preparing the JSON file
=======================

The script depends on the settings defined in the :file:`nrf5340_audio_dk_devices.json` file.
Before using the script, make sure to update this file with the following information for each development kit you want to use:

* ``nrf5340_audio_dk_snr`` -- This field lists the SEGGER serial number.
  You can check this number on the sticker on the nRF5340 Audio development kit.
  Alternatively, connect the development kit to your PC and run ``nrfjprog -i`` in a command window to print the SEGGER serial number of the kit.
* ``nrf5340_audio_dk_dev`` -- This field assigns the specific nRF5340 Audio development kit to be ``headset`` or ``gateway``.
* ``channel`` -- This field is valid only for headsets.
  It sets the channels on which the headset is meant to work.
  When no channel is set, the headset is programmed as a left channel one.

.. _nrf53_audio_app_building_script_running:

Running the script
==================

After editing the :file:`nrf5340_audio_dk_devices.json` file, run :file:`buildprog.py` to build the firmware for the development kits.
The building command for running the script requires providing the following parameters:

* Core type (``-c`` parameter): ``app``, ``net``, or ``both``
* Application version (``-b`` parameter): either ``release`` or ``debug``
* Device type (``-d`` parameter): ``headset``, ``gateway``, or ``both``
* DFU type (``-m`` parameter): ``internal``, ``external``
* Network core bootloader minimal size (``-M``)

See the following examples of the parameter usage with the command run from the :file:`buildprog` directory:

* Example 1: The following command builds headset and gateway applications using the script for the application core with the ``debug`` application version:

  .. code-block:: console

     python buildprog.py -c app -b debug -d both

* Example 2: The following command builds headset and gateway applications using the script for both the application and the network core (``both``).
  It builds with the ``release`` application version, with the DFU external flash memory layout enabled, and using the minimal size of the network core bootloader:

  .. code-block:: console

     python buildprog.py -c both -b release -d both -m external -M

The command can be run from any location, as long as the correct path to :file:`buildprog.py` is given.

The build files are saved in the :file:`applications/nrf5340_audio/build` directory.
The script creates a directory for each application version and device type combination.
For example, when running the command above, the script creates the :file:`dev_gateway/build_debug` and :file:`dev_headset/build_debug` directories.

Programming with the script
   The development kits are programmed according to the serial numbers set in the JSON file.
   Make sure to connect the development kits to your PC using USB and turn them on using the **POWER** switch before you run the command.

   The following parameters are available for programming:

   * Programming (``-p`` parameter) -- If you run the building script with this parameter, you can program one or both of the cores after building the files.
   * Sequential programming (``-s`` parameter) -- If you encounter problems while programming, include this parameter alongside other parameters to program sequentially.

   The command for programming can look as follows:

   .. code-block:: console

      python buildprog.py -c both -b debug -d both -p

   This command builds the headset and the gateway applications with the ``debug`` application version and programs the application core.
   Given the ``-c both`` parameter, it also takes the precompiled Bluetooth Low Energy Controller binary from the :file:`nrf/lib/bin/bt_ll_acs_nrf53/bin` directory and programs it to the network core of both the gateway and the headset.

   .. note::
      If the programming command fails because of :ref:`readback_protection_error`, run :file:`buildprog.py` with the ``--recover_on_fail`` or ``-f`` parameter to recover and re-program automatically when programming fails.
      For example, using the programming command example above:

      .. code-block:: console

         python buildprog.py -c both -b debug -d both -p --recover_on_fail

   If you want to program firmware that has DFU enabled, you must include the DFU parameters in the command.
   The command for programming with DFU enabled can look as follows:

   .. code-block:: console

     python buildprog.py -c both -b release -d both -m external -M -p

Getting help
   Run ``python buildprog.py -h`` for information about all available script parameters.

Configuration table overview
   When running the script command, a table similar to the following one is displayed to provide an overview of the selected options and parameter values:

   .. code-block:: console

      +------------+----------+---------+--------------+---------------------+---------------------+
      | snr        | snr conn | device  | only reboot  | core app programmed | core net programmed |
      +------------+----------+---------+--------------+---------------------+---------------------+
      | 1010101010 | True     | headset | Not selected | Selected TBD        | Not selected        |
      | 2020202020 | True     | gateway | Not selected | Selected TBD        | Not selected        |
      | 3030303030 | True     | headset | Not selected | Selected TBD        | Not selected        |
      +------------+----------+---------+--------------+---------------------+---------------------+

   See the following table for the meaning of each column and the list of possible values:

   +-----------------------+-----------------------------------------------------------------------------------------------------+-------------------------------------------------+
   | Column                | Indication                                                                                          | Possible values                                 |
   +=======================+=====================================================================================================+=================================================+
   | ``snr``               | Serial number of the device, as provided in the :file:`nrf5340_audio_dk_devices.json` file.         | Serial number.                                  |
   +-----------------------+-----------------------------------------------------------------------------------------------------+-------------------------------------------------+
   | ``snr conn``          | Whether the device with the provided serial number is connected to the PC with a serial connection. | ``True`` - Connected.                           |
   |                       |                                                                                                     +-------------------------------------------------+
   |                       |                                                                                                     | ``False`` - Not connected.                      |
   +-----------------------+-----------------------------------------------------------------------------------------------------+-------------------------------------------------+
   | ``device``            | Device type, as provided in the :file:`nrf5340_audio_dk_devices.json` file.                         | ``headset`` - Headset.                          |
   |                       |                                                                                                     +-------------------------------------------------+
   |                       |                                                                                                     | ``gateway`` - Gateway.                          |
   +-----------------------+-----------------------------------------------------------------------------------------------------+-------------------------------------------------+
   | ``only reboot``       | Whether the device is to be only reset and not programmed.                                          | ``Not selected`` - No reset.                    |
   |                       | This depends on the ``-r`` parameter in the command, which overrides other parameters.              +-------------------------------------------------+
   |                       |                                                                                                     | ``Selected TBD`` - Only reset requested.        |
   |                       |                                                                                                     +-------------------------------------------------+
   |                       |                                                                                                     | ``Done`` - Reset done.                          |
   |                       |                                                                                                     +-------------------------------------------------+
   |                       |                                                                                                     | ``Failed`` - Reset failed.                      |
   +-----------------------+-----------------------------------------------------------------------------------------------------+-------------------------------------------------+
   |``core app programmed``| Whether the application core is to be programmed.                                                   | ``Not selected`` - Core will not be programmed. |
   |                       | This depends on the value provided to the ``-c`` parameter (see above).                             +-------------------------------------------------+
   |                       |                                                                                                     | ``Selected TBD`` - Programming requested.       |
   |                       |                                                                                                     +-------------------------------------------------+
   |                       |                                                                                                     | ``Done`` - Programming done.                    |
   |                       |                                                                                                     +-------------------------------------------------+
   |                       |                                                                                                     | ``Failed`` - Programming failed.                |
   +-----------------------+-----------------------------------------------------------------------------------------------------+-------------------------------------------------+
   |``core net programmed``| Whether the network core is to be programmed.                                                       | ``Not selected`` - Core will not be programmed. |
   |                       | This depends on the value provided to the ``-c`` parameter (see above).                             +-------------------------------------------------+
   |                       |                                                                                                     | ``Selected TBD`` - Programming requested.       |
   |                       |                                                                                                     +-------------------------------------------------+
   |                       |                                                                                                     | ``Done`` - Programming done.                    |
   |                       |                                                                                                     +-------------------------------------------------+
   |                       |                                                                                                     | ``Failed`` - Programming failed.                |
   +-----------------------+-----------------------------------------------------------------------------------------------------+-------------------------------------------------+

.. _nrf53_audio_app_building_standard:

Building and programming using command line
*******************************************

You can also build the nRF5340 Audio applications using the standard |NCS| :ref:`build steps <programming_cmd>` for the command line.

.. note::
   Using this method requires you to build and program each development kit one at a time before moving to the next configuration, which can be time-consuming.
   :ref:`nrf53_audio_app_building_script` is recommended.

Building the application
========================

Complete the following steps to build the application:

1. Choose the combination of build flags:

   a. Choose the device type by using one of the following options:

      * For headset device: ``-DCONFIG_AUDIO_DEV=1``
      * For gateway device: ``-DCONFIG_AUDIO_DEV=2``

   #. Choose the application version by using one of the following options:

      * For the debug version: No build flag needed.
      * For the release version: ``-DCONF_FILE="prj_release.conf"``

#. Build the application using the standard :ref:`build steps <programming_cmd>` for the command line.
   For example, if you want to build the firmware for the application core as a headset using the ``release`` application version, you can run the following command from the :file:`applications/nrf5340_audio/` directory:

   .. code-block:: console

      west build -b nrf5340_audio_dk_nrf5340_cpuapp --pristine -- -DCONFIG_AUDIO_DEV=1 -DCONF_FILE="prj_release.conf"

   Unlike when :ref:`nrf53_audio_app_building_script`, this command creates the build files directly in the :file:`build` directory.
   This means that you first need to program the headset development kits before you build and program gateway development kits.
   Alternatively, you can add the ``-d`` parameter to the ``west`` command to specify a custom build folder. This lets you build firmware for both
   headset and gateway before programming any development kits.

Programming the application
===========================

After building the files for the development kit you want to program, complete the following steps to program the applications from the command line:

1. Plug the device into the USB port.

   .. note::
      |usb_known_issues|

#. Turn on the development kit using the On/Off switch.
#. Open a command prompt.
#. Run the following command to print the SEGGER serial number of your development kit:

   .. code-block:: console

      nrfjprog -i

   .. note::
      Pay attention to which device is to be programmed with the gateway HEX file and which devices are to be programmed with the headset HEX file.

#. Program the network core on the development kit by running the following command:

   .. code-block:: console

      nrfjprog --program bin/*.hex --chiperase --coprocessor CP_NETWORK -r

   |nrf5340_audio_net_core_hex_note|
#. Program the application core on the development kit with the respective HEX file from the :file:`build` directory by running the following command:

   .. code-block:: console

      nrfjprog --program build/zephyr/zephyr.hex --coprocessor CP_APPLICATION --chiperase -r

   In this command, :file:`build/zephyr/zephyr.hex` is the HEX binary file for the application core.
   If a custom build folder is specified, the path to this folder must be used instead of :file:`build/`.
#. If any device is not programmed due to :ref:`readback_protection_error`, complete the following steps:

   a. Run the following commands to recover the device:

      .. code-block:: console

         nrfjprog --recover --coprocessor CP_NETWORK
         nrfjprog --recover

   #. Repeat steps 5 and 6 to program both cores again.

#. When using the default CIS configuration, if you want to use two headset devices, you must also populate the UICR with the desired channel for each headset.
   Use the following commands, depending on which headset you want to populate:

   * Left headset:

     .. code-block:: console

        nrfjprog --memwr 0x00FF80F4 --val 0

   * Right headset:

     .. code-block:: console

        nrfjprog --memwr 0x00FF80F4 --val 1

   Select the correct board when prompted with the popup or add the ``--snr`` parameter followed by the SEGGER serial number of the correct board at the end of the ``nrfjprog`` command.

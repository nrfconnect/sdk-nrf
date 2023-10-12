.. _nrf53_audio_app_description:

nRF5340 Audio configuration and testing
#######################################

.. contents::
   :local:
   :depth: 2

The nRF5340 Audio application has the following unique characteristics:

* It is developed for use only with the :ref:`nRF5340 Audio development kit <nrf53_audio_app_requirements>`.
* In its default configuration, the application requires the :ref:`LC3 software codec <nrfxlib:lc3>`.
* The application also comes with various application-specific tools, including the :file:`buildprog.py` Python script that simplifies building and programming the firmware.

.. _nrf53_audio_app_requirements:

Requirements
************

The nRF5340 Audio application is designed to be used only with the following hardware:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340_audio_dk_nrf5340

.. note::
   The application supports PCA10121 revisions 1.0.0 or above.
   The application is also compatible with the following pre-launch revisions:

   * Revisions 0.8.0 and above.

You need at least two nRF5340 Audio development kits (one with the gateway firmware and one with headset firmware) to test the application.
For CIS with TWS in mind, three kits are required.

.. _nrf53_audio_app_requirements_codec:

Software codec requirements
===========================

The nRF5340 Audio application only supports the :ref:`LC3 software codec <nrfxlib:lc3>`, developed specifically for use with LE Audio.

.. _nrf53_audio_app_dk:
.. _nrf53_audio_app_dk_features:

nRF5340 Audio development kit
=============================

The nRF5340 Audio development kit is a hardware development platform that demonstrates the nRF5340 Audio application.
Read the `nRF5340 Audio DK Hardware`_ documentation on Nordic Semiconductor Infocenter for more information about this development kit.

.. _nrf53_audio_app_configuration_files:

nRF5340 Audio configuration files
=================================

The nRF5340 Audio application uses :file:`Kconfig.defaults` files to change configuration defaults automatically, based on the different application versions and device types.

Only one of the following :file:`.conf` files is included when building:

* :file:`prj.conf` is the default configuration file and it implements the debug application version.
* :file:`prj_release.conf` is the optional configuration file and it implements the release application version.
  No debug features are enabled in the release application version.
  When building using the command line, you must explicitly specify if :file:`prj_release.conf` is going to be included instead of :file:`prj.conf`.
  See :ref:`nrf53_audio_app_building` for details.

Requirements for FOTA
=====================

To test Firmware Over-The-Air (FOTA), you need an Android or iOS device with the `nRF Connect Device Manager`_ app installed.

If you want to do FOTA upgrades for the application core and the network core at the same time, you need an external flash shield.
See :ref:`nrf53_audio_app_configuration_configure_fota` for more details.

.. _nrf53_audio_app_ui:

User interface
**************

The application implements a simple user interface based on the available PCB elements.
You can control the application using predefined switches and buttons while the LEDs display information.

.. _nrf53_audio_app_ui_switches:

Switches
========

The application uses the following switches on the supported development kit:

+-------------------+-------------------------------------------------------------------------------------+
| Switch            | Function                                                                            |
+===================+=====================================================================================+
| **POWER**         | Turns the development kit on or off.                                                |
+-------------------+-------------------------------------------------------------------------------------+
| **DEBUG ENABLE**  | Turns on or off power for debug features.                                           |
|                   | This switch is used for accurate power and current measurements.                    |
+-------------------+-------------------------------------------------------------------------------------+

.. _nrf53_audio_app_ui_buttons:

Buttons
=======

The application uses the following buttons on the supported development kit:

+---------------+-----------------------------------------------------------------------------------------------------------+
| Button        | Function                                                                                                  |
+===============+===========================================================================================================+
| **VOL-**      | Depending on the moment it is pressed:                                                                    |
|               |                                                                                                           |
|               | * Long-pressed during startup: Changes the headset to the left channel one.                               |
|               | * Pressed on the headset or the CIS gateway during playback: Turns the playback volume down (and unmutes).|
+---------------+-----------------------------------------------------------------------------------------------------------+
| **VOL+**      | Depending on the moment it is pressed:                                                                    |
|               |                                                                                                           |
|               | * Long-pressed during startup: Changes the headset to the right channel one.                              |
|               | * Pressed on the headset or the CIS gateway during playback: Turns the playback volume up (and unmutes).  |
+---------------+-----------------------------------------------------------------------------------------------------------+
| **PLAY/PAUSE**| Starts or pauses the playback.                                                                            |
+---------------+-----------------------------------------------------------------------------------------------------------+
| **BTN 4**     | Depending on the moment it is pressed:                                                                    |
|               |                                                                                                           |
|               | * Long-pressed during startup: Turns on the DFU mode, if                                                  |
|               |   the device is :ref:`configured for it<nrf53_audio_app_configuration_configure_fota>`.                   |
|               | * Pressed on the gateway during playback: Sends a test tone generated on the device.                      |
|               |   Use this tone to check the synchronization of headsets.                                                 |
|               | * Pressed on the gateway during playback multiple times: Changes the tone frequency.                      |
|               |   The available values are 1000 Hz, 2000 Hz, and 4000 Hz.                                                 |
|               | * Pressed on a BIS headset during playback: Change audio stream, if more than one is                      |
|               |   available.                                                                                              |
+---------------+-----------------------------------------------------------------------------------------------------------+
| **BTN 5**     | Depending on the moment it is pressed:                                                                    |
|               |                                                                                                           |
|               | * Long-pressed during startup: Clears the previously stored bonding information.                          |
|               | * Pressed during playback: Mutes the playback volume.                                                     |
|               | * Pressed on a BIS headset during playback: Change the gateway, if more than one is                       |
|               |   available.                                                                                              |
+---------------+-----------------------------------------------------------------------------------------------------------+
| **RESET**     | Resets the device to the originally programmed settings.                                                  |
|               | This reverts any changes made during testing, for example the channel switches with **VOL** buttons.      |
+---------------+-----------------------------------------------------------------------------------------------------------+

.. _nrf53_audio_app_ui_leds:

LEDs
====

To indicate the tasks performed, the application uses the LED behavior described in the following table:

+--------------------------+-----------------------------------------------------------------------------------------------------------+
| LED                      |Indication                                                                                                 |
+==========================+===========================================================================================================+
| **LED1**                 | Off - No Bluetooth connection.                                                                            |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Blinking blue - Depending on the device and the mode:                                                     |
|                          |                                                                                                           |
|                          | * Headset: Kits have started streaming audio (BIS and CIS modes).                                         |
|                          | * Gateway: Kit has connected to a headset (CIS mode) or has started broadcasting audio (BIS mode).        |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid blue - Headset, depending on the mode:                                                              |
|                          | Kits have connected to the gateway (CIS mode) or found a broadcasting stream (BIS mode).                  |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **LED2**                 | Off - Sync not achieved.                                                                                  |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid green - Sync achieved (both drift and presentation compensation are in the ``LOCKED`` state).       |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **LED3**                 | Blinking green - The nRF5340 Audio DK application core is running.                                        |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **CODEC**                | Off - No configuration loaded to the onboard hardware codec.                                              |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid green - Hardware codec configuration loaded.                                                        |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **RGB1**                 | Solid green - The device is programmed as the gateway.                                                    |
| (bottom side LEDs around +-----------------------------------------------------------------------------------------------------------+
| the center opening)      | Solid blue - The device is programmed as the left headset.                                                |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid magenta - The device is programmed as the right headset.                                            |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid yellow - The device is programmed with factory firmware.                                            |
|                          | It must be re-programmed as gateway or headset.                                                           |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid red (debug mode) - Fault in the application core has occurred.                                      |
|                          | See UART log for details and use the **RESET** button to reset the device.                                |
|                          | In the release mode, the device resets automatically with no indication on LED or UART.                   |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **RGB 2**                | Controlled by the Bluetooth LE Controller on the network core.                                            |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Blinking green - Ongoing CPU activity.                                                                    |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid red - Error.                                                                                        |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid white (all colors on) - The **RGB 2** LED is not initialized by the Bluetooth LE Controller.        |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **ERR**                  | PMIC error or a charging error (or both).                                                                 |
|                          | Also turns on when charging the battery exceeds seven hours, since the PMIC has a protection timeout,     |
|                          | which stops the charging.                                                                                 |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **CHG**                  | Off - Charge completed or no battery connected.                                                           |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid yellow - Charging in progress.                                                                      |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **OB/EXT**               | Off - No 3.3 V power available.                                                                           |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid green - On-board hardware codec selected.                                                           |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Solid yellow - External hardware codec selected.                                                          |
|                          | This LED turns solid yellow also when the devices are reset, for the time then pins are floating.         |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **FTDI SPI**             | Off - No data is written to the hardware codec using SPI.                                                 |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Yellow - The same SPI is used for both the hardware codec and the SD card.                                |
|                          | When this LED is yellow, the shared SPI is used by the FTDI to write data to the hardware codec.          |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **IFMCU**                | Off - No PC connection available.                                                                         |
| (bottom side)            +-----------------------------------------------------------------------------------------------------------+
|                          | Solid green - Connected to PC.                                                                            |
|                          +-----------------------------------------------------------------------------------------------------------+
|                          | Rapid green flash - USB enumeration failed.                                                               |
+--------------------------+-----------------------------------------------------------------------------------------------------------+
| **HUB**                  | Off - No PC connection available.                                                                         |
| (bottom side)            +-----------------------------------------------------------------------------------------------------------+
|                          | Green - Standard USB hub operation.                                                                       |
+--------------------------+-----------------------------------------------------------------------------------------------------------+

.. _nrf53_audio_app_configuration:

Configuration
*************

|config|

.. _nrf53_audio_app_configuration_select_bis:

Selecting the BIS mode
======================

The CIS mode is the default operating mode for the application.
To switch to the BIS mode, set the :ref:`CONFIG_TRANSPORT_BIS <config_nrf53_audio_app_options>` Kconfig option to ``y`` in the :file:`prj.conf` file for the debug version and in the :file:`prj_release.conf` file for the release version.

Enabling the BIS mode with two gateways
---------------------------------------

In addition to the standard BIS mode with one gateway, you can also add a second gateway device that the BIS headsets can receive audio stream from.
To configure the second gateway, add both the :ref:`CONFIG_TRANSPORT_BIS <config_nrf53_audio_app_options>` and the :ref:`CONFIG_BT_AUDIO_USE_BROADCAST_NAME_ALT <config_nrf53_audio_app_options>` Kconfig options set to ``y`` to the :file:`prj.conf` file for the debug version and to the :file:`prj_release.conf` file for the release version.
You can provide an alternative name to the second gateway using the :ref:`CONFIG_BT_AUDIO_BROADCAST_NAME_ALT <config_nrf53_audio_app_options>` or use the default alternative name.

You build each BIS gateway separately using the normal procedures from :ref:`nrf53_audio_app_building`.
After building the first gateway, configure the required Kconfig options for the second gateway and build the second gateway firmware.
Remember to program the two firmware versions to two separate gateway devices.

.. _nrf53_audio_app_configuration_select_bidirectional:

Selecting the CIS bidirectional communication
=============================================

The CIS unidirectional mode is the default operating mode for the application.
To switch to the bidirectional mode, set the :ref:`CONFIG_STREAM_BIDIRECTIONAL <config_nrf53_audio_app_options>` Kconfig option to ``y``  in the :file:`prj.conf` file (for the debug version) or in the :file:`prj_release.conf` file (for the release version).

.. _nrf53_audio_app_configuration_enable_walkie_talkie:

Enabling the walkie-talkie demo
-------------------------------

The walkie-talkie demo uses one or two bidirectional streams from the gateway to one or two headsets.
The PDM microphone is used as input on both the gateway and headset device.
To switch to using the walkie-talkie, set the :ref:`CONFIG_WALKIE_TALKIE_DEMO <config_nrf53_audio_app_options>` Kconfig option to ``y``  in the :file:`prj.conf` file (for the debug version) or in the :file:`prj_release.conf` file (for the release version).

.. _nrf53_audio_app_configuration_select_i2s:

Selecting the I2S serial
========================

In the default configuration, the gateway application uses the USB serial port as the audio source.
The :ref:`nrf53_audio_app_building` and :ref:`nrf53_audio_app_testing` steps also refer to using the USB serial connection.

To switch to using the I2S serial connection, set the :ref:`CONFIG_AUDIO_SOURCE_I2S <config_nrf53_audio_app_options>` Kconfig option to ``y`` in the :file:`prj.conf` file for the debug version and in the :file:`prj_release.conf` file for the release version.

When testing the application, an additional audio jack cable is required to use I2S.
Use this cable to connect the audio source (PC) to the analog **LINE IN** on the development kit.

.. _nrf53_audio_app_configuration_configure_fota:

Configuring FOTA upgrades
=========================

.. caution::
	Firmware based on the |NCS| versions earlier than v2.1.0 does not support DFU.
	FOTA is not available for those versions.

	You can test performing separate application and network core upgrades, but for production, both cores must be updated at the same time.
	When updates take place in the inter-core communication module (HCI RPMsg), communication between the cores will break if they are not updated together.

You can configure Firmware Over-The-Air (FOTA) upgrades to replace the applications on both the application core and the network core.
The nRF5340 Audio application supports the following types of DFU flash memory layouts:

* Internal flash memory layout - which supports only single-image DFU.
* External flash memory layout - which supports :ref:`multi-image DFU <ug_nrf5340_multi_image_dfu>`.

The LE Audio Controller Subsystem for nRF53 supports both the normal and minimal sizes of the bootloader.
The minimal size is specified using the :kconfig:option:`CONFIG_NETBOOT_MIN_PARTITION_SIZE`.

Hardware requirements for external flash memory DFU
---------------------------------------------------

To enable the external flash DFU, you need an additional flash memory shield.
See `Requirements for external flash memory DFU`_ in the nRF5340 Audio DK Hardware documentation in Infocenter for more information.

Enabling FOTA upgrades
----------------------

The FOTA upgrades are only available when :ref:`nrf53_audio_app_building_script`.
With the appropriate parameters provided, the :file:`buildprog.py` Python script will add overlay files for the given DFU type.
To enable the desired FOTA functions:

* To define flash memory layout, include the ``-m internal`` parameter for the internal layout (when using the ``release`` application version) or the ``-m external`` parameter for the external layout (when using either ``release`` or ``debug``).
* To use the minimal size network core bootloader, add the ``-M`` parameter.

For the full list of parameters and examples, see the :ref:`nrf53_audio_app_building_script_running` section.

FOTA build files
----------------

The generated FOTA build files use the following naming patterns:

* For multi-image DFU, the file is called ``dfu_application.zip``.
  This file updates two cores with one single file.
* For single-image DFU, the bin file for the application core is called ``app_update.bin``.
  The bin file for the network core is called ``net_core_app_update.bin``.
  In this scenario, the cores are updated one by one with two separate files in two actions.

See :ref:`app_build_output_files` for more information about the image files.

.. note::
   |nrf5340_audio_net_core_hex_note|

Entering the DFU mode
---------------------

The |NCS| uses :ref:`SMP server and mcumgr <zephyr:device_mgmt>` as the DFU backend.
Unlike the CIS and BIS modes for gateway and headsets, the DFU mode is advertising using the SMP server service.
For this reason, to enter the DFU mode, you must long press **BTN 4** during each device startup to have the nRF5340 Audio DK enter the DFU mode.

To identify the devices before the DFU takes place, the DFU mode advertising names mention the device type directly.
The names follow the pattern in which the device *ROLE* is inserted before the ``_DFU`` suffix.
For example:

* Gateway: NRF5340_AUDIO_GW_DFU
* Left Headset: NRF5340_AUDIO_HL_DFU
* Right Headset: NRF5340_AUDIO_HR_DFU

The first part of these names is based on :kconfig:option:`CONFIG_BT_DEVICE_NAME`.

.. note::
   When performing DFU for the nRF5340 Audio application, there will be one or more error prints related to opening flash area ID 1.
   This is due to restrictions in the DFU system, and the error print is expected.
   The DFU process should still complete successfully.

.. _nrf53_audio_app_building:

Building and running
********************

This sample can be found under :file:`applications/nrf5340_audio` in the nRF Connect SDK folder structure.

.. note::
   Building and programming the nRF5340 Audio application is different from the :ref:`standard procedure <ug_nrf5340_building>` of building and programming for the nRF5340 DK.
   This is because the nRF5340 Audio application only builds and programs the files for the application core.
   |nrf5340_audio_net_core_hex_note|

You can build and program the application in one of the following ways:

* :ref:`nrf53_audio_app_building_script`.
  This is the suggested method.
  Using this method allows you to build and program multiple development kits at the same time.
* :ref:`nrf53_audio_app_building_standard`.
  Using this method requires building and programming each development kit separately.

.. important::
   Building and programming using the |nRFVSC| is currently not supported.

.. note::
   You might want to check the :ref:`nRF5340 Audio application known issues <known_issues_nrf5340audio>` before building and programming the application.

Testing out of the box
======================

Each development kit comes preprogrammed with basic firmware that indicates if the kit is functional.
Before building the application, you can verify if the kit is working by completing the following steps:

1. Plug the device into the USB port.
#. Turn on the development kit using the On/Off switch.
#. Observe **RGB1** (bottom side LEDs around the center opening that illuminate the Nordic Semiconductor logo) turn solid yellow, **OB/EXT** turn solid green, and **LED3** start blinking green.

You can now program the development kits with either gateway or headset firmware before they can be used.

.. _nrf53_audio_app_adding_FEM_support:

Adding FEM support
==================

You can add support for the nRF21540 front-end module (FEM) to this application by using one of the following options, depending on how you decide to build the application:

* If you opt for :ref:`nrf53_audio_app_building_script`, add the ``--nrf21540`` to the script's building command.
* If you opt for :ref:`nrf53_audio_app_building_standard`, add the ``-DSHIELD=nrf21540ek_fwd`` to the ``west build`` command.
  For example:

  .. code-block:: console

     west build -b nrf5340_audio_dk_nrf5340_cpuapp --pristine -- -DCONFIG_AUDIO_DEV=1 -DSHIELD=nrf21540ek_fwd -DCONF_FILE=prj_release.conf

To set the TX power output, use the :ref:`CONFIG_NRF_21540_MAIN_TX_POWER <config_nrf53_audio_app_options>` and :ref:`CONFIG_NRF_21540_PRI_ADV_TX_POWER <config_nrf53_audio_app_options>` Kconfig options.

.. note::
   When you build the nRF5340 Audio application with the nRF21540 FEM support, the :ref:`lib_bt_ll_acs_nrf53_readme` does not support the +20 dBm setting.
   This is because of a power class restriction in the controller's QDID.

See :ref:`ug_radio_fem` for more information about FEM in the |NCS|.

.. _nrf53_audio_app_building_script:

Building and programming using script
=====================================

The suggested method for building the application and programming it to the development kit is running the :file:`buildprog.py` Python script, which is located in the :file:`applications/nrf5340_audio/tools/buildprog` directory.
The script automates the process of selecting :ref:`configuration files <nrf53_audio_app_configuration_files>` and building different versions of the application.
This eases the process of building and programming images for multiple development kits.

Preparing the JSON file
-----------------------

The script depends on the settings defined in the :file:`nrf5340_audio_dk_devices.json` file.
Before using the script, make sure to update this file with the following information for each development kit you want to use:

* ``nrf5340_audio_dk_snr`` -- This field lists the SEGGER serial number.
  You can check this number on the sticker on the nRF5340 Audio development kit.
  Alternatively, connect the development kit to your PC and run ``nrfjprog -i`` in a command window to print the SEGGER serial number of the kit.
* ``nrf5340_audio_dk_dev`` -- This field assigns the specific nRF5340 Audio development kit to be a headset or a gateway.
* ``channel`` -- This field is valid only for headsets operating in the CIS mode.
  It sets the channels on which the headset is meant to work.
  When no channel is set, the headset is programmed as a left channel one.

.. _nrf53_audio_app_building_script_running:

Running the script
------------------

After editing the :file:`nrf5340_audio_dk_devices.json` file, run :file:`buildprog.py` to build the firmware for the development kits.
The building command for running the script requires providing the following parameters, in line with :ref:`nrf53_audio_app_configuration_files`:

* Core type (``-c`` parameter): ``app``, ``net``, or ``both``
* Application version (``-b`` parameter): either ``release`` or ``debug``
* Device type (``-d`` parameter): ``headset``, ``gateway``, or ``both``
* DFU type (``-m`` parameter): ``internal``, ``external``
* Network core bootloader minimal size (``-M``)

See the following examples of the parameter usage with the command run from the :file:`buildprog` directory:

* Example 1: The following command builds the application using the script for the application core with the ``debug`` application version for both the headset and the gateway:

  .. code-block:: console

     python buildprog.py -c app -b debug -d both

* Example 2: The following command builds the application using the script for both the application and the network core (``both``).
  It builds with the ``release`` application version, with the DFU internal flash memory layout enabled, and using the minimal size of the network core bootloader:

  .. code-block:: console

     python buildprog.py -c both -b release -d both -m internal -M

  If you run this command with the ``external`` DFU type parameter instead of ``internal``, the external flash memory layout will be enabled.

The command can be run from any location, as long as the correct path to :file:`buildprog.py` is given.

The build files are saved in the :file:`applications/nrf5340_audio/build` directory.
The script creates a directory for each application version and device type combination.
For example, when running the command above, the script creates the :file:`dev_gateway/build_debug` and :file:`dev_headset/build_debug` directories.

Programming with the script
   The development kits are programmed according to the serial numbers set in the JSON file.
   Make sure to connect the development kits to your PC using USB and turn them on using the **POWER** switch before you run the command.

   The following parameters are available for programming:

   * Programming (``-p`` parameter) -- If you run the building script with this parameter, you can program one or both of the cores after building the files.
   * Sequential programming (``-s`` parameter) -- If you are using Windows Subsystem for Linux (WSL) and encounter problems while programming, include this parameter alongside other parameters to program sequentially.

   The command for programming can look as follows:

   .. code-block:: console

      python buildprog.py -c both -b debug -d both -p

   This command builds the application with the ``debug`` application version for both the headset and the gateway and programs the application core.
   Given the ``-c both`` parameter, it also takes the precompiled Bluetooth Low Energy Controller binary from the :file:`nrf/lib/bin/bt_ll_acs_nrf53/bin` directory and programs it to the network core of both the gateway and the headset.

   .. note::
      If the programming command fails because of :ref:`readback_protection_error`, run :file:`buildprog.py` with the ``--recover_on_fail`` or ``-f`` parameter to recover and re-program automatically when programming fails.
      For example, using the programming command example above:

      .. code-block:: console

         python buildprog.py -c both -b debug -d both -p --recover_on_fail

   If you want to program firmware that has DFU enabled, you must include the DFU parameters in the command.
   The command for programming with DFU enabled can look as follows:

   .. code-block:: console

     python buildprog.py -c both -b release -d both -m internal -M -p

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
===========================================

You can also build the nRF5340 Audio application using the standard |NCS| :ref:`build steps <programming_cmd>` for the command line.

.. note::
   Using this method requires you to build and program each development kit one at a time before moving to the next configuration, which can be time-consuming.
   :ref:`nrf53_audio_app_building_script` is recommended.

Building the application
------------------------

Complete the following steps to build the application:

1. Choose the combination of build flags:

   a. Choose the device type by using one of the following options:

      * For headset device: ``-DCONFIG_AUDIO_DEV=1``
      * For gateway device: ``-DCONFIG_AUDIO_DEV=2``

   #. Choose the application version by using one of the following options:

      * For the debug version: No build flag needed.
      * For the release version: ``-DCONF_FILE=prj_release.conf``

#. Build the application using the standard :ref:`build steps <programming_cmd>` for the command line.
   For example, if you want to build the firmware for the application core as a headset using the ``release`` application version, you can run the following command:

   .. code-block:: console

      west build -b nrf5340_audio_dk_nrf5340_cpuapp --pristine -- -DCONFIG_AUDIO_DEV=1 -DCONF_FILE=prj_release.conf

   Unlike when :ref:`nrf53_audio_app_building_script`, this command creates the build files directly in the :file:`build` directory.
   This means that you first need to program the headset development kits before you build and program gateway development kits.
   Alternatively, you can add the ``-d`` parameter to the ``west`` command to specify a custom build folder. This lets you build firmware for both
   headset and gateway before programming any development kits.

Programming the application
---------------------------

After building the files for the development kit you want to program, complete the following steps to program the application from the command line:

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


.. _nrf53_audio_app_testing:

Testing
=======

After building and programming the application, you can test it for both the CIS and the BIS modes.
The following testing scenarios assume you are using USB as the audio source on the gateway.
This is the default setting.

.. _nrf53_audio_app_testing_steps_cis:

Testing the default CIS mode
----------------------------

Complete the following steps to test the unidirectional CIS mode for one gateway and two headset devices:

1. Make sure that the development kits are still plugged into the USB ports and are turned on.

   .. note::
      |usb_known_issues|

   After programming, **RGB2** starts blinking green on every device to indicate the ongoing CPU activity on the network core.
   **LED3** starts blinking green on every device to indicate the ongoing CPU activity on the application core.
#. Wait for the **LED1** on the gateway to start blinking blue.
   This happens shortly after programming the development kit and indicates that the gateway device is connected to at least one headset and ready to send data.
#. Search the list of audio devices listed in the sound settings of your operating system for *nRF5340 USB Audio* (gateway) and select it as the output device.
#. Connect headphones to the **HEADPHONE** audio jack on both headset devices.
#. Start audio playback on your PC from any source.
#. Wait for **LED1** to blink blue on both headsets.
   When they do, the audio stream has started on both headsets.

   .. note::
      The audio outputs only to the left channel of the audio jack, even if the given headset is configured as the right headset.
      This is because of the mono hardware codec chip used on the development kits.
      If you want to play stereo sound using one development kit, you must connect an external hardware codec chip that supports stereo.

#. Wait for **LED2** to light up solid green on the headsets to indicate that the audio synchronization is achieved.
#. Press the **VOL+** button on one of the headsets.
   The playback volume increases for both headsets.
#. Press the **VOL-** button on the gateway.
   The playback volume decreases for both headsets.
#. Press the **PLAY/PAUSE** button on any one of the devices.
   The playback stops for both headsets and the streaming state for all devices is set to paused.
#. Press the **RESET** button on the gateway.
   The gateway resets and the playback on the unpaused headset stops.
   After some time, the gateway establishes the connection with both headsets and resumes the playback on the unpaused headset.
#. Press the **PLAY/PAUSE** button on any one of the devices.
   The playback resumes in both headsets.
#. Press the **BTN 4** button on the gateway multiple times.
   For each button press, the audio stream playback is stopped and the gateway sends a test tone to both headsets.
   These tones can be used as audio cues to check the synchronization of the headsets.
#. Hold down the **VOL+** button and press the **RESET** button on the left headset.
   After startup, this headset will be configured as the right channel headset.
#. Hold down the **VOL-** button and press the **RESET** button on the left headset.
   After startup, this headset will go back to be configured as the left channel headset.
   You can also just press the **RESET** button to restore the original programmed settings.

After the kits have paired for the first time, they are now bonded.
This means the Long-Term Key (LTK) is stored on each side, and that the kits will only connect to each other unless the bonding information is cleared.
To clear the bonding information, press and hold **BTN 5** during boot.

When you finish testing, power off the nRF5340 Audio development kits by switching the power switch from On to Off.

.. _nrf53_audio_app_testing_steps_bis:

Testing the BIS mode
--------------------

Testing the BIS mode is identical to `Testing the default CIS mode`_, except for the following differences:

* You must :ref:`select the BIS mode manually <nrf53_audio_app_configuration_select_bis>` before building the application.
* You can play the audio stream with different audio settings on the receivers.
  For example, you can decrease or increase the volume separately for each receiver during playback.
* When pressing the **PLAY/PAUSE** button on a headset, the streaming state only changes for that given headset.
* Pressing the **PLAY/PAUSE** button on the gateway will respectively start or stop the stream for all headsets listening in.
* Pressing the **BTN 4** button on a headset will change the active audio stream.
  The default configuration of the BIS mode supports two audio streams (left and right).
* Pressing the **BTN 5** button on a headset will change the gateway source for the audio stream (after `Enabling the BIS mode with two gateways`_).
  If a second gateway is not present, the headset will not play audio.

.. _nrf53_audio_app_testing_steps_cis_walkie_talkie:

Testing the walkie-talkie demo
------------------------------

Testing the walkie-talkie demo is identical to `Testing the default CIS mode`_, except for the following differences:

* You must enable the Kconfig option mentioned in `Enabling the walkie-talkie demo`_ before building the application.
* Instead of controlling the playback, you can speak through the PDM microphones.
  The line is open all the time, no need to press any buttons to talk, but the volume control works as in `Testing the default CIS mode`_.

.. _nrf53_audio_app_porting_guide:

Testing FOTA upgrades
---------------------

`nRF Connect Device Manager`_ can be used for testing FOTA upgrades.
The procedure for upgrading the firmware is identical for both headset and gateway firmware.
You can test upgrading the firmware on both cores at the same time on a headset device by completing the following steps:

1. Make sure you have :ref:`configured the application for FOTA <nrf53_audio_app_configuration_configure_fota>`.
#. Install `nRF Connect Device Manager`_ on your Android or iOS device.
#. Connect an external flash shield to the headset.
#. Make sure the headset runs a firmware that supports DFU using external flash memory.
   One way of doing this is to connect the headset to the USB port, turn it on, and then run this command:

   .. code-block:: console

      python buildprog.py -c both -b debug -d headset --pristine -m external -p

   .. note::
      When using the FOTA related functionality in the :file:`buildprog.py` script on Linux, the ``python`` command must execute Python 3.

#. Use the :file:`buildprog.py` script to create a zip file that contains new firmware for both cores:

   .. code-block:: console

      python buildprog.py -c both -b debug -d headset --pristine -m external

#. Transfer the generated file to your Android or iOS device, depending on the DFU scenario.
   See the `FOTA build files`_ section for information about FOTA file name patterns.
   For transfer, you can use cloud services like Google Drive for Android or iCloud for iOS.
#. Enter the DFU mode by pressing and holding down **RESET** and **BTN 4** at the same time, and then releasing **RESET** while continuing to hold down **BTN 4** for a couple more seconds.
#. Open `nRF Connect Device Manager`_ and look for ``NRF5340_AUDIO_HL_DFU`` in the scanned devices window.
   The headset is left by default.
#. Tap on :guilabel:`NRF5340_AUDIO_HL_DFU` and then on the downward arrow icon at the bottom of the screen.
#. In the :guilabel:`Firmware Upgrade` section, tap :guilabel:`SELECT FILE`.
#. Select the file you transferred to the device.
#. Tap :guilabel:`START` and check :guilabel:`Confirm only` in the notification.
#. Tap :guilabel:`START` again to start the DFU process.
#. When the DFU has finished, verify that the new application core and network core firmware works properly.

Dependencies
************

.. note::
   The following lists mention the most important dependencies.
   For the full list, check the application's Kconfig options.
   All dependencies are automatically included.

The application uses the following |NCS| components:

* :ref:`lib_bt_ll_acs_nrf53_readme`
* :ref:`lib_contin_array`
* :ref:`lib_pcm_mix`
* :ref:`lib_tone`

This application uses the following `nrfx`_ libraries:

* :file:`nrfx_clock.h`
* :file:`nrfx_gpiote.h`
* :file:`nrfx_timer.h`
* :file:`nrfx_dppi.h`
* :file:`nrfx_i2s.h`
* :file:`nrfx_ipc.h`
* :file:`nrfx_nvmc.h`

The application also depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
* :ref:`zephyr:zbus`
* :ref:`zephyr:api_peripherals`:

   * :ref:`zephyr:usb_api`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`

.. _nrf53_audio_app_options:
.. _config_nrf53_audio_app_options:

Application configuration options
*********************************

.. options-from-kconfig::
   :show-type:

.. |usb_known_issues| replace:: Make sure to check the :ref:`nRF5340 Audio application known issues <known_issues_nrf5340audio>` related to serial connection with the USB.

.. _nrf53_audio_app_building:

Building and running nRF5340 Audio applications
###############################################

.. contents::
   :local:
   :depth: 2

This nRF5340 Audio application source files can be found in their respective folders under :file:`applications/nrf5340_audio` in the nRF Connect SDK folder structure.

You can build and program the applications in one of the following ways:

* :ref:`nrf53_audio_app_building_script` - This is the suggested method.
  Using this method allows you to build and program multiple development kits at the same time.
* :ref:`nrf53_audio_app_building_standard` - Using this method requires building and programming each development kit separately.

.. note::
   Check the :ref:`nRF5340 Audio application known issues <known_issues_nrf5340audio>` before building and programming the applications.
   Some of them might impact running the application on certain platforms.

   For example, one of the known issues (OCT-2154) mentions intermittent audio stream on the headset when using USB audio interface on macOS.

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

The suggested method for building each of the applications and programming it to the development kit is running the :file:`buildprog.py` Python script.
The script automates the process of selecting :ref:`configuration files <nrf53_audio_app_configuration_files>` and building different applications.
This eases the process of building and programming images for multiple development kits.

The script is located in the :file:`applications/nrf5340_audio/tools/buildprog` directory.

  .. note::
     The :file:`buildprog.py` script is an app-specific script for building and programming multiple kits and cores with various audio application configurations. The script will be deprecated in a future release. The audio applications will gradually shift only to using standard tools for building and programming development kits.

Preparing the JSON file
=======================

The script depends on the settings defined in the :file:`nrf5340_audio_dk_devices.json` file.
Before using the script, make sure to update this file with the following information for each development kit you want to use.
This is how the file looks by default:

.. literalinclude:: ../tools/buildprog/nrf5340_audio_dk_devices.json
    :language: json

When preparing the JSON file, update the following fields:

* ``nrf5340_audio_dk_snr`` - This field lists the SEGGER serial number.
  You can check this ten-digit number on the sticker on the nRF5340 Audio development kit.
  Alternatively, connect the development kit to your PC and run ``nrfutil device list`` in a command window to print the SEGGER serial number of all connected kits.
* ``nrf5340_audio_dk_dev`` - This field assigns the specific nRF5340 Audio development kit to be ``headset`` or ``gateway``.
* ``location`` - This field is valid only for headsets.
  It sets the location on which the headset is meant to work, especially when using the :ref:`default CIS transport mode configuration <nrf53_audio_transport_mode_configuration>`.
  For more information, see :ref:`nrf53_audio_app_configuration_headset_location`.

.. _nrf53_audio_app_building_script_running:

Running the script
==================

The script handles building and parallel programming of multiple kits.
The following sections explain these two steps separately.

Script parameters for building
------------------------------

After editing the :file:`nrf5340_audio_dk_devices.json` file, run :file:`buildprog.py` to build the firmware for the development kits.
The building command for running the script requires providing the following parameters:

.. list-table:: Parameters for the script
   :header-rows: 1

   * - Parameter
     - Description
     - Options
     - More information
   * - Core type (``-c``)
     - Specifies the core type.
     - ``app``, ``net``, ``both``
     - :ref:`nrf53_audio_app_overview_architecture`
   * - Application version (``-b``)
     - Specifies the application version.
     - ``release``, ``debug``
     - | :ref:`nrf53_audio_app_configuration_files`
       | **Note:** For FOTA DFU, you must use :ref:`nrf53_audio_app_building_standard`.
   * - Transport type (``-t``)
     - Specifies the transport type.
     - ``broadcast``, ``unicast``
     - :ref:`nrf53_audio_app_overview_architecture`
   * - Device type (``-d``)
     - Specifies the device type.
     - ``headset``, ``gateway``, ``both``
     - :ref:`nrf53_audio_app_overview_gateway_headsets`

For example, the following command builds headset and gateway applications using the script for the application core with the ``debug`` application version:

.. code-block:: console

   python buildprog.py -c app -b debug -d both -t unicast

The command can be run from any location, as long as the correct path to :file:`buildprog.py` is given.

The build files are saved in separate subdirectories in the :file:`applications/nrf5340_audio/tools/build` directory.
The script creates a directory for each transport, device type, core, and version combination.
For example, when running the command above, the script creates the :file:`unicast/gateway/app/debug`, :file:`unicast/gateway/net/debug`, :file:`unicast/headset/app/debug`, :file:`unicast/headset/net/debug` files and directories.

Script parameters for programming
---------------------------------

The script can program the build files as part of the same `python buildprog.py` command used for building.
Use one of the following programming parameters:

* Programming (``-p`` parameter) - If you run the ``buildprog`` script with this parameter, you can program one or both of the cores after building the files.
* Sequential programming (``-s`` parameter) - If you encounter problems while programming, include this parameter alongside other parameters to program sequentially.

.. note::
    The development kits are programmed according to the serial numbers set in the JSON file.
    Make sure to connect the development kits to your PC using USB and turn them on using the **POWER** switch before you run the script with the programming parameter.

The command for programming can look as follows:

.. code-block:: console

   python buildprog.py -c both -b debug -d both -t unicast -p

This command builds the unicast headset and the gateway applications with ``debug`` version of both the application core binary and the network core binary - and programs each to its respective core.
If you want to rebuild from scratch, you can add the ``--pristine`` parameter to the command (west's ``-p`` for cannot be used for a pristine build with the script).

.. note::
   If the programming command fails because of a :ref:`readback protection error <readback_protection_error>`, run :file:`buildprog.py` with the ``--recover_on_fail`` or ``-f`` parameter to recover and re-program automatically when programming fails.
   For example, using the programming command example above:

   .. code-block:: console

      python buildprog.py -c both -b debug -d both -t unicast -p --recover_on_fail

Getting help
------------

Run ``python buildprog.py -h`` for information about all available script parameters.

Configuration table overview
----------------------------

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

Building and programming using standard methods
***********************************************

You can also build the nRF5340 Audio applications using the standard |NCS| :ref:`build steps <programming_cmd>` for |nRFVSC| or the command line.

.. _nrf53_audio_app_building_config_files:

Application configuration files
===============================

The application uses a :file:`prj.conf` configuration file located in the application root directory for the default configuration.
It also uses application-specific overlay files and can use additional files for different custom configurations.
When you build the application, you can select one of these configurations using the :makevar:`FILE_SUFFIX` variable.

See :ref:`nrf53_audio_app_configuration_files`, :ref:`app_build_file_suffixes`, and :ref:`cmake_options` for more information.

The nRF5340 Audio applications come with the following configuration files:

.. list-table:: Application configurations
   :widths: auto
   :header-rows: 1

   * - Configuration
     - File name
     - FILE_SUFFIX
     - Description
   * - Debug (default)
     - :file:`prj.conf`
     - No suffix
     - Debug version of the application. Provides full logging capabilities and debug optimizations to ease development.
   * - Release
     - :file:`prj_release.conf`
     - ``release``
     - Release version of the application. Disables logging capabilities and disables development features to create a smaller application binary.
   * - FOTA DFU
     - :file:`prj_fota.conf`
     - ``fota``
     - | Builds the debug version of the application (:file:`prj.conf`) with the features needed to perform DFU over Bluetooth LE, and includes bootloaders so that the applications on both the application core and network core can be updated.
       | See :ref:`nrf53_audio_app_fota` for more information.
   * - Application-specific overlay file
     - :file:`unicast_server/overlay-unicast_server.conf`
     - ``unicast_server``
     - Configuration file for the unicast server application.
   * - Application-specific overlay file
     - :file:`unicast_client/overlay-unicast_client.conf`
     - ``unicast_client``
     - Configuration file for the unicast client application.
   * - Application-specific overlay file
     - :file:`broadcast_sink/overlay-broadcast_sink.conf`
     - ``broadcast_sink``
     - Configuration file for the broadcast sink application.
   * - Application-specific overlay file
     - :file:`broadcast_source/overlay-broadcast_source.conf`
     - ``broadcast_source``
     - Configuration file for the broadcast source application.

.. _nrf53_audio_app_configuration_select_build:

Building the application
========================

Complete the following steps to build the application:

.. tabs::

   .. group-tab:: |nRFVSC|

      1. Open |nRFVSC|.
      #. Create a new audio application in |nRFVSC| by completing the steps listed in the :ref:`creating_vsc` section.
         Use the :guilabel:`Copy a sample` option and select the :guilabel:`nRF5340 Audio` application when prompted.
      #. Complete the steps listed on the `How to set up a build configuration`_ section in the |nRFVSC| documentation to create a build configuration.
         Depending on the configuration and applications you want to build, set the correct :ref:`nrf53_audio_app_building_config_files` in the extension UI:

         * :guilabel:`Base configuration files (Kconfig fragments)`:

           * For the debug version: No file selected.
           * For the release version: :file:`prj_release.conf`

         * :guilabel:`Extra Kconfig fragments`:

           * For the unicast server application: :file:`unicast_server/overlay-unicast_server.conf`
           * For the unicast client application: :file:`unicast_client/overlay-unicast_client.conf`
           * For the broadcast sink application: :file:`broadcast_sink/overlay-broadcast_sink.conf`
           * For the broadcast source application: :file:`broadcast_source/overlay-broadcast_source.conf`

         For other sections, check the tooltips in the extension UI.

      #. Make sure the :guilabel:`Generate only` checkbox is *not* selected.
      #. Generate and build the application by clicking the :guilabel:`Generate and Build` button.

      You can now program the application to one or more development kits.
      See :ref:`nrf53_audio_app_building_standard_programming`.

   .. group-tab:: Command line

      1. Choose the combination of build flags:

         a. Choose the configuration file for the device selected by using one of the following options:

            * For unicast headset: ``-DEXTRA_CONF_FILE=".\unicast_server\overlay-unicast_server.conf"``
            * For unicast gateway: ``-DEXTRA_CONF_FILE=".\unicast_client\overlay-unicast_client.conf"``
            * For broadcast headset: ``-DEXTRA_CONF_FILE=".\broadcast_sink\overlay-broadcast_sink.conf"``
            * For broadcast gateway: ``-DEXTRA_CONF_FILE=".\broadcast_source\overlay-broadcast_source.conf"``

         #. Choose the application version (:ref:`nrf53_audio_app_building_config_files`) by using one of the following options:

            * For the debug version: No build flag needed.
            * For the release version: ``-DFILE_SUFFIX=release``

      #. Build the application using the standard :ref:`build steps <building>` for the command line.
         For example, if you want to build the firmware for the application core as a headset using the ``release`` application version, you can run the following command from the :file:`applications/nrf5340_audio/` directory:

         .. code-block:: console

            west build -b nrf5340_audio_dk/nrf5340/cpuapp --pristine -- -DEXTRA_CONF_FILE=".\unicast_server\overlay-unicast_server.conf" -DFILE_SUFFIX=release

         This command creates the build files for a unicast headset device directly in the :file:`build` directory.
         What this means is that you cannot create build files for all devices you want to program, because the subsequent commands will overwrite the files in the :file:`build` directory.

         To work around this standard west behavior, you can add the ``-d`` parameter to the ``west`` command to specify a custom build folder for each device.
         This way, you can build firmware for headset and gateway to separate directories before programming the development kits.
         Alternatively, you can use the :ref:`nrf53_audio_app_building_script`, which handles this automatically.

Building the application for FOTA
---------------------------------

To build application for :ref:`nrf53_audio_app_fota` depending on the environment you are using:

.. tabs::

   .. group-tab:: |nRFVSC|

      Set the following options in the extension UI:

      * :guilabel:`Base configuration files (Kconfig fragments)` - :file:`prj_fota.conf`
      * :guilabel:`Extra Kconfig fragments` - :file:`unicast_server/overlay-unicast_server.conf`

      When generating and building the application, this selection picks :file:`prj_fota.conf` instead of the default :file:`prj.conf`.

   .. group-tab:: Command line

      Use the following command:

      .. code-block:: console

         west build -b nrf5340_audio_dk/nrf5340/cpuapp --pristine -- -DEXTRA_CONF_FILE=".\unicast_server\overlay-unicast_server.conf" -DFILE_SUFFIX=fota

      The command uses ``-DFILE_SUFFIX=fota`` to pick :file:`prj_fota.conf` instead of the default :file:`prj.conf`.
      It also uses the ``--pristine`` to clean the existing directory before starting the build process.

.. _nrf53_audio_app_building_standard_programming:

Programming the application
===========================

After building the files for the development kit you want to program, follow the :ref:`standard procedure for programming applications <programming>` in the |NCS|.

If you want to program the same audio application to multiple development kits connected to your PC, you can use the following methods:

* If you are programming using |nRFVSC|, you can select several development kits for programming by selecting them in the extension picker that appears after clicking the :guilabel:`Flash` action.
  See `How to work with boards and devices`_ in the |nRFVSC| documentation for more information.
* If you are programming using the command line, you need to program the application to each development kit using separate commands.

Programming CIS transport mode with two headsets or stereo
----------------------------------------------------------

When using the :ref:`default CIS transport mode configuration <nrf53_audio_transport_mode_configuration>`, if you want to use two headset devices or the stereo configuration, you must :ref:`configure the headset location <nrf53_audio_app_configuration_headset_location>`.
Use the combined bitfield values, depending on which headset you want to configure:

* Two headsets, left and right:

  * Left headset (``--value 1``):

    .. code-block:: console

       nrfutil device x-write --address 0x00FF80F4 --value 1

  * Right headset (``--value 2``):

    .. code-block:: console

       nrfutil device x-write --address 0x00FF80F4 --value 2

* One stereo headset (``--value 3``):

  .. code-block:: console

     nrfutil device x-write --address 0x00FF80F4 --value 3

Select the correct board when prompted with the popup.
Alternatively, you can add the ``--serial-number`` parameter followed by the SEGGER serial number of the correct board at the end of the ``nrfutil device`` command.
You can check the serial numbers of the connected devices with the ``nrfutil device list`` command.

.. note::
   |usb_known_issues|

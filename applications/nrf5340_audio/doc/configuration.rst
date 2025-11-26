.. _nrf53_audio_app_configuration:

Configuring the nRF5340 Audio applications
##########################################

.. contents::
   :local:
   :depth: 2


The nRF5340 Audio applications introduce Kconfig options that you can use to configure audio functionality, device roles, and transport modes.
You can use these options to select between gateway and headset roles, choose between CIS (unicast) and BIS (broadcast) transport modes, and configure audio-specific features.

The nRF5340 Audio applications align the configuration with the nRF5340 Audio use case by overlaying Kconfig defaults and selecting or implying the required Kconfig options.
Among others, the Kconfig options for Bluetooth, Zephyr's audio subsystems, and hardware peripherals are selected to ensure they are enabled.

The :option:`CONFIG_NRF5340_AUDIO` option is the main Kconfig option that activates all nRF5340 Audio functionality.
See the :file:`Kconfig.defaults` file in the :file:`nrf5340_audio` directory for details related to the default common configuration.

.. note::
   Part of the default configuration is applied by modifying the default values of Kconfig options.
   Changing configuration in menuconfig does not automatically adjust user-configurable values to the new defaults.
   So, you must update those values manually.
   For more information, see the Stuck symbols in menuconfig and guiconfig section on the :ref:`kconfig_tips_and_tricks` in the Zephyr documentation.

   The default Kconfig option values are automatically updated if configuration changes are applied directly in the configuration files.

The application-specific Kconfig options mentioned on this page are listed in :ref:`nRF5340 Audio: Application-specific Kconfig options <config_audio_app_options>`.

|config|

.. _nrf53_audio_device_role_configuration:

Configuring the device role
***************************

The nRF5340 Audio application supports two device roles: gateway and headset.
See :ref:`nrf53_audio_app_overview_gateway_headsets` for more information about these roles.

You can :ref:`select gateway or headset build <nrf53_audio_app_configuration_select_build>` when :ref:`nrf53_audio_app_building`.

The :option:`CONFIG_AUDIO_DEV` Kconfig option defines the nRF5340 Audio device role.
The device role can be either a gateway (:option:`CONFIG_AUDIO_DEV` set to ``2``) or a headset (:option:`CONFIG_AUDIO_DEV` set to ``1``).

Each role automatically implies the nRF5340 Audio modules needed for the role.
For example, the gateway role automatically enables USB audio source support, while the headset role enables I2S audio output.

.. _nrf53_audio_transport_mode_configuration:

Configuring the transport mode
******************************

The nRF5340 Audio application supports two transport modes for LE Audio:

* Connected Isochronous Stream (CIS) - for unicast communication
* Broadcast Isochronous Stream (BIS) - for broadcast communication

See :ref:`nrf53_audio_app_overview_modes` for detailed information about these modes.

The transport mode is selected using the following Kconfig options:

* (Default mode) :option:`CONFIG_TRANSPORT_CIS` - Enables CIS mode for clients and servers (unicast applications).
  With this option enabled, you can configure application Kconfig options specific to unicast communication (see :file:`applications/nrf5340_audio/src/bluetooth/bt_stream/unicast/Kconfig`).
* :option:`CONFIG_TRANSPORT_BIS` - Enables BIS mode for `Auracast™`_ sources and sinks (broadcast applications).
  With this option enabled, you can configure application Kconfig options specific to broadcast communication (see :file:`applications/nrf5340_audio/src/bluetooth/bt_stream/broadcast/Kconfig`).

The transport mode selection automatically configures the appropriate Bluetooth stack components and audio processing modules.

.. _nrf53_audio_app_configuration_select_bidirectional:

Selecting the CIS bidirectional communication
=============================================

To switch to the bidirectional mode, set the :option:`CONFIG_STREAM_BIDIRECTIONAL` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file (for the debug version) or in the :file:`applications/nrf5340_audio/prj_release.conf` file (for the release version).

.. _nrf53_audio_app_configuration_enable_walkie_talkie:

Enabling the walkie-talkie demo
-------------------------------

The walkie-talkie demo uses one or two bidirectional streams from the gateway to one or two headsets.
The PDM microphone is used as input on both the gateway and headset device.
To switch to using the walkie-talkie, set the :option:`CONFIG_WALKIE_TALKIE_DEMO` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file (for the debug version) or in the :file:`applications/nrf5340_audio/prj_release.conf` file (for the release version).

Enabling the Auracast™ (broadcast) mode
=======================================

If you want to work with `Auracast™`_ (broadcast) sources and sinks, set the :option:`CONFIG_TRANSPORT_BIS` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file.

.. _nrf53_audio_app_configuration_select_bis_two_gateways:

Enabling the BIS mode with two gateways
=======================================

In addition to the standard BIS mode with one gateway, you can also add a second gateway device.
The BIS headsets can then switch between the two gateways and receive the audio stream from one of the two gateways.

To configure the second gateway, add both the :option:`CONFIG_TRANSPORT_BIS` and the :option:`CONFIG_BT_AUDIO_USE_BROADCAST_NAME_ALT` Kconfig options set to ``y`` to the :file:`applications/nrf5340_audio/prj.conf` file for the debug version and to the :file:`applications/nrf5340_audio/prj_release.conf` file for the release version.
You can provide an alternative name to the second gateway using the :option:`CONFIG_BT_AUDIO_BROADCAST_NAME_ALT` or use the default alternative name.

You build each BIS gateway separately using the normal procedures from :ref:`nrf53_audio_app_building`.
After building the first gateway, configure the required Kconfig options for the second gateway and build the second gateway firmware.
Remember to program the two firmware versions to two separate gateway devices.

.. _nrf53_audio_app_configuration_headset_location:

Configuring the headset location
================================

When using the :ref:`default CIS transport mode configuration <nrf53_audio_transport_mode_configuration>`, if you want to use two headset devices or the stereo configuration, you must also define the correct headset location.

The nRF5340 Audio applications use audio location definitions from the Audio Location Definition chapter in the `Bluetooth Assigned Numbers`_ specification.
These correspond to the bitfields in the :file:`bt_audio_location` enum in the :file:`zephyr/include/zephyr/bluetooth/assigned_numbers.h` file.
When building the audio application, the location value is used to populate the UICR with the correct bitfield for each headset.

You can set the location for each headset in the following ways, depending on the building and programming method:

* When :ref:`nrf53_audio_app_building_script`, set the location for each headset in the :file:`nrf5340_audio_dk_devices.json` file.
  Use the location labels from the Audio Location Definition chapter in the `Bluetooth Assigned Numbers`_ specification.
  For example:

  .. code-block:: json

     [
      {
        "nrf5340_audio_dk_snr": 1000,
        "nrf5340_audio_dk_dev": "headset",
        "location": ["FRONT_LEFT"]
      }
     ]

* When :ref:`nrf53_audio_app_building_standard`, set the location for each headset when running the :ref:`programming command <nrf53_audio_app_building_standard_programming>`.
  Use the combined bitfield values from the :file:`zephyr/include/zephyr/bluetooth/assigned_numbers.h` file to define the headset location.
  For example, if you want to use the stereo configuration, use the ``0x3`` value, which is the combined bitfield value of the left (``0x01``) and right (``0x02``) location:

  .. code-block:: console

     nrfutil device x-write --address 0x00FF80F4 --value 3

The following table lists some of the available locations and their bitfield values:

.. list-table:: Example audio locations and their bitfield values
   :header-rows: 1

   * - Audio location
     - Value from specification
     - Bitfield value
   * - Mono Audio
     - ``0x00000000``
     - n/a
   * - ``"FRONT_LEFT"``
     - ``0x00000001``
     - ``0``
   * - ``"FRONT_RIGHT"``
     - ``0x00000002``
     - ``1``
   * - Stereo
     - n/a
     - ``3`` (``0`` and ``1`` set)

.. _nrf53_audio_source_configuration:

Configuring the audio source
****************************

The nRF5340 Audio application supports multiple audio sources for gateway devices.
See :ref:`nrf53_audio_app_overview_architecture_usb` and :ref:`nrf53_audio_app_overview_architecture_i2s` for information about the firmware architecture differences.

The audio source is selected using the following Kconfig options:

* USB audio source (:option:`CONFIG_AUDIO_SOURCE_USB`) - Uses USB as the audio source (default for gateway)
* I2S audio source (:option:`CONFIG_AUDIO_SOURCE_I2S`) - Uses 3.5 mm jack analog input using I2S

In the default configuration, the gateway application uses USB as the audio source.
The :ref:`nrf53_audio_app_building` and testing steps also refer to using the USB serial connection.

The audio source selection affects the firmware architecture and available features.
USB audio source is limited to unidirectional streams due to CPU load considerations, while I2S supports bidirectional communication.

.. _nrf53_audio_app_configuration_select_i2s:

Selecting the analog jack input using I2S
=========================================

To switch to using the 3.5 mm jack analog input, set the :option:`CONFIG_AUDIO_SOURCE_I2S` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file for the debug version and in the :file:`applications/nrf5340_audio/prj_release.conf` file for the release version.

When testing the application, an additional audio jack cable is required to use I2S.
Use this cable to connect the audio source (PC) to the analog **LINE IN** on the development kit.

.. _nrf53_audio_codec_configuration:

Configuring codecs
******************

The nRF5340 Audio application uses both software and hardware codecs.
The software codec is responsible for encoding and decoding, while the hardware codec is responsible for DAC/ADC and other audio processing.
See :ref:`nrf53_audio_app_overview_architecture` for information about how both codecs are integrated into the firmware architecture.

You can enable the software codec using the :option:`CONFIG_SW_CODEC_LC3` Kconfig option.
This codec is mandatory for LE Audio.

You can enable the CS47L63 hardware codec using the :option:`CONFIG_NRF5340_AUDIO_CS47L63_DRIVER` Kconfig option.

The codec selection affects audio quality, processing requirements, and power consumption.

.. _nrf53_audio_quality_configuration:

Configuring audio quality
*************************

The nRF5340 Audio application provides extensive configuration options for audio quality.
These settings affect the :ref:`nrf53_audio_app_overview_architecture_sync_module` and overall audio performance.

See :ref:`config_audio_app_options` for the list of options to configure the following audio quality settings:

* Frame duration (example: :option:`CONFIG_AUDIO_FRAME_DURATION_10_MS`)
* Sample rates (example: :option:`CONFIG_AUDIO_SAMPLE_RATE_16000_HZ`)
* Bit depth (example: :option:`CONFIG_AUDIO_BIT_DEPTH_16`)
* Presentation delay (example: :option:`CONFIG_AUDIO_MIN_PRES_DLY_US`)

.. _nrf53_audio_bluetooth_configuration:

Configuring Bluetooth LE Audio
******************************

The nRF5340 Audio application introduces application-specific configuration options related to Bluetooth LE Audio.
These options configure the Bluetooth stack components described in :ref:`nrf53_audio_app_overview_architecture`.

See :ref:`config_audio_app_options` for options starting with ``CONFIG_BT_AUDIO``.

.. _nrf53_audio_app_configuration_power_measurements:

Configuring power measurements
******************************

The power measurements are disabled by default in the :ref:`debug version <nrf53_audio_app_overview_files>` of the application.

.. note::
   Enabling power measurements in the debug version together with :ref:`debug logging <ug_logging>` increases the power consumption compared with the release version of the application.
   For better results, consider using the `Power Profiler Kit II (PPK2)`_ and the `Power Profiler app`_ from nRF Connect for Desktop to measure the power consumption.

To enable power measurements in the debug version, set the :kconfig:option:`CONFIG_NRF5340_AUDIO_POWER_MEASUREMENT` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file.

.. _nrf53_audio_app_configuration_sd_card_playback:

Enabling SD card playback
*************************

The SD Card Playback module allows you to play audio files directly from an SD card inserted into the nRF5340 Audio development kit.
This feature supports both WAV and LC3 audio file formats and is compatible with all nRF5340 Audio applications.
See :ref:`nrf53_audio_app_overview_architecture_sd_card_playback` for detailed information about the SD card playback module.

File format support requirements
================================

The SD card playback module supports both WAV and LC3 audio file formats.
The audio files must meet the following requirements:

* WAV files must be 48 kHz, 16-bit, mono PCM format.
* LC3 files must be in the LC3 file format with proper headers.

SD card requirements
====================

Make sure the SD card meets the following requirements:

* Formatted with FAT32 or exFAT file system.
* Audio files are placed in the root directory or subdirectories.

Configuring SD card playback
============================

To enable SD card playback functionality, you need to set the following Kconfig options to ``y``:

* :option:`CONFIG_NRF5340_AUDIO_SD_CARD_MODULE` - to enable the SD card module; this option is enabled by default on nRF5340 Audio DK
* :option:`CONFIG_SD_CARD_PLAYBACK` - to enable the playback functionality

Optionally, you can also set the following Kconfig options:

* :option:`CONFIG_SD_CARD_PLAYBACK_STACK_SIZE`
* :option:`CONFIG_SD_CARD_PLAYBACK_RING_BUF_SIZE`
* :option:`CONFIG_SD_CARD_PLAYBACK_THREAD_PRIO`

Shell commands for SD card playback
===================================

When SD card playback is enabled, the following shell commands are available:

.. list-table:: SD card playback shell commands
   :header-rows: 1

   * - Command
     - Description
   * - ``sd_card_playback play_wav <filename>.wav``
     - Play a WAV file from the SD card
   * - ``sd_card_playback play_lc3 <filename>.lc3``
     - Play an LC3 file from the SD card
   * - ``sd_card_playback list_files``
     - List files in the current directory
   * - ``sd_card_playback cd <directory>``
     - Change to a different directory
   * - ``sd_card_playback cd /``
     - Return to the root directory

To issue these commands, you can use the RTT or UART serial connection.

Example usage
=============

To play audio from the SD card, complete the following steps:

1. Configure the SD card playback module in your application as described in `Configuring SD card playback`_.
#. :ref:`Build and run the application <nrf53_audio_app_building>`.
#. Insert a properly formatted SD card with audio files into the development kit.
#. Connect to the device using the RTT or UART serial connection.
   For example, you can use the `Serial Terminal app`_ to connect to the device.
#. In the terminal:

   a. Issue the following command to list files on the SD card:

      .. code-block:: console

         sd_card_playback list_files

   b. Issue the following command to play a WAV file:

      .. code-block:: console

         sd_card_playback play_wav <filename>.wav

   c. Issue the following command to play an LC3 file:

      .. code-block:: console

         sd_card_playback play_lc3 <filename>.lc3

   The audio from the SD card will be mixed with any existing audio stream and played through the device's audio output.
#. To stop the playback, issue the ``sd_card_playback stop`` command.
#. To exit the shell, issue the ``exit`` command.

.. _nrf53_audio_app_adding_FEM_support:

Adding FEM support
******************

You can add support for the nRF21540 front-end module (FEM) to the following nRF5340 Audio applications:

* :ref:`Broadcast source <nrf53_audio_broadcast_source_app>`
* :ref:`Unicast client <nrf53_audio_unicast_client_app>`
* :ref:`Unicast server <nrf53_audio_unicast_server_app>`

The :ref:`broadcast sink application <nrf53_audio_broadcast_sink_app>` does not need FEM support as it only receives data.

Adding FEM support happens when :ref:`nrf53_audio_app_building`.
You can use one of the following options, depending on how you decide to build the application:

* If you opt for :ref:`nrf53_audio_app_building_script`, add the ``--nrf21540`` to the script's building command.
* If you opt for :ref:`nrf53_audio_app_building_standard`, add the ``-Dnrf5340_audio_SHIELD=nrf21540ek -Dipc_radio_SHIELD=nrf21540ek`` to the ``west build`` command.
  For example:

  .. code-block:: console

     west build -b nrf5340_audio_dk/nrf5340/cpuapp --pristine -- -DEXTRA_CONF_FILE=".\unicast_server\overlay-unicast_server.conf" -Dnrf5340_audio_SHIELD=nrf21540ek -Dipc_radio_SHIELD=nrf21540ek

To set the TX power output, use the :kconfig:option:`CONFIG_BT_CTLR_TX_PWR_ANTENNA` and :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB` Kconfig options in :file:`applications/nrf5340_audio/sysbuild/ipc_radio/prj.conf`.

See :ref:`ug_radio_fem` for more information about FEM in the |NCS|.

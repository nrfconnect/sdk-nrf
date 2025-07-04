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

The :ref:`CONFIG_NRF5340_AUDIO <nrf53_audio_app_configuration_options>` option is the main Kconfig option that activates all nRF5340 Audio functionality.
See the :file:`Kconfig.defaults` file for details related to the default common configuration.

.. note::
   Part of the default configuration is applied by modifying the default values of Kconfig options.
   Changing configuration in menuconfig does not automatically adjust user-configurable values to the new defaults.
   So, you must update those values manually.
   For more information, see the Stuck symbols in menuconfig and guiconfig section on the :ref:`kconfig_tips_and_tricks` in the Zephyr documentation.

   The default Kconfig option values are automatically updated if configuration changes are applied directly in the configuration files.

The application-specific Kconfig options mentioned on this page are listed in :ref:`nrf53_audio_app_configuration_options`.

|config|

.. _nrf53_audio_device_role_configuration:

Configuring the device role
***************************

The nRF5340 Audio application supports two device roles: gateway and headset.
See :ref:`nrf53_audio_app_overview_gateway_headsets` for more information about these roles.

You can :ref:`select gateway or headset build <nrf53_audio_app_configuration_select_build>` when :ref:`nrf53_audio_app_building`.

The :ref:`CONFIG_AUDIO_DEV <nrf53_audio_app_configuration_options>` Kconfig option defines the nRF5340 Audio device role.
The device role can be either a gateway (:ref:`CONFIG_AUDIO_DEV=2 <nrf53_audio_app_configuration_options>`) or a headset (:ref:`CONFIG_AUDIO_DEV=1 <nrf53_audio_app_configuration_options>`).

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

* (Default mode) :ref:`CONFIG_TRANSPORT_CIS <nrf53_audio_app_configuration_options>` - Enables CIS mode for clients and servers (unicast applications)
* :ref:`CONFIG_TRANSPORT_BIS <nrf53_audio_app_configuration_options>` - Enables BIS mode for `Auracast™`_ sources and sinks (broadcast applications)

The transport mode selection automatically configures the appropriate Bluetooth stack components and audio processing modules.

.. _nrf53_audio_app_configuration_select_bidirectional:

Selecting the CIS bidirectional communication
=============================================

To switch to the bidirectional mode, set the :ref:`CONFIG_STREAM_BIDIRECTIONAL<nrf53_audio_app_configuration_options>` Kconfig option to ``y``  in the :file:`applications/nrf5340_audio/prj.conf` file (for the debug version) or in the :file:`applications/nrf5340_audio/prj_release.conf` file (for the release version).

.. _nrf53_audio_app_configuration_enable_walkie_talkie:

Enabling the walkie-talkie demo
-------------------------------

The walkie-talkie demo uses one or two bidirectional streams from the gateway to one or two headsets.
The PDM microphone is used as input on both the gateway and headset device.
To switch to using the walkie-talkie, set the :ref:`CONFIG_WALKIE_TALKIE_DEMO<nrf53_audio_app_configuration_options>` Kconfig option to ``y``  in the :file:`applications/nrf5340_audio/prj.conf` file (for the debug version) or in the :file:`applications/nrf5340_audio/prj_release.conf` file (for the release version).

Enabling the Auracast™ (broadcast) mode
=======================================

If you want to work with `Auracast™`_ (broadcast) sources and sinks, set the :kconfig:option:`CONFIG_TRANSPORT_BIS` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file.

.. _nrf53_audio_app_configuration_select_bis_two_gateways:

Enabling the BIS mode with two gateways
=======================================

In addition to the standard BIS mode with one gateway, you can also add a second gateway device.
The BIS headsets can then switch between the two gateways and receive audio stream from one of the two gateways.

To configure the second gateway, add both the :ref:`CONFIG_TRANSPORT_BIS<nrf53_audio_app_configuration_options>` and the :ref:`CONFIG_BT_AUDIO_USE_BROADCAST_NAME_ALT<nrf53_audio_app_configuration_options>` Kconfig options set to ``y`` to the :file:`applications/nrf5340_audio/prj.conf` file for the debug version and to the :file:`applications/nrf5340_audio/prj_release.conf` file for the release version.
You can provide an alternative name to the second gateway using the :ref:`CONFIG_BT_AUDIO_BROADCAST_NAME_ALT<nrf53_audio_app_configuration_options>` or use the default alternative name.

You build each BIS gateway separately using the normal procedures from :ref:`nrf53_audio_app_building`.
After building the first gateway, configure the required Kconfig options for the second gateway and build the second gateway firmware.
Remember to program the two firmware versions to two separate gateway devices.

.. _nrf53_audio_source_configuration:

Configuring the audio source
****************************

The nRF5340 Audio application supports multiple audio sources for gateway devices.
See :ref:`nrf53_audio_app_overview_architecture_usb` and :ref:`nrf53_audio_app_overview_architecture_i2s` for information about the firmware architecture differences.

* USB audio source (:ref:`CONFIG_AUDIO_SOURCE_USB <nrf53_audio_app_configuration_options>`) - Uses USB as the audio source (default for gateway)
* I2S audio source (:ref:`CONFIG_AUDIO_SOURCE_I2S <nrf53_audio_app_configuration_options>`) - Uses 3.5 mm jack analog input using I2S

In the default configuration, the gateway application uses USB as the audio source.
The :ref:`nrf53_audio_app_building` and the testing steps also refer to using the USB serial connection.

The audio source selection affects the firmware architecture and available features.
USB audio source is limited to unidirectional streams due to CPU load considerations, while I2S supports bidirectional communication.

.. _nrf53_audio_app_configuration_select_i2s:

Selecting the analog jack input using I2S
=========================================

To switch to using the 3.5 mm jack analog input, set the :ref:`CONFIG_AUDIO_SOURCE_I2S<nrf53_audio_app_configuration_options>` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file for the debug version and in the :file:`applications/nrf5340_audio/prj_release.conf` file for the release version.

When testing the application, an additional audio jack cable is required to use I2S.
Use this cable to connect the audio source (PC) to the analog **LINE IN** on the development kit.

.. _nrf53_audio_codec_configuration:

Configuring codecs
******************

The nRF5340 Audio application supports both software and hardware codecs.
See :ref:`nrf53_audio_app_overview_architecture` for information about how codecs are integrated into the firmware architecture.

Software codec options
* :ref:`CONFIG_SW_CODEC_LC3 <nrf53_audio_app_configuration_options>` - LC3 codec (mandatory for LE Audio)
* :ref:`CONFIG_SW_CODEC_NONE <nrf53_audio_app_configuration_options>` - No software codec

Hardware codec options
* :ref:`CONFIG_NRF5340_AUDIO_CS47L63_DRIVER <nrf53_audio_app_configuration_options>` - CS47L63 hardware codec driver

The codec selection affects audio quality, processing requirements, and power consumption.

.. _nrf53_audio_quality_configuration:

Configuring audio quality
*************************

The nRF5340 Audio application provides extensive configuration options for audio quality.
These settings affect the :ref:`nrf53_audio_app_overview_architecture_sync_module` and overall audio performance.

Frame duration
* :ref:`CONFIG_AUDIO_FRAME_DURATION_7_5_MS <nrf53_audio_app_configuration_options>` - 7.5 ms frame duration
* :ref:`CONFIG_AUDIO_FRAME_DURATION_10_MS <nrf53_audio_app_configuration_options>` - 10 ms frame duration (default)

Sample rates
* :ref:`CONFIG_AUDIO_SAMPLE_RATE_16000_HZ <nrf53_audio_app_configuration_options>` - 16 kHz sample rate
* :ref:`CONFIG_AUDIO_SAMPLE_RATE_24000_HZ <nrf53_audio_app_configuration_options>` - 24 kHz sample rate
* :ref:`CONFIG_AUDIO_SAMPLE_RATE_48000_HZ <nrf53_audio_app_configuration_options>` - 48 kHz sample rate (default)

Bit depth
* :ref:`CONFIG_AUDIO_BIT_DEPTH_16 <nrf53_audio_app_configuration_options>` - 16-bit audio (default)
* :ref:`CONFIG_AUDIO_BIT_DEPTH_32 <nrf53_audio_app_configuration_options>` - 32-bit audio

Presentation delay
* :ref:`CONFIG_AUDIO_MIN_PRES_DLY_US <nrf53_audio_app_configuration_options>` - Minimum presentation delay
* :ref:`CONFIG_AUDIO_MAX_PRES_DLY_US <nrf53_audio_app_configuration_options>` - Maximum presentation delay

.. _nrf53_audio_bluetooth_configuration:

Configuring Bluetooth LE Audio
******************************

The nRF5340 Audio application introduces application-specific configuration options related to Bluetooth LE Audio.
These options configure the Bluetooth stack components described in :ref:`nrf53_audio_app_overview_architecture`.

Bluetooth LE Audio preferences
* :ref:`CONFIG_BT_AUDIO_PACKING_INTERLEAVED <nrf53_audio_app_configuration_options>` - Interleaved packing for ISO channels
* :ref:`CONFIG_BT_AUDIO_PREF_SAMPLE_RATE_48KHZ <nrf53_audio_app_configuration_options>` - Preferred 48-kHz sample rate
* :ref:`CONFIG_BT_AUDIO_PREF_SAMPLE_RATE_24KHZ <nrf53_audio_app_configuration_options>` - Preferred 24-kHz sample rate
* :ref:`CONFIG_BT_AUDIO_PREF_SAMPLE_RATE_16KHZ <nrf53_audio_app_configuration_options>` - Preferred 16-kHz sample rate

Quality of Service (QoS)
* :ref:`CONFIG_BT_AUDIO_PRESENTATION_DELAY_US <nrf53_audio_app_configuration_options>` - Presentation delay configuration
* :ref:`CONFIG_BT_AUDIO_MAX_TRANSPORT_LATENCY_MS <nrf53_audio_app_configuration_options>` - Max transport latency
* :ref:`CONFIG_BT_AUDIO_RETRANSMITS <nrf53_audio_app_configuration_options>` - Number of re-transmits

Broadcast-specific options
* :ref:`CONFIG_BT_AUDIO_USE_BROADCAST_NAME_ALT <nrf53_audio_app_configuration_options>` - Use alternative broadcast name
* :ref:`CONFIG_BT_AUDIO_BROADCAST_NAME_ALT <nrf53_audio_app_configuration_options>` - Alternative broadcast name

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

* :ref:`CONFIG_NRF5340_AUDIO_SD_CARD_MODULE<nrf53_audio_app_configuration_options>` - to enable the SD card module; this option is enabled by default on nRF5340 Audio DK
* :ref:`CONFIG_SD_CARD_PLAYBACK<nrf53_audio_app_configuration_options>` - to enable the playback functionality

Optionally, you can also set the following Kconfig options:

* :ref:`CONFIG_SD_CARD_PLAYBACK_STACK_SIZE<nrf53_audio_app_configuration_options>`
* :ref:`CONFIG_SD_CARD_PLAYBACK_RING_BUF_SIZE<nrf53_audio_app_configuration_options>`
* :ref:`CONFIG_SD_CARD_PLAYBACK_THREAD_PRIO<nrf53_audio_app_configuration_options>`

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

.. _nrf53_audio_other_configuration:

Other configuration
*******************

The following sections describe other application-specific configuration options.

.. _nrf53_audio_power_measurement_configuration:

Configuring power measurement
=============================

You can configure power measurement in the nRF5340 Audio application using the following Kconfig options:

* :ref:`CONFIG_NRF5340_AUDIO_POWER_MEASUREMENT <nrf53_audio_app_configuration_options>` - Power measurement functionality
* :ref:`CONFIG_POWER_MEAS_INTERVAL_MS <nrf53_audio_app_configuration_options>` - Power measurement interval
* :ref:`CONFIG_POWER_MEAS_START_ON_BOOT <nrf53_audio_app_configuration_options>` - Auto-start power measurements

These options enable monitoring of power consumption for different audio configurations and use cases.

.. _nrf53_audio_thread_configuration:

Configuring thread priorities and stack sizes
=============================================

You can configure thread priorities and stack sizes for different modules in the nRF5340 Audio application using the following Kconfig options:

Power measurement
* :ref:`CONFIG_POWER_MEAS_THREAD_PRIO <nrf53_audio_app_configuration_options>` - Power measurement thread priority
* :ref:`CONFIG_POWER_MEAS_STACK_SIZE <nrf53_audio_app_configuration_options>` - Power measurement stack size

Button handling
* :ref:`CONFIG_BUTTON_PUBLISH_THREAD_PRIO <nrf53_audio_app_configuration_options>` - Button publish thread priority
* :ref:`CONFIG_BUTTON_PUBLISH_STACK_SIZE <nrf53_audio_app_configuration_options>` - Button publish stack size
* :ref:`CONFIG_BUTTON_DEBOUNCE_MS <nrf53_audio_app_configuration_options>` - Button debounce time

Volume control
* :ref:`CONFIG_VOLUME_MSG_SUB_THREAD_PRIO <nrf53_audio_app_configuration_options>` - Volume message subscribe thread priority
* :ref:`CONFIG_VOLUME_MSG_SUB_STACK_SIZE <nrf53_audio_app_configuration_options>` - Volume message subscribe stack size
* :ref:`CONFIG_VOLUME_MSG_SUB_QUEUE_SIZE <nrf53_audio_app_configuration_options>` - Volume message subscriber queue size

.. _nrf53_audio_fifo_configuration:

Configuring the FIFO buffers
============================

You can configure the FIFO buffers for audio data management in the nRF5340 Audio application using the following Kconfig options:

* :ref:`CONFIG_FIFO_FRAME_SPLIT_NUM <nrf53_audio_app_configuration_options>` - Number of blocks per audio frame
* :ref:`CONFIG_FIFO_TX_FRAME_COUNT <nrf53_audio_app_configuration_options>` - Max TX audio frames
* :ref:`CONFIG_FIFO_RX_FRAME_COUNT <nrf53_audio_app_configuration_options>` - Max RX audio frames

These options affect audio latency and buffer management for different audio configurations.

.. _nrf53_audio_testing_configuration:

Configuring testing
===================

You can configure testing in the nRF5340 Audio application using the following Kconfig options:

* :ref:`CONFIG_TESTING_BLE_ADDRESS_RANDOM <nrf53_audio_app_configuration_options>` - Random address generation for testing
* :ref:`CONFIG_AUDIO_TEST_TONE <nrf53_audio_app_configuration_options>` - Test tone generation
* :ref:`CONFIG_AUDIO_MUTE <nrf53_audio_app_configuration_options>` - Audio muting functionality

These options are useful for development, testing, and debugging audio applications.

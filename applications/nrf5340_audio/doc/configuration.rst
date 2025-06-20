.. _nrf53_audio_app_configuration:

Configuring the nRF5340 Audio applications
##########################################

.. contents::
   :local:
   :depth: 2

|config|

.. _nrf53_audio_app_configuration_select_bidirectional:

Selecting the CIS bidirectional communication
*********************************************

To switch to the bidirectional mode, set the ``CONFIG_STREAM_BIDIRECTIONAL`` Kconfig option to ``y``  in the :file:`applications/nrf5340_audio/prj.conf` file (for the debug version) or in the :file:`applications/nrf5340_audio/prj_release.conf` file (for the release version).

.. _nrf53_audio_app_configuration_enable_walkie_talkie:

Enabling the walkie-talkie demo
===============================

The walkie-talkie demo uses one or two bidirectional streams from the gateway to one or two headsets.
The PDM microphone is used as input on both the gateway and headset device.
To switch to using the walkie-talkie, set the ``CONFIG_WALKIE_TALKIE_DEMO`` Kconfig option to ``y``  in the :file:`applications/nrf5340_audio/prj.conf` file (for the debug version) or in the :file:`applications/nrf5340_audio/prj_release.conf` file (for the release version).

Enabling the Auracast™ (broadcast) mode
=======================================

If you want to work with `Auracast™`_ (broadcast) sources and sinks, set the :kconfig:option:`CONFIG_TRANSPORT_BIS` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file.

.. _nrf53_audio_app_configuration_select_bis_two_gateways:

Enabling the BIS mode with two gateways
***************************************

In addition to the standard BIS mode with one gateway, you can also add a second gateway device.
The BIS headsets can then switch between the two gateways and receive audio stream from one of the two gateways.

To configure the second gateway, add both the ``CONFIG_TRANSPORT_BIS`` and the ``CONFIG_BT_AUDIO_USE_BROADCAST_NAME_ALT`` Kconfig options set to ``y`` to the :file:`applications/nrf5340_audio/prj.conf` file for the debug version and to the :file:`applications/nrf5340_audio/prj_release.conf` file for the release version.
You can provide an alternative name to the second gateway using the ``CONFIG_BT_AUDIO_BROADCAST_NAME_ALT`` or use the default alternative name.

You build each BIS gateway separately using the normal procedures from :ref:`nrf53_audio_app_building`.
After building the first gateway, configure the required Kconfig options for the second gateway and build the second gateway firmware.
Remember to program the two firmware versions to two separate gateway devices.

.. _nrf53_audio_app_configuration_select_i2s:

Selecting the analog jack input using I2S
*****************************************

In the default configuration, the gateway application uses USB as the audio source.
The :ref:`nrf53_audio_app_building` and the testing steps also refer to using the USB serial connection.

To switch to using the 3.5 mm jack analog input, set the ``CONFIG_AUDIO_SOURCE_I2S`` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file for the debug version and in the :file:`applications/nrf5340_audio/prj_release.conf` file for the release version.

When testing the application, an additional audio jack cable is required to use I2S.
Use this cable to connect the audio source (PC) to the analog **LINE IN** on the development kit.

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

To set the TX power output, use the ``CONFIG_BT_CTLR_TX_PWR_ANTENNA`` and ``CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB`` Kconfig options in :file:`applications/nrf5340_audio/sysbuild/ipc_radio/prj.conf`.

See :ref:`ug_radio_fem` for more information about FEM in the |NCS|.

.. _nrf53_audio_app_configuration_sd_card_playback:

Enabling SD card playback
*************************

The SD Card Playback module allows you to play audio files directly from an SD card inserted into the nRF5340 Audio development kit.
This feature supports both WAV and LC3 audio file formats and is compatible with all nRF5340 Audio applications.

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

* ``CONFIG_NRF5340_AUDIO_SD_CARD_MODULE`` - to enable the SD card module; this option is enabled by default on nRF5340 Audio DK
* ``CONFIG_SD_CARD_PLAYBACK`` - to enable the playback functionality

Optionally, you can also set the following Kconfig options:

.. list-table:: Optional SD card playback Kconfig options
   :header-rows: 1

   * - Kconfig option
     - Description
     - Default value
   * - ``CONFIG_SD_CARD_PLAYBACK_STACK_SIZE``
     - Stack size for the SD card playback thread
     - 4096
   * - ``CONFIG_SD_CARD_PLAYBACK_RING_BUF_SIZE``
     - Size of the ring buffer for audio data
     - 960
   * - ``CONFIG_SD_CARD_PLAYBACK_THREAD_PRIO``
     - Priority for the SD card playback thread
     - 7

Shell commands for SD card playback
===================================

When SD card playback is enabled, the following shell commands are available:

* ``sd_card_playback play_wav <filename>.wav`` - Play a WAV file from the SD card
* ``sd_card_playback play_lc3 <filename>.lc3`` - Play an LC3 file from the SD card
* ``sd_card_playback list_files`` - List files in the current directory
* ``sd_card_playback cd <directory>`` - Change to a different directory
* ``sd_card_playback cd /`` - Return to the root directory

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

   a. Issue the ``sd_card_playback list_files`` command to list the files on the SD card.
   b. Issue the ``sd_card_playback play_wav <filename>.wav`` command to play a WAV file.
   c. Issue the ``sd_card_playback play_lc3 <filename>.lc3`` command to play an LC3 file.

   The audio from the SD card will be mixed with any existing audio stream and played through the device's audio output.
#. To stop the playback, issue the ``sd_card_playback stop`` command.
#. To exit the shell, issue the ``exit`` command.

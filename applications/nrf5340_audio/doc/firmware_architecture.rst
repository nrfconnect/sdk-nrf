.. _nrf53_audio_app_overview:

nRF5340 Audio overview and firmware architecture
################################################

.. contents::
   :local:
   :depth: 2

The application can work as a gateway or a headset.
The gateway receives the audio data from external sources (USB or I2S) and forwards it to one or more headsets.
The headset is a receiver device that plays back the audio it gets from the gateway.
It is also possible to enable a bidirectional mode where one gateway can send and receive audio to and from one or two headsets at the same time.

Both device types use the same code base, but different firmware, and you need both types of devices for testing the application.
Gateways and headsets can both run in one of the available application modes, either the *connected isochronous stream* (CIS) mode or in the *broadcast isochronous stream* (BIS) mode.
The CIS mode is the default mode of the application.

Changing configuration related to the device type and the application modes requires rebuilding the firmware and reprogramming the development kits.

Regardless of the configuration, the application handles the audio data in the following manner:

1. The gateway receives audio data from the audio source over USB or I2S.
#. The gateway processes the audio data in its application core, which channels the data through the application layers:

   a. Audio data is sent to the synchronization module (I2S-based firmware) or directly to the software codec (USB-based firmware).
   #. Audio data is encoded by the software codec.
   #. Encoded audio data is sent to the Bluetooth LE Host.

#. The host sends the encoded audio data to the LE Audio Controller Subsystem for nRF53 on the network core.
#. The subsystem forwards the audio data to the hardware radio and sends it to the headset devices, as per the LE Audio specifications.
#. The headsets receive the encoded audio data on their hardware radio on the network core side.
#. The LE Audio Controller Subsystem for nRF53 running on each of the headsets sends the encoded audio data to the Bluetooth LE Host on the headsets' application core.
#. The headsets process the audio data in their application cores, which channel the data through the application layers:

   a. Audio data is sent to the stream control module and placed in a FIFO buffer.
   #. Audio data is sent from the FIFO buffer to the synchronization module (headsets only use I2S-based firmware).
   #. Audio data is decoded by the software codec.

#. Decoded audio data is sent to the hardware audio output over I2S.

In the `I2S-based firmware for gateway and headsets`_, sending the audio data through the application layers includes a mandatory synchronization step using the synchronization module.
This proprietary module ensures that the audio is played at the same time with the correct speed.
For more information, see `Synchronization module overview`_.

.. _nrf53_audio_app_overview_modes:

Application modes
*****************

The application can work either in the *connected isochronous stream* (CIS) mode or in the *broadcast isochronous stream* (BIS) mode, depending on the chosen firmware configuration.

.. figure:: /images/octave_application_topologies.svg
   :alt: CIS and BIS mode overview

   CIS and BIS mode overview

Connected Isochronous Stream (CIS)
  CIS is a bidirectional communication protocol that allows for sending separate connected audio streams from a source device to one or more receivers.
  The gateway can send the audio data using both the left and the right ISO channels at the same time, allowing for stereophonic sound reproduction with synchronized playback.

  This is the default configuration of the nRF5340 Audio application.
  In this configuration, you can use the nRF5340 Audio development kit in the role of the gateway, the left headset, or the right headset.

  In the current version of the nRF5340 Audio application, the CIS mode offers both unidirectional and bidirectional communication.
  In the bidirectional communication, the headset device will send audio from the on-board PDM microphone.
  See :ref:`nrf53_audio_app_configuration_select_bidirectional` in the application description for more information.

  You can also enable a walkie-talkie demonstration.
  In this demonstration, the gateway device will send audio from the on-board PDM microphone instead of using USB or the line-in.
  See :ref:`nrf53_audio_app_configuration_enable_walkie_talkie` in the application description for more information.

Broadcast Isochronous Stream (BIS)
  BIS is a unidirectional communication protocol that allows for broadcasting one or more audio streams from a source device to an unlimited number of receivers that are not connected to the source.

  In this configuration, you can use the nRF5340 Audio development kit in the role of the gateway or as one of the headsets.
  Use multiple nRF5340 Audio development kits to test BIS having multiple receiving headsets.

  .. note::
     In the BIS mode, you can use any number of nRF5340 Audio development kits as receivers.

The audio quality for both modes does not change, although the processing time for stereo can be longer.

.. _nrf53_audio_app_overview_architecture:

Firmware architecture
*********************

The following figure illustrates the software layout for the nRF5340 Audio application:

.. figure:: /images/octave_application_structure_generic.svg
   :alt: nRF5340 Audio high-level design (overview)

   nRF5340 Audio high-level design (overview)

The network core of the nRF5340 SoC runs the *LE Audio Controller Subsystem for nRF53*, which is included in the :ref:`lib_bt_ll_acs_nrf53_readme` library's HEX file.
This subsystem is custom-made for the application.
It is responsible for receiving the audio stream data from hardware layers and forwarding the data to the Bluetooth LE host on the application core.
The subsystem implements the lower layers of the Bluetooth Low Energy software stack and follows the LE Audio specification requirements.

The application core runs both the Bluetooth LE Host from Zephyr and the application layer.
The application layer is composed of a series of modules from different sources.
These modules include the following major ones:

* Peripheral modules from the |NCS|:

  * I2S
  * USB
  * SPI
  * TWI/I2C
  * UART (debug)
  * Timer
  * LC3 encoder/decoder

* Application-specific Bluetooth modules for handling the Bluetooth connection:

  * :file:`le_audio_cis_gateway.c` or :file:`le_audio_cis_headset.c` - One of these ``cis`` modules is used by default.
  * :file:`le_audio_bis_gateway.c` or :file:`le_audio_bis_headset.c` - One of these ``bis`` modules is selected automatically when you :ref:`switch to the BIS configuration <nrf53_audio_app_configuration_select_bis>`.

  Only one of these files is used at compile time.
  Each of these files handles the Bluetooth connection and Bluetooth events and funnels the data to the relevant audio modules.

* Application-specific custom modules:

  * Stream Control - This module implements a simple state machine for the application (``STREAMING`` or ``PAUSED``).
    It also handles events from Bluetooth LE and buttons, receives audio from the host, and forwards the audio data to the next module.
  * FIFO buffers
  * Synchronization module (part of `I2S-based firmware for gateway and headsets`_) - See `Synchronization module overview`_ for more information.

Since the application architecture is uniform and the firmware code is shared, the set of audio modules in use depends on the chosen stream mode (BIS or CIS), the chosen audio inputs and outputs (USB or analog jack), and if the gateway or the headset configuration is selected.

.. note::
   In the current version of the application, the bootloader is disabled by default.
   Device Firmware Update (DFU) can only be enabled when :ref:`nrf53_audio_app_building_script`.
   See :ref:`nrf53_audio_app_configuration_configure_fota` for details.

Communications between modules
==============================

Communication between modules is primarily done through Zephyr's :ref:`zephyr:zbus` to make sure that there are as few dependencies as possible. Each of the buses used by the application has their message structures described in :file:`nrf5340_audio_common.h`.

The application uses the following buses:

  * ``le_audio_chan`` - For handling LE Audio events from :file:`le_audio_cis_headset.c`, :file:`le_audio_bis_headset.c`, :file:`le_audio_cis_gateway.c`, or :file:`le_audio_bis_gateway.c`.
  * ``button_chan`` - For handling button events from :file:`button_handler.c`.
  * ``bt_mgmt_chan`` - For handling ACL events from :file:`bt_mmgmt.c`.
  * ``volume_chan`` - For handling volume events from :file:`bt_rend.c`.
  * ``cont_media_chan`` - For handling media events from :file:`content_ctrl.c`.

The consumer functions for each of these buses are residing, for the most part, in :file:`streamctrl.c`. ``volume_chan`` is an exception, with its consumer residing directly in :file:`hw_codec.c`. The linking of producers and consumers is done in :file:`main.c` and will potentially be different between different user applications..

.. _nrf53_audio_app_overview_architecture_usb:

USB-based firmware for gateway
==============================

The following figure shows an overview of the modules currently included in the firmware that uses USB:

.. figure:: /images/octave_application_structure_gateway.svg
   :alt: nRF5340 Audio modules on the gateway using USB

   nRF5340 Audio modules on the gateway using USB

In this firmware design, no synchronization module is used after decoding the incoming frames or before encoding the outgoing ones.
The Bluetooth LE RX FIFO is mainly used to make decoding run in a separate thread.

.. _nrf53_audio_app_overview_architecture_i2s:

I2S-based firmware for gateway and headsets
===========================================

The following figure shows an overview of the modules currently included in the firmware that uses I2S:

.. figure:: /images/octave_application_structure.svg
   :alt: nRF5340 Audio modules on the gateway and the headsets using I2S

   nRF5340 Audio modules on the gateway and the headsets using I2S

The Bluetooth LE RX FIFO is mainly used to make :file:`audio_datapath.c` (synchronization module) run in a separate thread.
After encoding the audio data received from I2S, the frames are sent by the encoder thread using a function located in :file:`streamctrl.c`.

.. _nrf53_audio_app_overview_architecture_sync_module:

Synchronization module overview
===============================

The synchronization module (:file:`audio_datapath.c`) handles audio synchronization.
To synchronize the audio, it executes the following types of adjustments:

* Presentation compensation
* Drift compensation

The presentation compensation makes all the headsets play audio at the same time, even if the packets containing the audio frames are not received at the same time on the different headsets.
In practice, it moves the audio data blocks in the FIFO forward or backward a few blocks, adding blocks of *silence* when needed.

The drift compensation adjusts the frequency of the audio clock to adjust the speed at which the audio is played.
This is required in the CIS mode, where the gateway and headsets must keep the audio playback synchronized to provide True Wireless Stereo (TWS) audio playback.
As such, it provides both larger adjustments at the start and then continuous small adjustments to the audio synchronization.
This compensation method counters any drift caused by the differences in the frequencies of the quartz crystal oscillators used in the development kits.
Development kits use quartz crystal oscillators to generate a stable clock frequency.
However, the frequency of these crystals always slightly differs.
The drift compensation makes the inter-IC sound (I2S) interface on the headsets run as fast as the Bluetooth packets reception.
This prevents I2S overruns or underruns, both in the CIS mode and the BIS mode.

See the following figure for an overview of the synchronization module.

.. figure:: /images/octave_application_structure_sync_module.svg
   :alt: nRF5340 Audio synchronization module overview

   nRF5340 Audio synchronization module overview

Both synchronization methods use the SDU reference timestamps (:c:type:`sdu_ref`) as the reference variable.
If the device is a gateway that is :ref:`using I2S as audio source <nrf53_audio_app_overview_architecture_i2s>` and the stream is unidirectional (gateway to headsets), :c:type:`sdu_ref` is continuously being extracted from the LE Audio Controller Subsystem for nRF53 on the gateway.
The extraction happens inside the :file:`le_audio_cis_gateway.c` and :file:`le_audio_bis_gateway.c` files' send function.
The :c:type:`sdu_ref` values are then sent to the gateway's synchronization module, and used to do drift compensation.

.. note::
   Inside the synchronization module (:file:`audio_datapath.c`), all time-related variables end with ``_us`` (for microseconds).
   This means that :c:type:`sdu_ref` becomes :c:type:`sdu_ref_us` inside the module.

As the nRF5340 is a dual-core SoC, and both cores need the same concept of time, each core runs a free-running timer in an infinite loop.
These two timers are reset at the same time, and they run from the same clock source.
This means that they should always show the same values for the same points in time.
The network core of the nRF5340 running the LE controller for nRF53 uses its timer to generate the :c:type:`sdu_ref` timestamp for every audio packet received.
The application core running the nRF5340 Audio application uses its timer to generate :c:type:`cur_time` and :c:type:`frame_start_ts`.

After the decoding takes place, the audio data is divided into smaller blocks and added to a FIFO.
These blocks are then continuously being fed to I2S, block by block.

See the following figure for the details of the compensation methods of the synchronization module.

.. figure:: /images/octave_application_sync_module_states.svg
   :alt: nRF5340 Audio's state machine for compensation mechanisms

   nRF5340 Audio's state machine for compensation mechanisms

The following external factors can affect the presentation compensation:

* The drift compensation must be synchronized to the locked state (:c:enumerator:`DRIFT_STATE_LOCKED`) before the presentation compensation can start.
  This drift compensation adjusts the frequency of the audio clock, indicating that the audio is being played at the right speed.
  When the drift compensation is not in the locked state, the presentation compensation does not leave the init state (:c:enumerator:`PRES_STATE_INIT`).
  Also, if the drift compensation loses synchronization, moving out of :c:enumerator:`DRIFT_STATE_LOCKED`, the presentation compensation moves back to :c:enumerator:`PRES_STATE_INIT`.
* When audio is being played, it is expected that a new audio frame is received in each ISO connection interval.
  If this does not occur, the headset might have lost its connection with the gateway.
  When the connection is restored, the application receives a :c:type:`sdu_ref` not consecutive with the previously received :c:type:`sdu_ref`.
  Then the presentation compensation is put into :c:enumerator:`PRES_STATE_WAIT` to ensure that the audio is still in sync.

.. note::
   When both the drift and presentation compensation are in state *locked* (:c:enumerator:`DRIFT_STATE_LOCKED` and :c:enumerator:`PRES_STATE_LOCKED`), **LED2** lights up.

Synchronization module flow
---------------------------

The received audio data in the I2S-based firmware devices follows the following path:

1. The LE Audio Controller Subsystem for nRF53 running on the network core receives the compressed audio data.
#. The controller subsystem sends the audio data to the Zephyr Bluetooth LE host similarly to the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample.
#. The host sends the data to the stream control module (:file:`streamctrl.c`).
#. The data is sent to a FIFO buffer.
#. The data is sent from the FIFO buffer to the :file:`audio_datapath.c` synchronization module.
   The :file:`audio_datapath.c` module performs the audio synchronization based on the SDU reference timestamps.
   Each package sent from the gateway gets a unique SDU reference timestamp.
   These timestamps are generated on the headset controllers (in the network core).
   This enables the creation of True Wireless Stereo (TWS) earbuds where the audio is synchronized in the CIS mode.
   It does also keep the speed of the inter-IC sound (I2S) interface synchronized with the sending and receiving speed of Bluetooth packets.
#. The :file:`audio_datapath.c` module sends the compressed audio data to the LC3 audio decoder for decoding.

#. The audio decoder decodes the data and sends the uncompressed audio data (PCM) back to the :file:`audio_datapath.c` module.
#. The :file:`audio_datapath.c` module continuously feeds the uncompressed audio data to the hardware codec.
#. The hardware codec receives the uncompressed audio data over the inter-IC sound (I2S) interface and performs the digital-to-analog (DAC) conversion to an analog audio signal.

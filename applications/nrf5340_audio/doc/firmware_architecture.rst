.. _nrf53_audio_app_overview:

nRF5340 Audio overview and firmware architecture
################################################

.. contents::
   :local:
   :depth: 2

Each nRF5340 Audio application corresponds to one specific LE Audio role: unicast client (gateway), unicast server (headset), broadcast source (gateway), or broadcast sink (headset).

Likewise, each nRF5340 Audio application is configured for one specific LE Audio mode: the *connected isochronous stream* (CIS, unicast) mode or in the *broadcast isochronous stream* (BIS) mode.
See :ref:`nrf53_audio_app_overview_modes` for more information.

The applications use the same code base, but use different :file:`main.c` files and include different modules and libraries depending on the configuration.

You might need to configure and program two applications for testing the interoperability, depending on your use case.
See the testing steps for each of the application for more information.

.. _nrf53_audio_app_overview_gateway_headsets:

Gateway and headset roles
*************************

The gateway is a common term for a base device, such as the unicast client or an Auracast/broadast source, often used with USB or analog jack input.
Often, but not always, the gateway is the largest or most stationary device, and is commonly the Bluetooth Central (if applicable).

The headset is a common term for a receiver device that plays back the audio it gets from the gateway.
Headset devices include earbuds, headphones, speakers, hearing aids, or similar.
They act as a unicast server or a broadcast sink.
With reference to the gateway, the headset is often the smallest and most portable device, and is commonly the Bluetooth Peripheral (if applicable).

You can :ref:`select gateway or headset build <nrf53_audio_app_configuration_select_build>` when :ref:`nrf53_audio_app_configuration`.

.. _nrf53_audio_app_overview_modes:

Application modes
*****************

Each application works either in the *connected isochronous stream* (CIS) mode or in the *broadcast isochronous stream* (BIS) mode.

.. figure:: /images/nrf5340_audio_application_topologies.png
   :alt: CIS and BIS mode overview

   CIS and BIS mode overview

Connected Isochronous Stream (CIS)
  CIS is a bidirectional communication protocol that allows for sending separate connected audio streams from a source device to one or more receivers.
  The gateway can send the audio data using both the left and the right ISO channels at the same time, allowing for stereophonic sound reproduction with synchronized playback.

  This is the mode available for the unicast applications (:ref:`unicast client<nrf53_audio_unicast_client_app>` and :ref:`unicast server<nrf53_audio_unicast_server_app>`).
  In this mode, you can use the nRF5340 Audio development kit in the role of the gateway, the left headset, or the right headset.

  In the current version of the nRF5340 Audio unicast client, the application offers both unidirectional and bidirectional communication.
  In the bidirectional communication, the headset device will send audio from the on-board PDM microphone.
  See :ref:`nrf53_audio_app_configuration_select_bidirectional` in the application description for more information.

  You can also enable a walkie-talkie demonstration.
  In this demonstration, the gateway device will send audio from the on-board PDM microphone instead of using USB or the line-in.
  See :ref:`nrf53_audio_app_configuration_enable_walkie_talkie` in the application description for more information.

Broadcast Isochronous Stream (BIS)
  BIS is a unidirectional communication protocol that allows for broadcasting one or more audio streams from a source device to an unlimited number of receivers that are not connected to the source.

  This is the mode available for the broadcast applications (:ref:`broadcast source<nrf53_audio_broadcast_source_app>` for headset and :ref:`broadcast sink<nrf53_audio_broadcast_sink_app>` for gateway).
  In this mode, you can use the nRF5340 Audio development kit in the role of the gateway or as one of the headsets.
  Use multiple nRF5340 Audio development kits to test BIS having multiple receiving headsets.

  .. note::
     In the BIS mode, you can use any number of nRF5340 Audio development kits as receivers.

The audio quality for both modes does not change, although the processing time for stereo can be longer.

.. _nrf53_audio_app_overview_architecture:

Firmware architecture
*********************

The following figure illustrates the high-level software layout for the nRF5340 Audio application:

.. figure:: /images/nrf5340_audio_structure_generic.svg
   :alt: nRF5340 Audio high-level design (overview)

   nRF5340 Audio high-level design (overview)

The network core of the nRF5340 SoC runs the SoftDevice Controller, which is responsible for receiving the audio stream data from hardware layers and forwarding the data to the Bluetooth LE host on the application core.
The controller implements the lower layers of the Bluetooth Low Energy software stack.
See :ref:`ug_ble_controller_softdevice` for more information about the controller, and :ref:`SoftDevice Controller for LE Isochronous Channels <nrfxlib:softdevice_controller_iso>` for information on how it implements ISO channels used by the nRF5340 Audio applications.

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

  * Management - This module handles scanning and advertising, in addition to general initialization, controller configuration, and transfer of DFU images.
  * Stream - This module handles the setup and transfer of audio in the Bluetooth LE Audio context.
    It includes submodules for CIS (unicast) and BIS (broadcast).
  * Renderer - This module handles rendering, such as volume up and down.
  * Content Control - This module handles content control, such as play and pause.

* Application-specific custom modules, including the synchronization module (part of `I2S-based firmware for gateway and headsets`_) - See `Synchronization module overview`_ for more information.

Since the application architecture is the same for all applications and the code before compilation is shared to a significant degree, the set of modules in use depends on the chosen audio inputs and outputs (USB or analog jack).

.. note::
   In the current versions of the applications, the bootloader is disabled by default.
   Device Firmware Update (DFU) can only be enabled when :ref:`nrf53_audio_app_building_script`.
   See :ref:`nrf53_audio_app_configuration_configure_fota` for details.

.. _nrf53_audio_app_overview_files:

Source file architecture
========================

The following figure illustrates the software layout for the nRF5340 Audio application on the file-by-file level, regardless of the application chosen:

.. figure:: /images/nrf5340audio_all_packages.svg
   :alt: nRF5340 Audio application file-level breakdown

   nRF5340 Audio application file-level breakdown

Communication between modules is primarily done through Zephyr's :ref:`zephyr:zbus` to make sure that there are as few dependencies as possible. Each of the buses used by the applications has their message structures described in :file:`zbus_common.h`.

.. _nrf53_audio_app_overview_architecture_usb:

USB-based firmware for gateway
==============================

The following figures show an overview of the modules currently included in the firmware of applications that use USB.

In this firmware design, no synchronization module is used after decoding the incoming frames or before encoding the outgoing ones.
The Bluetooth LE RX FIFO is mainly used to make decoding run in a separate thread.

Broadcast source USB-based firmware
-----------------------------------

.. figure:: /images/nrf5340_audio_broadcast_source_USB_structure.svg
   :alt: nRF5340 Audio modules for the broadcast source using USB

   nRF5340 Audio modules for the broadcast source using USB

Unicast client USB-based firmware
---------------------------------

.. figure:: /images/nrf5340_audio_unicast_client_USB_structure.svg
   :alt: nRF5340 Audio modules for the unicast client using USB

   nRF5340 Audio modules for the unicast client using USB

.. _nrf53_audio_app_overview_architecture_i2s:

I2S-based firmware for gateway and headsets
===========================================

The following figure shows an overview of the modules currently included in the firmware of applications that use I2S.

The Bluetooth LE RX FIFO is mainly used to make :file:`audio_datapath.c` (synchronization module) run in a separate thread.

Broadcast source I2S-based firmware
-----------------------------------

.. figure:: /images/nrf5340_audio_broadcast_source_I2S_structure.svg
   :alt: nRF5340 Audio modules for the broadcast source using I2S

   nRF5340 Audio modules for the broadcast source using I2S

Broadcast sink I2S-based firmware
---------------------------------

.. figure:: /images/nrf5340_audio_broadcast_sink_I2S_structure.svg
   :alt: nRF5340 Audio modules for the broadcast sink using I2S

   nRF5340 Audio modules for the broadcast sink using I2S

Unicast client I2S-based firmware
---------------------------------

.. figure:: /images/nrf5340_audio_unicast_client_I2S_structure.svg
   :alt: nRF5340 Audio modules for the unicast client using I2S

   nRF5340 Audio modules for the unicast client using I2S

Unicast server I2S-based firmware
---------------------------------

.. figure:: /images/nrf5340_audio_unicast_server_I2S_structure.svg
   :alt: nRF5340 Audio modules for the unicast server using I2S

   nRF5340 Audio modules for the unicast server using I2S

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

.. figure:: /images/nrf5340_audio_structure_sync_module.svg
   :alt: nRF5340 Audio synchronization module overview

   nRF5340 Audio synchronization module overview

Both synchronization methods use the SDU reference timestamps (:c:type:`sdu_ref`) as the reference variable.
If the device is a gateway that is :ref:`using I2S as audio source <nrf53_audio_app_overview_architecture_i2s>` and the stream is unidirectional (gateway to headsets), :c:type:`sdu_ref` is continuously being extracted from the LE Audio Controller Subsystem for nRF53 on the gateway.
The extraction happens inside the :file:`unicast_client.c` and :file:`broadcast_source.c` files' send function.
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

.. figure:: /images/nrf5340_audio_sync_module_states.svg
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

1. The SoftDevice Controller running on the network core receives the compressed audio data.
#. The controller, running in the :zephyr:code-sample:`bluetooth_hci_ipc` sample on the nRF5340 SoC network core, sends the audio data to the Zephyr Bluetooth LE host running on the nRF5340 SoC application core.
#. The host sends the data to the stream control module.
#. The data is sent to a FIFO buffer.
#. The data is sent from the FIFO buffer to the :file:`audio_datapath.c` synchronization module.
   The :file:`audio_datapath.c` module performs the audio synchronization based on the SDU reference timestamps.
   Each package sent from the gateway gets a unique SDU reference timestamp.
   These timestamps are generated on the headset Bluetooth LE controller (in the network core).
   This enables the creation of True Wireless Stereo (TWS) earbuds where the audio is synchronized in the CIS mode.
   It does also keep the speed of the inter-IC sound (I2S) interface synchronized with the sending and receiving speed of Bluetooth packets.
#. The :file:`audio_datapath.c` module sends the compressed audio data to the LC3 audio decoder for decoding.

#. The audio decoder decodes the data and sends the uncompressed audio data (PCM) back to the :file:`audio_datapath.c` module.
#. The :file:`audio_datapath.c` module continuously feeds the uncompressed audio data to the hardware codec.
#. The hardware codec receives the uncompressed audio data over the inter-IC sound (I2S) interface and performs the digital-to-analog (DAC) conversion to an analog audio signal.

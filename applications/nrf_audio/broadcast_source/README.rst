.. _nrf_audio_broadcast_source_app:

nRF Audio: Broadcast source
###############################

.. contents::
   :local:
   :depth: 2

The nRF Audio broadcast source application implements the :ref:`BIS gateway mode <nrf_audio_app_overview>`.

In this mode, transmitting broadcast audio happens using Broadcast Isochronous Stream (BIS) and Broadcast Isochronous Group (BIG).
Play and pause are emulated by enabling and disabling stream, respectively.

The following limitations apply to this application:

* One BIG with two BIS streams.
* Audio input: USB or I2S (Line-in or using Pulse Density Modulation).
* Configuration: 16-bit, several bit rates ranging from 32 kbps to 124 kbps.

.. _nrf_audio_broadcast_source_app_requirements:

Requirements
************

The application shares the :ref:`requirements common to all nRF Audio application <nrf_audio_app_requirements>`.

.. _nrf_audio_broadcast_source_app_ui:

User interface
**************

Most of the user interface mappings are common across all nRF Audio applications.
See the :ref:`nrf_audio_app_ui` page for detailed overview.

This application uses specific mapping for the following user interface elements:

* Pressed on the broadcast source device during playback:

  * **PLAY/PAUSE** - Starts or pauses the playback of the stream.
  * **BTN 4** -  Toggles between the normal audio stream and different test tones generated on the device.
    Use this tone to check the synchronization of headsets.

* **LED1** - Blinking blue - Device has started broadcasting audio.
* **RGB** - Solid green - The device is programmed as the gateway.

.. _nrf_audio_broadcast_source_app_configuration:

Configuration
*************

The application requires the ``CONFIG_TRANSPORT_BIS`` Kconfig option to be set to ``y`` in the :file:`applications/nrf_audio/prj.conf` file for `Building and running`_ to succeed.

For other configuration options, see :ref:`nrf_audio_app_configuration`.

For information about how to configure applications in the |NCS|, see :ref:`configure_application`.

.. _nrf_audio_broadcast_source_app_building:

Building and running
********************

This application can be found under :file:`applications/nrf_audio/broadcast_source` in the nRF Connect SDK folder structure, but it uses :file:`.conf` files at :file:`applications/nrf_audio/`.

The nRF5340 Audio DK comes preprogrammed with basic firmware that indicates if the kit is functional.
See :ref:`nrf_audio_app_dk_testing_out_of_the_box` for more information.

To build the application, complete the following steps:

1. Select the BIS mode by setting the ``CONFIG_TRANSPORT_BIS`` Kconfig option to ``y`` in the :file:`applications/nrf_audio/prj.conf` file for the debug version and in the :file:`applications/nrf_audio/prj_release.conf` file for the release version.
#. Complete the steps for building and programming common to all audio applications using one of the following methods:

   * :ref:`nrf_audio_app_building_script`
   * :ref:`nrf_audio_app_building_standard`

After programming, the broadcast source automatically starts broadcasting the default 48-kHz audio stream.

.. _nrf_audio_broadcast_source_app_testing:

Testing
*******

.. note::
    |nrf_audio_external_devices_note|

To test the broadcast source application, complete the following steps:

1. Make sure you have another nRF5340 Audio DK for testing purposes.
#. Program the other DK with the :ref:`broadcast sink <nrf_audio_broadcast_sink_app>` application.
   The broadcast sink device automatically synchronizes with the broadcast source after programming.
#. Proceed to testing the broadcast source using the :ref:`nrf_audio_broadcast_source_app_ui` buttons and LEDs.

Dependencies
************

For the list of dependencies, check the application's source files.

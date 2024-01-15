.. _nrf53_audio_broadcast_source_app:

nRF5340 Audio: Broadcast source
###############################

.. contents::
   :local:
   :depth: 2

The nRF5340 Audio broadcast source application implements the :ref:`BIS gateway mode <nrf53_audio_app_overview>`.

In this mode, transmitting broadcast audio happens using Broadcast Isochronous Stream (BIS) and Broadcast Isochronous Group (BIG).
Play and pause are emulated by enabling and disabling stream, respectively.

The following limitations apply to this application:

* One BIG with two BIS streams.
* Audio input: USB or I2S (Line in or using Pulse Density Modulation).
* Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

.. _nrf53_audio_broadcast_source_app_requirements:

Requirements
************

The application shares the :ref:`requirements common to all nRF5340 Audio application <nrf53_audio_app_requirements>`.

.. _nrf53_audio_broadcast_source_app_ui:

User interface
**************

Most of the user interface mappings are common across all nRF5340 Audio applications.
See the :ref:`nrf53_audio_app_ui` page for detailed overview.

This application uses specific mapping for the following user interface elements:

* Pressed on the broadcast source device during playback:

  * **PLAY/PAUSE** - Starts or pauses the playback of the stream.
  * **BTN 4** -  Toggles between the normal audio stream and different test tones generated on the device.
    Use this tone to check the synchronization of headsets.

* **LED1** - Blinking blue - Device has started broadcasting audio.
* **RGB** - Solid green - The device is programmed as the gateway.

.. _nrf53_audio_broadcast_source_app_configuration:

Configuration
*************

See :ref:`nrf53_audio_app_configuration` and :ref:`nrf53_audio_app_fota` for configuration options common to all nRF5340 Audio applications.

For information about how to configure applications in the |NCS|, see :ref:`configure_application`.

.. _nrf53_audio_broadcast_source_app_building:

Building and running
********************

This application can be found under :file:`applications/nrf5340_audio/broadcast_source` in the nRF Connect SDK folder structure.

The nRF5340 Audio DK comes preprogrammed with basic firmware that indicates if the kit is functional.
See :ref:`nrf53_audio_app_dk_testing_out_of_the_box` for more information.

To build the application, see :ref:`nrf53_audio_app_building`.

After programming, the broadcast source automatically starts broadcasting the default 48-kHz audio stream.

.. _nrf53_audio_broadcast_source_app_testing:

Testing
*******

.. note::
    |nrf5340_audio_external_devices_note|

To test the broadcast source application, complete the following steps:

1. Make sure you have another nRF5340 Audio DK for testing purposes.
#. Program the other DK with the :ref:`broadcast sink <nrf53_audio_broadcast_sink_app>` application.
   The broadcast sink device automatically synchronizes with the broadcast source after programming.
#. Proceed to testing the broadcast source using the :ref:`nrf53_audio_broadcast_source_app_ui` buttons and LEDs.

Dependencies
************

For the list of dependencies, check the application's source files.

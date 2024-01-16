.. _nrf53_audio_broadcast_sink_app:

nRF5340 Audio: Broadcast sink
#############################

.. contents::
   :local:
   :depth: 2

The nRF5340 Audio broadcast sink application implements the :ref:`BIS headset mode <nrf53_audio_app_overview>`.
In this mode, receiving broadcast audio happens using Broadcast Isochronous Stream (BIS) and Broadcast Isochronous Group (BIG).

The following limitations apply to this application:

* One BIG, one of the two BIS streams (selectable).
* Audio output: I2S/Analog headset output.
* Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

.. _nrf53_audio_broadcast_sink_app_requirements:

Requirements
************

The application shares the :ref:`requirements common to all nRF5340 Audio application <nrf53_audio_app_requirements>`.

.. _nrf53_audio_broadcast_sink_app_ui:

User interface
**************

Most of the user interface mappings are common across all nRF5340 Audio applications.
See the :ref:`nrf53_audio_app_ui` page for detailed overview.

This application uses specific mapping for the following user interface elements:

* Long-pressed on the broadcast sink device during startup:

  * **VOL-** - Changes the headset to the left channel one.
  * **VOL+** - Changes the headset to the right channel one.

* Pressed on the broadcast sink device during playback:

  * **PLAY/PAUSE** - Starts or pauses listening to the stream.
  * **VOL-** - Turns the playback volume down.
  * **VOL+** - Turns the playback volume up.
  * **BTN 4** - Changes audio stream (different BIS), if more than one is available.
  * **BTN 5** - Changes the gateway, if more than one is available.

* **LED1**:

  * Solid blue - Devices have synchronized with a broadcasted stream.
  * Blinking blue - Devices have started streaming audio (BIS mode).

* **RGB**:

  * Solid blue - The device is programmed as the left headset.
  * Solid magenta - The device is programmed as the right headset.

.. _nrf53_audio_broadcast_sink_app_configuration:

Configuration
*************

See :ref:`nrf53_audio_app_configuration` and :ref:`nrf53_audio_app_fota` for configuration options common to all nRF5340 Audio applications.

For information about how to configure applications in the |NCS|, see :ref:`modifying`.

.. _nrf53_audio_broadcast_sink_app_building:

Building and running
********************

This application can be found under :file:`applications/nrf5340_audio/broadcast_sink` in the nRF Connect SDK folder structure.

The nRF5340 Audio DK comes preprogrammed with basic firmware that indicates if the kit is functional.
See :ref:`nrf53_audio_app_dk_testing_out_of_the_box` for more information.

To build the application, see :ref:`nrf53_audio_app_building`.

.. _nrf53_audio_broadcast_sink_app_testing:

Testing
*******

.. note::
    |nrf5340_audio_external_devices_note|

To test the broadcast sink application, complete the following steps:

1. Make sure you have another nRF5340 Audio DK for testing purposes.
#. Program the other DK with the :ref:`broadcast source <nrf53_audio_broadcast_source_app>` application.
   The broadcast sink device automatically synchronizes with the broadcast source after programming.
#. Proceed to testing the devices using the :ref:`nrf53_audio_broadcast_sink_app_ui` buttons and LEDs.

Dependencies
************

For the list of dependencies, check the application's source files under :file:`applications/nrf5340_audio/broadcast_sink`.

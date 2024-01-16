.. _nrf53_audio_unicast_server_app:

nRF5340 Audio: Unicast server
#############################

.. contents::
   :local:
   :depth: 2

The nRF5340 Audio unicast server application implements the :ref:`CIS headset mode <nrf53_audio_app_overview>`.

In this mode, one Connected Isochronous Group (CIG) can be used with two Connected Isochronous Streams (CIS).
Receiving unidirectional or transceiving bidirectional audio happens using CIG and CIS.
In addition, Coordinated Set Identification Service (CSIS) is implemented on the server side.

The following limitations apply to this application:

* One CIG, one of the two CIS streams (selectable).
* Audio output: I2S/Analog headset output.
* Audio input: PDM microphone over I2S.
* Configuration: 16 bit, several bit rates ranging from 32 kbps to 124 kbps.

.. _nrf53_audio_unicast_server_app_requirements:

Requirements
************

The application shares the :ref:`requirements common to all nRF5340 Audio application <nrf53_audio_app_requirements>`.

.. _nrf53_audio_unicast_server_app_ui:

User interface
**************

Most of the user interface mappings are common across all nRF5340 Audio applications.
See the :ref:`nrf53_audio_app_ui` page for detailed overview.

This application uses specific mapping for the following user interface elements:

* Long-pressed on the unicast server device during startup:

  * **VOL-** - Changes the headset to the left channel one.
  * **VOL+** - Changes the headset to the right channel one.
  * **BTN5** - Clears the previously stored bonding information.

* Pressed on the unicast server device during playback:

  * **PLAY/PAUSE** - Starts or pauses the playback of the stream.
  * **VOL-** - Turns the playback volume down.
  * **VOL+** - Turns the playback volume up.
  * **BTN5** - Mutes the playback volume (and unmutes).

* **LED1** - Blinking blue - Kits have started streaming audio.
* **RGB**:

  * Solid blue - The device is programmed as the left headset.
  * Solid magenta - The device is programmed as the right headset.

.. _nrf53_audio_unicast_server_app_configuration:

Configuration
*************

See :ref:`nrf53_audio_app_configuration` and :ref:`nrf53_audio_app_fota` for configuration options common to all nRF5340 Audio applications.

For information about how to configure applications in the |NCS|, see :ref:`modifying`.

.. _nrf53_audio_unicast_server_app_building:

Building and running
********************

This application can be found under :file:`applications/nrf5340_audio/unicast_server` in the nRF Connect SDK folder structure.

The nRF5340 Audio DK comes preprogrammed with basic firmware that indicates if the kit is functional.
See :ref:`nrf53_audio_app_dk_testing_out_of_the_box` for more information.

To build the application, see :ref:`nrf53_audio_app_building`.

.. _nrf53_audio_unicast_server_app_testing:

Testing
*******

After building and programming the application, you can test the default CIS headset mode using one :ref:`unicast client application <nrf53_audio_unicast_client_app>` and one or two unicast server devices (this application).
The recommended approach is to use two other nRF5340 Audio DKs programmed with the :ref:`unicast client application <nrf53_audio_unicast_client_app>` for the CIS gateway and the unicast server application (this application) for the CIS headset, respectively, but you can also use an external device that supports the role of unicast server.

.. note::
    |nrf5340_audio_external_devices_note|

The following testing scenario assumes you are using USB as the audio source on the gateway.
This is the default setting.

Complete the following steps to test the unidirectional CIS mode for one gateway and two headset devices:

1. Make sure that the development kits are still plugged into the USB ports and are turned on.

   .. note::
      |usb_known_issues|

   **LED3** starts blinking green on every device to indicate the ongoing CPU activity on the application core.
#. Wait for the **LED1** on the gateway to start blinking blue.
   This happens shortly after programming the development kit and indicates that the gateway device is connected to at least one headset and ready to send data.
#. Search the list of audio devices listed in the sound settings of your operating system for *nRF5340 USB Audio* (gateway) and select it as the output device.
#. Connect headphones to the **HEADPHONE** audio jack on both headset devices.
#. Start audio playback on your PC from any source.
#. Wait for **LED1** to blink blue on the headset.
   When they do, the audio stream has started on the headset.

   .. note::
      The audio outputs only to the left channel of the audio jack, even if the given headset is configured as the right headset.
      This is because of the mono hardware codec chip used on the development kits.
      If you want to play stereo sound using one development kit, you must connect an external hardware codec chip that supports stereo.

#. Wait for **LED2** to light up solid green on the headsets to indicate that the audio synchronization is achieved.
#. Press the **VOL+** button on one of the headsets.
   The playback volume increases for the headset.
#. If you use more than one headset, hold down the **VOL+** button and press the **RESET** button on a headset.
   After startup, this headset will be configured as the right channel headset.
#. If you use more than one headset, hold down the **VOL-** button and press the **RESET** button on a headset.
   After startup, this headset will be configured as the left channel headset.
   You can also just press the **RESET** button to restore the original programmed settings.

For other testing options, refer to :ref:`nrf53_audio_unicast_server_app_ui`.

After the kits have paired for the first time, they are now bonded.
This means the Long-Term Key (LTK) is stored on each side, and that the kits will only connect to each other unless the bonding information is cleared.
To clear the bonding information, press and hold **BTN 5** during boot or reprogram all the development kits.

When you finish testing, power off the nRF5340 Audio development kits by switching the power switch from On to Off.

.. _nrf53_audio_unicast_server_app_testing_steps_cis_walkie_talkie:

Testing the walkie-talkie demo
==============================

Testing the walkie-talkie demo is identical to the default testing procedure, except for the following differences:

* You must enable the Kconfig option mentioned in :ref:`nrf53_audio_app_configuration_enable_walkie_talkie` before building the application.
* Instead of controlling the playback, you can speak through the PDM microphones.
  The line is open all the time, no need to press any buttons to talk, but the volume control works as in the default testing procedure.

Dependencies
************

For the list of dependencies, check the application's source files.

.. _nrf53_audio_app_ui:

User interface
##############

.. contents::
   :local:
   :depth: 2

All nRF5340 Audio applications implement the same, simple user interface based on the available PCB elements of the nRF5340 Audio development kit.
You can control the application using predefined switches and buttons while the LEDs display information.

Some user interface options are only valid for some nRF5340 Audio applications.

.. _nrf53_audio_app_ui_switches:

Switches
********

The application uses the following switches on the supported development kit:

+-------------------+-------------------------------------------------------------------------------------+---------------------------------------+
| Switch            | Function                                                                            | Applications                          |
+===================+=====================================================================================+=======================================+
| **POWER**         | Turns the development kit on or off.                                                | All                                   |
+-------------------+-------------------------------------------------------------------------------------+---------------------------------------+
| **DEBUG ENABLE**  | Turns on or off power for debug features.                                           | All                                   |
|                   | This switch is used for accurate power and current measurements.                    |                                       |
+-------------------+-------------------------------------------------------------------------------------+---------------------------------------+

.. _nrf53_audio_app_ui_buttons:

Buttons
*******

The application uses the following buttons on the supported development kit:

+---------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| Button        | Function                                                                                                  | Applications                                |
+===============+===========================================================================================================+=============================================+
| **VOL-**      | Long-pressed during startup: Changes the headset to the left channel one.                                 | * :ref:`nrf53_audio_broadcast_sink_app`     |
|               |                                                                                                           | * :ref:`nrf53_audio_unicast_server_app`     |
|               +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|               | Pressed on the headset or the CIS gateway during playback: Turns the playback volume down.                | * :ref:`nrf53_audio_broadcast_sink_app`     |
|               |                                                                                                           | * :ref:`nrf53_audio_unicast_server_app`     |
|               |                                                                                                           | * :ref:`nrf53_audio_unicast_client_app`     |
+---------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **VOL+**      | Long-pressed during startup: Changes the headset to the right channel one.                                | * :ref:`nrf53_audio_broadcast_sink_app`     |
|               |                                                                                                           | * :ref:`nrf53_audio_unicast_server_app`     |
|               +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|               | Pressed on the headset or the CIS gateway during playback: Turns the playback volume up.                  | * :ref:`nrf53_audio_broadcast_sink_app`     |
|               |                                                                                                           | * :ref:`nrf53_audio_unicast_server_app`     |
|               |                                                                                                           | * :ref:`nrf53_audio_unicast_client_app`     |
+---------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **PLAY/PAUSE**| Starts or pauses the playback of the stream or listening to the stream.                                   | All                                         |
+---------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **BTN 4**     | Long-pressed during startup: Turns on the DFU mode, if                                                    | All                                         |
|               | the device is :ref:`configured for it<nrf53_audio_app_configuration_configure_fota>`.                     |                                             |
|               +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|               | Pressed on the gateway during playback: Toggles between the normal audio stream and different test        | * :ref:`nrf53_audio_broadcast_source_app`   |
|               | tones generated on the device. Use this tone to check the synchronization of headsets.                    | * :ref:`nrf53_audio_unicast_client_app`     |
|               +-----------------------------------------------------------------------------------------------------------+                                             |
|               | Pressed on the gateway during playback multiple times: Changes the test tone frequency.                   |                                             |
|               | The available values are 1000 Hz, 2000 Hz, and 4000 Hz.                                                   |                                             |
|               +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|               | Pressed on a BIS headset during playback: Change stream (different BIS), if more than one is available.   | :ref:`nrf53_audio_broadcast_sink_app`       |
+---------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **BTN 5**     | Long-pressed during startup: Clears the previously stored bonding information.                            | * :ref:`nrf53_audio_unicast_server_app`     |
|               |                                                                                                           | * :ref:`nrf53_audio_unicast_client_app`     |
|               +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|               | Pressed during playback: Mutes the playback volume.                                                       | * :ref:`nrf53_audio_unicast_server_app`     |
|               |                                                                                                           | * :ref:`nrf53_audio_unicast_client_app`     |
|               +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|               | Pressed on a BIS headset during playback: Change the gateway, if more than one is available.              | :ref:`nrf53_audio_broadcast_sink_app`       |
+---------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **RESET**     | Resets the device to the originally programmed settings.                                                  | All                                         |
|               | This reverts any changes made during testing, for example the channel switches with **VOL** buttons.      |                                             |
+---------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+

.. _nrf53_audio_app_ui_leds:

LEDs
****

To indicate the tasks performed, the application uses the LED behavior described in the following table:

+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| LED                      |Indication                                                                                                 | Applications                                |
+==========================+===========================================================================================================+=============================================+
| **LED1**                 | Off - No Bluetooth connection.                                                                            | All                                         |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Solid blue on the CIS gateway and headset: Kits have connected.                                           | * :ref:`nrf53_audio_unicast_server_app`     |
|                          |                                                                                                           | * :ref:`nrf53_audio_unicast_client_app`     |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Solid blue on the BIS headset: Kits have found a broadcasting stream.                                     | :ref:`nrf53_audio_broadcast_sink_app`       |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Blinking blue on headset: Kits have started streaming audio (BIS and CIS modes).                          | * :ref:`nrf53_audio_broadcast_sink_app`     |
|                          |                                                                                                           | * :ref:`nrf53_audio_unicast_server_app`     |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Blinking blue on the CIS gateway: Kit is streaming to a headset.                                          | :ref:`nrf53_audio_unicast_client_app`       |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Blinking blue on the BIS gateway: Kit has started broadcasting audio.                                     | :ref:`nrf53_audio_broadcast_source_app`     |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **LED2**                 | Off - Sync not achieved.                                                                                  | All                                         |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Solid green - Sync achieved (both drift and presentation compensation are in the ``LOCKED`` state).       | * :ref:`nrf53_audio_broadcast_sink_app`     |
|                          |                                                                                                           | * :ref:`nrf53_audio_unicast_server_app`     |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **LED3**                 | Blinking green - The nRF5340 Audio DK application core is running.                                        | All                                         |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **CODEC**                | Off - No configuration loaded to the onboard hardware codec.                                              | All                                         |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Solid green - Hardware codec configuration loaded.                                                        | All                                         |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **RGB**                  | Solid green - The device is programmed as the gateway.                                                    | * :ref:`nrf53_audio_broadcast_source_app`   |
|                          |                                                                                                           | * :ref:`nrf53_audio_unicast_client_app`     |
| (bottom side LEDs around +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| the center opening)      | Solid blue - The device is programmed as the left headset.                                                | * :ref:`nrf53_audio_broadcast_sink_app`     |
|                          |                                                                                                           | * :ref:`nrf53_audio_unicast_server_app`     |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Solid magenta - The device is programmed as the right headset.                                            | * :ref:`nrf53_audio_broadcast_sink_app`     |
|                          |                                                                                                           | * :ref:`nrf53_audio_unicast_server_app`     |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Solid yellow - The device is programmed with factory firmware.                                            | All                                         |
|                          | It must be re-programmed as gateway or headset.                                                           |                                             |
|                          +-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
|                          | Solid red (debug mode) - Fault in the application core has occurred.                                      | All                                         |
|                          | See UART log for details and use the **RESET** button to reset the device.                                |                                             |
|                          | In the release mode, the device resets automatically with no indication on LED or UART.                   |                                             |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **ERR**                  | PMIC error or a charging error (or both).                                                                 | All                                         |
|                          | Also turns on when charging the battery exceeds seven hours, since the PMIC has a protection timeout,     |                                             |
|                          | which stops the charging.                                                                                 |                                             |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **CHG**                  | Off - Charge completed or no battery connected.                                                           | All                                         |
|                          +-----------------------------------------------------------------------------------------------------------+                                             |
|                          | Solid yellow - Charging in progress.                                                                      |                                             |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **OB/EXT**               | Off - No 3.3 V power available.                                                                           | All                                         |
|                          +-----------------------------------------------------------------------------------------------------------+                                             |
|                          | Solid green - On-board hardware codec selected.                                                           |                                             |
|                          +-----------------------------------------------------------------------------------------------------------+                                             |
|                          | Solid yellow - External hardware codec selected.                                                          |                                             |
|                          | This LED turns solid yellow also when the devices are reset, for the time then pins are floating.         |                                             |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **FTDI SPI**             | Off - No data is written to the hardware codec using SPI.                                                 | All                                         |
|                          +-----------------------------------------------------------------------------------------------------------+                                             |
|                          | Yellow - The same SPI is used for both the hardware codec and the SD card.                                |                                             |
|                          | When this LED is yellow, the shared SPI is used by the FTDI to write data to the hardware codec.          |                                             |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **IFMCU**                | Off - No PC connection available.                                                                         | All                                         |
| (bottom side)            +-----------------------------------------------------------------------------------------------------------+                                             |
|                          | Solid green - Connected to PC.                                                                            |                                             |
|                          +-----------------------------------------------------------------------------------------------------------+                                             |
|                          | Rapid green flash - USB enumeration failed.                                                               |                                             |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+
| **HUB**                  | Off - No PC connection available.                                                                         | All                                         |
| (bottom side)            +-----------------------------------------------------------------------------------------------------------+                                             |
|                          | Green - Standard USB hub operation.                                                                       |                                             |
+--------------------------+-----------------------------------------------------------------------------------------------------------+---------------------------------------------+

.. _nrf_desktop_led_stream:

LED stream module
#################

Use the |led_stream| to receive the LED effect sequence and display it using LEDs.
The LED effect sequence is generated on the host computer and sent through the :ref:`nrf_desktop_config_channel`.

Module Events
*************

+-----------------------------------------------+--------------------------------+----------------+------------------------+---------------------------------------------+
| Source Module                                 | Input Event                    | This Module    | Output Event           | Sink Module                                 |
+===============================================+================================+================+========================+=============================================+
| :ref:`nrf_desktop_led_state`                  | ``led_event``                  | ``led_stream`` |                        |                                             |
+-----------------------------------------------+                                |                |                        |                                             |
| :ref:`nrf_desktop_led_stream`                 |                                |                |                        |                                             |
+-----------------------------------------------+--------------------------------+                |                        |                                             |
| :ref:`nrf_desktop_leds`                       | ``led_ready_event``            |                |                        |                                             |
+-----------------------------------------------+--------------------------------+                |                        |                                             |
| :ref:`nrf_desktop_hids`                       | ``config_event``               |                |                        |                                             |
+-----------------------------------------------+                                |                |                        |                                             |
| :ref:`nrf_desktop_usb_state`                  |                                |                |                        |                                             |
+-----------------------------------------------+--------------------------------+                |                        |                                             |
| :ref:`nrf_desktop_hids`                       | ``config_fetch_request_event`` |                |                        |                                             |
+-----------------------------------------------+                                |                |                        |                                             |
| :ref:`nrf_desktop_usb_state`                  |                                |                |                        |                                             |
+-----------------------------------------------+--------------------------------+                +------------------------+---------------------------------------------+
|                                               |                                |                | ``led_event``          | :ref:`nrf_desktop_leds`                     |
|                                               |                                |                +------------------------+---------------------------------------------+
|                                               |                                |                | ``config_fetch_event`` | :ref:`nrf_desktop_hids`                     |
|                                               |                                |                |                        +---------------------------------------------+
|                                               |                                |                |                        | :ref:`nrf_desktop_usb_state`                |
+-----------------------------------------------+--------------------------------+----------------+------------------------+---------------------------------------------+

Configuration
*************

The module receives LED effects through the :ref:`nrf_desktop_config_channel` and displays them using the :ref:`nrf_desktop_leds`.
For this reason, make sure that ``CONFIG_DESKTOP_LED_ENABLE`` and ``CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE`` are both set.

To enable the module, use the ``CONFIG_DESKTOP_LED_STREAM_ENABLE`` Kconfig option.

You can also define the stream LED event queue size (``CONFIG_DESKTOP_LED_STREAM_QUEUE_SIZE``).
The queue is used by the module as data buffer for the data received from the host computer.

Configuration channel
*********************

The module provides the following :ref:`nrf_desktop_config_channel` functionalities:

* Displaying LED effects synchronized to the music played by the host computer.
* Displaying random LED effects generated by the host computer.

Use the following command to display led effects synchronized to the music from :file:`sample.wav`:

.. code-block:: console

    python configurator_cli.py gaming_mouse led_stream 0 15 --file="sample.wav"

..

In this command, provide the following arguments:

* ``0`` - ID of selected LED.
* ``15`` - LED color change frequency (in Hz).

  * Low frequency results in averaging and smooth color changes.
  * High frequency results in dynamic color changes.

  When the frequency is too high, the host computer may be unable to send effects fast enough.
  This can result in interrupting the sequence.
* ``--file="sample.wav"`` - selected music file.
  If no music file in :file:`*.wav` format is provided, the host computer generates random LED effects.

Implementation details
**********************

The module receives LED effects as ``config_event``.
The effects are sent to :ref:`nrf_desktop_leds` as ``led_event``.
Displaying the sequence begins when the first LED effect is received by the |led_stream|.

Every received LED effect has a predefined duration.
The LEDs module submits ``led_ready_event`` when it finishes displaying a LED effect.
On this event, the |led_stream| sends the next effect from the queue.

When the sequence is active, the host computer keeps sending new effects that are queued by the |led_stream|.
The sequence ends when there are no more effects available in the queue.

Synchronization of displayed effects
   The host computer can get information about the current number of queued LED effects by fetching a :ref:`nrf_desktop_config_channel` value.
   This information is used to synchronize the displayed LED effects to the music.

LED state interaction
   The :ref:`nrf_desktop_led_state` uses LEDs to display both the state of the system and the state of the connected Bluetooth peer.
   The |led_stream| takes control over the selected LED only when the sequence is displayed.
   After the sequence ends, the LED effect selected by the LED state module is restored.

.. |led_stream| replace:: LED stream module

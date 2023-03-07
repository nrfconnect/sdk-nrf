.. _nrf_desktop_led_stream:

LED stream module
#################

.. contents::
   :local:
   :depth: 2

Use the |led_stream| to receive the LED effect sequence and display it using LEDs.
The LED effect sequence is generated on the host computer and sent through the :ref:`nrf_desktop_config_channel`.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_led_stream_start
    :end-before: table_led_stream_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module receives LED effects through the :ref:`nrf_desktop_config_channel` and displays them using the :ref:`caf_leds`.
For this reason, make sure that both :kconfig:option:`CONFIG_CAF_LEDS` and :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE <config_desktop_app_options>` options are set.

To enable the module, use the :ref:`CONFIG_DESKTOP_LED_STREAM_ENABLE <config_desktop_app_options>` Kconfig option.

You can also define the stream LED event queue size using :ref:`CONFIG_DESKTOP_LED_STREAM_QUEUE_SIZE <config_desktop_app_options>` option.
The queue is used by the module as a data buffer for the data received from the host computer.

Configuration channel
*********************

.. note::
   All the described :ref:`nrf_desktop_config_channel` options are used by the :ref:`nrf_desktop_config_channel_script` during the ``led_stream`` operation.
   You can trigger displaying the LED stream effects using the ``led_stream`` command in CLI.

The module is a :ref:`nrf_desktop_config_channel` listener and provides the following options:

* ``set_led_effect``
    The :ref:`nrf_desktop_config_channel_script` performs the set operation on this option to send the LED effect step that will be displayed by the :ref:`caf_leds`.
    The selected LED is identified using the ``LED ID`` provided in the data received from the host.
    The module queues the received LED effect steps and forwards them to :ref:`caf_leds` one after another.
    See the :ref:`caf_leds` documentation for more detailed information about the LED effect and LED effect step.
* ``get_leds_state``
    The :ref:`nrf_desktop_config_channel_script` performs the fetch operation on this option to get the number of available free places in the queue of LED effect steps for every LED.
    This information can be used, for example, to synchronize the displayed LED effects with music.

    Fetching this option also provides information whether the :ref:`caf_leds` is ready.
    If the device is suspended by :ref:`nrf_desktop_power_manager`, the LEDs are turned off and the effects cannot be displayed.
    You then must wake up the device before displaying the LED stream.

Implementation details
**********************

The module receives LED effects as ``config_event``.
The effects are sent to :ref:`caf_leds` as ``led_event``.
Displaying the sequence begins when the first LED effect is received by the |led_stream|.

Every received LED effect has a predefined duration.
The LEDs module submits ``led_ready_event`` when it finishes displaying a LED effect.
On this event, the |led_stream| sends the next effect from the queue.

When the sequence is active, the host computer keeps sending new effects that are queued by the |led_stream|.
The sequence ends when there are no more effects available in the queue.

LED state interaction
=====================

The :ref:`nrf_desktop_led_state` uses LEDs to display both the state of the system and the state of the connected Bluetooth® peer.
The |led_stream| takes control over the selected LED only when the sequence is displayed.
After the sequence ends, the LED effect selected by the LED state module is restored.

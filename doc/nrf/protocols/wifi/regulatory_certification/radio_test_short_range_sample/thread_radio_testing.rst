.. _ug_thread_radio_testing:

Using the Thread Radio testing
##############################

The following example describes how to perform a continuous transmit on a fixed channel.

.. code-block:: shell

   uart:~$ data_rate ieee802154_250Kbit

Select the lowest channel 11.
Replace 11 with 18 for mid channel.
Replace 11 with 26 for high channel.

.. code-block:: shell

   uart:~$ start_channel 11

Transmit packets continuously with high duty cycle.

.. code-block:: shell

   uart:~$ start_tx_modulated_carrier

Terminate the transmission.

.. code-block:: shell

   uart:~$ cancel

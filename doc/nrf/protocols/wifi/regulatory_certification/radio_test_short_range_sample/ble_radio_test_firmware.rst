.. _ug_wifi_ble_radio_test_firmware:

Using the Bluetooth Low Energy Radio test firmware
##################################################

The following example describes how to perform a continuous transmit on a fixed channel.

Configure the data rate to 1 Mbps and set a random data transmission pattern:

.. code-block:: shell

   uart:~$ data_rate ble_1Mbit
   uart:~$ transmit_pattern pattern_random

Select the lowest channel, which is 2400 MHz:

.. code-block:: shell

   uart:~$ start_channel 0

Transmit packets continuously with high duty cycle:

.. code-block:: shell

   uart:~$ start_tx_modulated_carrier

Terminate the transmission:

.. code-block:: shell

   uart:~$ cancel

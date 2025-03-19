.. _ug_thread_radio_test_for_per_measurements:

Using Thread Radio test for PER measurements
############################################

You can perform a :term:`Packet Error Rate (PER)` measurement using the Radio test application running on two nRF7002 :term:`Development Kit (DK)` or :term:`Evaluation Kit (EK)` devices, where one is used as a transmitter and the other as a receiver.

Configure the receiving device (DK or EK) to receive packets with a known access address at a center frequency of 2400 MHz.

.. code-block:: shell

   uart:~$ data_rate ieee802154_250Kbit
   uart:~$ start_channel 18
   uart:~$ parameters_print
   uart:~$ start_rx

Configure the transmitting device (DK or EK) to transmit 10000 packets (TX transmit count) with the matching access address at center channel 18.

.. code-block:: shell

   uart:~$ data_rate ieee802154_250Kbit
   uart:~$ start_channel 18
   uart:~$ parameters_print
   uart:~$ start_tx_modulated_carrier 10000

Record the number of successfully received packets on the receiving device (DK or EK).
Repeat as necessary until the count stops incrementing.
RX success count is the final item in the print display: ``Number of packets``.

.. code-block:: shell

   uart:~$ print_rx

Terminate receiving on the transmitting device (DK or EK).

.. code-block:: shell

   uart:~$ cancel

Calculate PER using the following equation:

.. math::

    1 - \left( \frac{\text{RX success count}}{\text{TX transmit count}} \right)

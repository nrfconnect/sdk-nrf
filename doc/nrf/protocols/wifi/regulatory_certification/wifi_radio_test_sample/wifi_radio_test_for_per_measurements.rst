.. _ug_wifi_radio_test_for_per_measurements:

Using the Wi-Fi Radio test for PER measurements
###############################################

.. contents::
   :local:
   :depth: 2

You can perform a :term:`Packet Error Rate (PER)` measurement using the Wi-Fi® Radio test sample running on two nRF7002 :term:`Development Kit (DK)` or :term:`Evaluation Kit (EK)` devices, where one is used as a transmitter and the other as a receiver.

PER measurements in 802.11b mode
********************************

Set up and configure the :term:`Device Under Test (DUT)` to perform PER measurements in 802.11b mode.

Configure the receiving DK or EK to receive packets on the required channel number.

Use the following set of commands to configure the DUT in channel 1 and receive mode:

.. code-block:: shell

   uart:~$ wifi_radio_test init 1
   uart:~$ wifi_radio_test rx 1 #this will clear the earlier stats and wait for packets

Configure the transmitting DK or EK to send 10,000 packets (TX transmit count) with the required channel, frame format, and MCS rate, for example, channel 1, 11b, and 1 Mbps.

.. code-block:: shell

   uart:~$ wifi_radio_test init 1
   uart:~$ wifi_radio_test tx_pkt_tput_mode 0
   uart:~$ wifi_radio_test tx_pkt_preamble 0
   uart:~$ wifi_radio_test tx_pkt_rate 1
   uart:~$ wifi_radio_test tx_pkt_len 1024
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num 10000
   uart:~$ wifi_radio_test tx 1

Record the number of successfully received packets on the receiving DK or EK.
Repeat as necessary until the count stops incrementing.
The RX success count is displayed as ``dsss_crc32_pass_cnt``.

.. code-block:: shell

   uart:~$ wifi_radio_test get_stats

Terminate receiving on the transmitting DK or EK:

.. code-block:: shell

   uart:~$ wifi_radio_test rx 0

Calculate PER using the following equation:

.. math::

   1 - \left( \frac{\text{RX success count}}{\text{TX transmit count}} \right)

PER measurements in 802.11g mode
********************************

Set up and configure the DUT to perform PER measurements in 802.11g mode.

Configure the receiving DK or EK to receive packets on the required channel number.

The following set of commands configures the DUT in channel 11, receive mode:

.. code-block:: shell

   wifi_radio_test init 11
   wifi_radio_test rx 1     #this will clear the earlier stats and wait for packets

Configure the transmitting DK or EK to send 10,000 packets (TX transmit count) with the required channel, frame format, and MCS rate, for example, channel 11, 11g, and 54 Mbps.

Change the TX commands.
To prevent lengthy transmission times, keep the interpacket gap at minimum 200 µs.

.. code-block:: shell

   uart:~$ wifi_radio_test init 11
   uart:~$ wifi_radio_test tx_pkt_tput_mode 0
   uart:~$ wifi_radio_test tx_pkt_rate 54
   uart:~$ wifi_radio_test tx_pkt_len 1024
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num 10000
   uart:~$ wifi_radio_test tx 1

Record the number of successfully received packets on the receiving DK or EK.
Repeat as necessary until the count stops incrementing.
The RX success count is displayed as ``ofdm_crc32_pass_cnt``.

.. code-block:: shell

   uart:~$ wifi_radio_test get_stats

Terminate receiving on the transmitting DK or EK:

.. code-block:: shell

   uart:~$ wifi_radio_test rx 0

Calculate PER using the following equation:

.. math::

   1 - \left( \frac{\text{RX success count}}{\text{TX transmit count}} \right)

PER measurements in 802.11a mode
********************************

Set up and configure the DUT to perform PER measurements in 802.11a mode.

Configure the receiving DK or EK to receive packets on the required channel number.
The following set of commands configures the DUT in channel 36, receive mode:

.. code-block:: shell

   wifi_radio_test init 36
   wifi_radio_test rx 1     #this will clear the earlier stats and wait for packets

Configure the transmitting DK or EK to send 10,000 packets (TX transmit count) with the required channel, frame format, and MCS rate, for example, channel 36, 11a, and 54 Mbps.

Change the TX commands.
To prevent lengthy transmission times, keep the interpacket gap at minimum 200 µs.

.. code-block:: shell

   uart:~$ wifi_radio_test init 36
   uart:~$ wifi_radio_test tx_pkt_tput_mode 0
   uart:~$ wifi_radio_test tx_pkt_rate 54
   uart:~$ wifi_radio_test tx_pkt_len 1024
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num 10000
   uart:~$ wifi_radio_test tx 1

Record the number of successfully received packets on the receiving DK or EK.
Repeat as necessary until the count stops incrementing.
The RX success count is displayed as ``ofdm_crc32_pass_cnt``.

.. code-block:: shell

   uart:~$ wifi_radio_test get_stats

Terminate receiving on the transmitting DK or EK:

.. code-block:: shell

   uart:~$ wifi_radio_test rx 0

Calculate PER using the following equation:

.. math::

   1 - \left( \frac{\text{RX success count}}{\text{TX transmit count}} \right)

PER measurements in 802.11n mode
********************************

Set up and configure the DUT to perform PER measurements in 802.11n mode.

Configure the receiving DK or EK to receive packets on the required channel number.
The following set of commands configures the DUT in channel 36, receive mode:

.. code-block:: shell

   uart:~$ wifi_radio_test init 36
   uart:~$ wifi_radio_test rx 1  #this will clear the earlier stats and wait for packets

Configure the transmitting DK or EK to send 10,000 packets (TX transmit count) with the required channel, frame format, and MCS rate, for example, channel 36, 11n, and MCS0.

Change the TX commands.
To prevent lengthy transmission times, keep the interpacket gap at minimum 200 µs.

.. code-block:: shell

   uart:~$ wifi_radio_test init 36
   uart:~$ wifi_radio_test tx_pkt_tput_mode 1
   uart:~$ wifi_radio_test tx_pkt_preamble 2
   uart:~$ wifi_radio_test tx_pkt_mcs 0
   uart:~$ wifi_radio_test tx_pkt_len 4000
   uart:~$ wifi_radio_test tx_pkt_sgi 0
   uart:~$ wifi_radio_test tx_pkt_gap 1000
   uart:~$ wifi_radio_test tx_pkt_num 10000
   uart:~$ wifi_radio_test tx 1

Record the number of successfully received packets on the receiving DK or EK.
Repeat as necessary until the count stops incrementing.
The RX success count is displayed as ``ofdm_crc32_pass_cnt``.

.. code-block:: shell

    uart:~$ wifi_radio_test get_stats

Terminate receiving on the transmitting DK or EK:

.. code-block:: shell

   uart:~$ wifi_radio_test rx 0

Calculate PER using the following equation:

.. math::

   1 - \left( \frac{\text{RX success count}}{\text{TX transmit count}} \right)

PER measurements in 802.11ac mode
*********************************

Set up and configure the DUT to perform PER measurements in 802.11ac mode.

Configure the receiving DK or EK to receive packets on the required channel number.
The following set of commands configures the DUT in channel 40, receive mode:

.. code-block:: shell

   uart:~$ wifi_radio_test init 40
   uart:~$ wifi_radio_test rx 1  #this will clear the earlier stats and wait for packets

Configure the transmitting DK or EK to send 10,000 packets (TX transmit count) with the required channel, frame format, and MCS rate, for example, channel 40, 11ac, and MCS7.

Change the TX commands.
To prevent lengthy transmission times, keep the interpacket gap at minimum 200 µs.

.. code-block:: shell

   uart:~$ wifi_radio_test init 40
   uart:~$ wifi_radio_test tx_pkt_tput_mode 2
   uart:~$ wifi_radio_test tx_pkt_mcs 7
   uart:~$ wifi_radio_test tx_pkt_len 4000
   uart:~$ wifi_radio_test tx_pkt_sgi 0
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num 10000
   uart:~$ wifi_radio_test tx 1

Record the number of successfully received packets on the receiving DK or EK.
Repeat as necessary until the count stops incrementing.
The RX success count is displayed as ``ofdm_crc32_pass_cnt``.

.. code-block:: shell

   uart:~$ wifi_radio_test get_stats

Terminate receiving on the transmitting DK or EK:

.. code-block:: shell

   uart:~$ wifi_radio_test rx 0

Calculate PER using the following equation:

.. math::

   1 - \left( \frac{\text{RX success count}}{\text{TX transmit count}} \right)

PER measurements in 802.11ax mode
*********************************

Set up and configure the DUT to perform PER measurements in 802.11ax mode.

Configure the receiving DK or EK to receive packets on the required channel number.
The following set of commands configures the DUT in channel 100, receive mode:

.. code-block:: shell

   uart:~$ wifi_radio_test init 100
   uart:~$ wifi_radio_test rx 1  #this will clear the earlier stats and wait for packets

Configure the transmitting DK or EK to send 10,000 packets (TX transmit count) with the required channel, frame format, and MCS rate, for example, channel 100, 11ax, and MCS0.

Change the TX commands.
To prevent lengthy transmission times, keep the interpacket gap at minimum 200 µs.

.. code-block:: shell

   uart:~$ wifi_radio_test init 100
   uart:~$ wifi_radio_test tx_pkt_tput_mode 3
   uart:~$ wifi_radio_test tx_pkt_mcs 0
   uart:~$ wifi_radio_test tx_pkt_len 4000
   uart:~$ wifi_radio_test he_ltf 2
   uart:~$ wifi_radio_test he_gi 2
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num 10000
   uart:~$ wifi_radio_test tx 1

Record the number of successfully received packets on the receiving DK or EK.
Repeat as necessary until the count stops incrementing.
The RX success count is displayed as ``ofdm_crc32_pass_cnt``.

.. code-block:: shell

   uart:~$ wifi_radio_test get_stats

Terminate receiving on the transmitting DK or EK:

.. code-block:: shell

   uart:~$ wifi_radio_test rx 0

Calculate PER using the following equation:

.. math::

   1 - \left( \frac{\text{RX success count}}{\text{TX transmit count}} \right)

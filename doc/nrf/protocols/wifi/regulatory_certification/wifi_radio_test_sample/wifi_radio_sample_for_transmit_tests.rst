.. _ug_wifi_radio_sample_for_transmit_tests:

Using the Wi-Fi Radio sample for transmit tests
###############################################

.. contents::
   :local:
   :depth: 2

Set the transmit parameters and modes with the correct order of commands.
The following examples show the input and sequence of commands for the different Wi-Fi® modes.

Order of commands
*****************

The order of the Wi-Fi Radio sub-commands is very important.

Use the following order of sub-commands:

1. ``init`` - Disables any ongoing TX or RX testing and sets all configured parameters to default.
#. ``tx_pkt_tput_mode`` - Sets the frame format of the transmitted packet.

   other commands...
#. ``ru_tone`` - Configures the Resource Unit (RU) size.
#. ``ru_index`` - Configures the location of RU in 20 MHz spectrum.
#. ``tx_pkt_len`` - Packet data length to be used for the TX stream.

   other commands...
#. ``tx`` - Starts transmit.

   .. note::
      Use the command to start transmit (``tx 1``) only after all the parameters are configured.
      Further changes in TX parameters are not allowed during TX transmission.

The following example of HE TB packet transmit illustrates the required order of the commands.

The first and second sub-commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 100
   uart:~$ wifi_radio_test tx_pkt_tput_mode 5

When used, the following three sub-commands must be in this order:

.. code-block:: shell

   uart:~$ wifi_radio_test ru_tone 106
   uart:~$ wifi_radio_test ru_index 2
   uart:~$ wifi_radio_test tx_pkt_len 1024

These sub-commands can be given in any order:

.. code-block:: shell

   uart:~$ wifi_radio_test tx_pkt_mcs 7
   uart:~$ wifi_radio_test he_ltf 2
   uart:~$ wifi_radio_test he_gi 2
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num -1

The last sub-command:

.. code-block:: shell

   uart:~$ wifi_radio_test tx 1

Adjusting TX power
******************

For any Wi-Fi mode and modulation, the regulatory domain control is enabled as a default setting where the TX power defaults to :math:`TX_\text{max}`.
This is the lower of the nRF70 Series Product Specification value and the maximum value set by the regulatory domain for the specified channel.

To set the transmission power to a specific value, regardless of product specification or regulatory domain limits, you must first disable the regulatory domain settings.
Then, use the ``tx_power`` command to set the required transmit power.
``<X>`` is the transmit power expressed in dBm in the range 0 to 24.

.. code-block:: shell

   uart:~$ wifi_radio_test bypass_reg_domain 1
   uart:~$ wifi_radio_test tx_power <X>

The TX power cannot be set below :math:`TX_\text{max}` without disabling the regulatory domain control.

.. note::
   For regulatory certification type testing, both regulatory domain control and TX power should use default settings, meaning that the commands described above should not be issued.

TX inter-packet gap
*******************

The ``wifi_radio_test init`` commands reset the inter-packet gap to ``0``.
To explicitly set the inter-packet gap to ``0``, use the following command:

.. code-block:: shell

   $ wifi_radio_test tx_pkt_gap 0

Running a continuous DSSS/CCK TX traffic sequence in 11b mode
=============================================================

Set the parameters to run a continuous DSSS/CCK TX traffic sequence in 802.11b mode.

* Channel: 1
* Payload length: 1024 bytes
* Inter-frame gap: 8600 µs
* Data rate: 1 Mbps
* Long preamble

This configuration gives a frame duration of 8624 µs and an achieved duty-cycle of 50.07%.

Execute the following sequence of commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 1
   uart:~$ wifi_radio_test tx_pkt_tput_mode 0
   uart:~$ wifi_radio_test tx_pkt_preamble 0
   uart:~$ wifi_radio_test tx_pkt_rate 1
   uart:~$ wifi_radio_test tx_pkt_len 1024
   uart:~$ wifi_radio_test tx_pkt_gap 8600
   uart:~$ wifi_radio_test tx_pkt_num -1
   uart:~$ wifi_radio_test tx 1

Running a continuous OFDM TX traffic sequence in 11g mode
=========================================================

Set the parameters to run a continuous OFDM TX traffic sequence in 11g mode.

* Channel: 11
* Payload length: 4000 bytes
* Inter-frame gap: 200 µs
* Data rate: 6 Mbps

This configuration gives a frame duration of 5400 µs and an achieved duty-cycle of 96.4%.

Execute the following sequence of commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 11
   uart:~$ wifi_radio_test tx_pkt_tput_mode 0
   uart:~$ wifi_radio_test tx_pkt_rate 6
   uart:~$ wifi_radio_test tx_pkt_len 4000
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num -1
   uart:~$ wifi_radio_test tx 1

Running a continuous OFDM TX traffic sequence in 11a mode
=========================================================

Set the parameters to run a continuous OFDM TX traffic sequence in 11a mode.

* Channel: 40
* Payload length: 4000 bytes
* Inter-frame gap: 200 µs
* Data rate: 54 Mbps

This configuration gives a frame duration of 620 µs and an achieved duty-cycle of 75.6%.

Execute the following sequence of commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 40
   uart:~$ wifi_radio_test tx_pkt_tput_mode 0
   uart:~$ wifi_radio_test tx_pkt_rate 54
   uart:~$ wifi_radio_test tx_pkt_len 4000
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num -1
   uart:~$ wifi_radio_test tx 1

Running a continuous OFDM TX traffic sequence in HT 11n mode
============================================================

Set the parameters to run a continuous OFDM TX traffic sequence in HT 11n mode.

* Channel: 11
* Frame format: HT (11n)
* Payload length: 4000 bytes
* Inter-frame gap: 200 µs
* Data rate: MCS7
* Long guard

This configuration gives a frame duration of 536 µs and an achieved duty-cycle of 72.8%.

Execute the following sequence of commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 11
   uart:~$ wifi_radio_test tx_pkt_tput_mode 1
   uart:~$ wifi_radio_test tx_pkt_preamble 2
   uart:~$ wifi_radio_test tx_pkt_mcs 7
   uart:~$ wifi_radio_test tx_pkt_len 4000
   uart:~$ wifi_radio_test tx_pkt_sgi 0
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num -1
   uart:~$ wifi_radio_test tx 1

Running a continuous OFDM TX traffic sequence in VHT 11ac mode
==============================================================

Set the parameters to run a continuous OFDM TX traffic sequence in VHT 11ac mode.

* Channel: 40
* Frame format: VHT (11ac)
* Payload length: 4000 bytes
* Inter-frame gap: 200 µs
* Data rate: MCS7
* Long guard

This configuration gives a frame duration of 540 µs and an achieved duty-cycle of 73%.

Execute the following sequence of commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 40
   uart:~$ wifi_radio_test tx_pkt_tput_mode 2
   uart:~$ wifi_radio_test tx_pkt_mcs 7
   uart:~$ wifi_radio_test tx_pkt_len 4000
   uart:~$ wifi_radio_test tx_pkt_sgi 0
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num -1
   uart:~$ wifi_radio_test tx 1

Running a continuous OFDM TX traffic sequence in HE-SU 11ax mode
================================================================

Set the parameters to run a continuous OFDM TX traffic sequence in HE-SU 11ax mode.

* Channel: 116
* Frame format: HESU (11ax)
* Payload length: 4000 bytes
* Inter-frame gap: 200 µs
* Data rate: MCS7
* 3.2 µs GI
* 4xHELTF

This configuration gives a frame duration of 488 µs and an achieved duty-cycle of 70.9%.

Execute the following sequence of commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 116
   uart:~$ wifi_radio_test tx_pkt_tput_mode 3
   uart:~$ wifi_radio_test tx_pkt_mcs 7
   uart:~$ wifi_radio_test tx_pkt_len 4000
   uart:~$ wifi_radio_test he_ltf 2
   uart:~$ wifi_radio_test he_gi 2
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num -1
   uart:~$ wifi_radio_test tx 1

Running a continuous OFDM TX traffic sequence in HE-ER-SU 11ax mode
===================================================================

Set the parameters to run a continuous OFDM TX traffic sequence in HE-ER-SU 11ax mode.

* Channel: 100
* Frame format: HE-ERSU (11ax)
* Payload length: 1000 bytes
* Inter-frame gap: 200 µs
* Data rate: MCS0
* 3.2 µs GI
* 4xHELTF

This configuration gives a frame duration of 1184 µs and an achieved duty-cycle of 85.5%.

Execute the following sequence of commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 100
   uart:~$ wifi_radio_test tx_pkt_tput_mode 4
   uart:~$ wifi_radio_test tx_pkt_mcs 0
   uart:~$ wifi_radio_test tx_pkt_len 1000
   uart:~$ wifi_radio_test he_ltf 2
   uart:~$ wifi_radio_test he_gi 2
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num -1
   uart:~$ wifi_radio_test tx 1

Running a continuous OFDM TX traffic sequence in HE-TB-PPDU 11ax mode
=====================================================================

Set the parameters to run a continuous OFDM TX traffic sequence in HE-TB-PPDU 11ax mode.

* Channel: 100
* Frame format: HE-TB (11ax)
* Payload length: 1024 bytes
* Inter-frame gap: 200 µs
* Data rate: MCS7
* 3.2 µs GI
* 106 Tone
* 4xHELTF
* RU Index 2

This configuration gives a frame duration of 332 µs and an achieved duty-cycle of 62.4%.

Execute the following sequence of commands:

.. code-block:: shell

   uart:~$ wifi_radio_test init 100
   uart:~$ wifi_radio_test tx_pkt_tput_mode 5
   uart:~$ wifi_radio_test tx_pkt_mcs 7
   uart:~$ wifi_radio_test ru_tone 106
   uart:~$ wifi_radio_test ru_index 2
   uart:~$ wifi_radio_test tx_pkt_len 1024
   uart:~$ wifi_radio_test he_ltf 2
   uart:~$ wifi_radio_test he_gi 2
   uart:~$ wifi_radio_test tx_pkt_gap 200
   uart:~$ wifi_radio_test tx_pkt_num -1
   uart:~$ wifi_radio_test tx 1

.. note::
   The value of ``tx_pkt_len`` depends on ``ru_tone``.
   For each ``ru_tone`` value, the following maximum limits for ``tx_pkt_len`` apply:

   * 26 tone - 350 bytes
   * 52 tone - 800 bytes
   * 106 tone - 1800 bytes
   * 242 tone - 4000 bytes

   Ensure that the packet length does not exceed the specified limits for the selected RU configuration.

At any point in time, you can use the following command to verify the configurations set.
Use the command before setting ``tx`` or ``rx`` to ``1``:

.. code-block:: shell

   uart:~$ wifi_radio_test show_config

To set payload parameters for a maximum duty cycle, assume a 200 µs inter-packet gap and set ``tx_pkt_len`` to the following:

* 11b - 1Mbps: 1024 (97% duty cycle)
* OFDM - 6Mbps/MCS0: 4000 (> 95% duty cycle)

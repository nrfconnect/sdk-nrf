.. _wifi_radio_test:

Wi-Fi: Radio test
#################

.. contents::
   :local:
   :depth: 2

The Wi-Fi Radio test sample demonstrates how to configure the Wi-Fi radio in a specific mode and then test its performance.
The sample provides a set of predefined commands that allow you to configure the radio in the following modes:

* Modulated carrier TX
* Modulated carrier RX

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

To run the tests, connect to the development kit through the serial port and send shell commands.
Zephyr's :ref:`zephyr:shell_api` module is used to handle the commands.

You can start running ``wifi_radio_test`` subcommands to set up and control the radio.
See :ref:`wifi_radio_test_subcmds` for a list of available subcommands.

In the Modulated carrier RX mode, you can use the ``get_stats`` subcommand to display the statistics.
See :ref:`wifi_radio_test_stats` for a list of available statistics.

User interface
**************

``wifi_radio_test`` is the Wi-Fi radio test command and it supports the following subcommands:

.. _wifi_radio_test_subcmds:

Wi-Fi radio test subcommands
============================

.. list-table:: Wi-Fi radio test subcommands
   :header-rows: 1

   * - Subcommand
     - Argument
     - Description
   * - phy_calib_rxdc
     - | 0 – Disable
       | 1 – Enable
     - Enable/Disable RX DC calibration.
   * - phy_calib_txdc
     - | 0 – Disable
       | 1 – Enable
     - Enable/Disable TX DC calibration.
   * - phy_calib_txpow
     - | 0 – Disable
       | 1 – Enable
     - Enable/Disable TX power calibration.
   * - phy_calib_rxiq
     - | 0 – Disable
       | 1 – Enable
     - Enable/Disable RX IQ calibration.
   * - phy_calib_txiq
     - | 0 – Disable
       | 1 – Enable
     - Enable/Disable TX IQ calibration.
   * - he_ltf
     - | 0 – 1x HE LTF
       | 1 – 2x HE LTF
       | 2 – 3x HE LTF
     - Configure HE long training field (LTF) value while transmitting the packet.
   * - he_gi
     - | 0 – 0.8 us
       | 1 – 1.6 us
       | 2 – 3.2 us
     - Configure HE guard interval (GI) while transmitting the packet.
   * - rf_params
     - Hex value string
     - Hexadecimal value string for RF related parameters. See :ref:`wifi_radio_test_rf_params` for the description.
   * - tx_pkt_tput_mode
     - | 0 – Legacy
       | 1 – HT mode
       | 2 – VHT mode
       | 3 – HE (SU) mode
       | 4 – HE (ERSU) mode
     - Throughput mode to be used for transmitting the packet.
   * - tx_pkt_sgi
     - | 0 – Disable
       | 1 – Enable
     - Enable/Disable Short guard interval (GI) while transmitting the packet.
   * - tx_pkt_preamble
     - | 0 – Short Preamble
       | 1 – Long Preamble
       | 2 – Mixed Preamble
     - Type of preamble to be used for each packet. Short/Long Preamble are applicable only when tx_pkt_tput_mode is set to Legacy and Mixed Preamble is applicable only when tx_pkt_tput_mode is set to HT/VHT.
   * - tx_pkt_mcs
     - | -1 – Not being used
       | <val> – MCS index to be used
     - MCS index at which TX packet will be transmitted. Mutually exclusive with tx_pkt_rate.
   * - tx_pkt_rate
     - | -1 – Not being used
       | <val> – Legacy rate to be used (1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54)
     - Legacy rate at which packets will be transmitted. Mutually exclusive with tx_pkt_mcs.
   * - tx_pkt_gap
     - <val> - (Min: 200, Max: 200000, Default: 200)
     - Interval between TX packets in microseconds.
   * - chnl_primary
     - <val> - Primary channel number (Default: 1)
     - Configures the Primary channel to be used.
   * - tx_pkt_num
     - | -1 – Transmit infinite packets
       | <val> – Number of packets to transmit
     - Number of packets to transmit before stopping. Applicable only when tx_mode is set to Regular Tx.
   * - tx_pkt_len
     - <val> – Desired packet length (Default: 1400)
     - Packet data length to be used for the TX stream.
   * - tx_power
     - <val> – Transmit power in dBm.
     - Transmit power for frame transmission.
   * - tx
     - | 0 – Disable
       | 1 – Enable
     - Enable/Disable packet transmission. Transmits configured number of packets (tx_pkt_num) of packet length (tx_pkt_len).
   * - rx
     - | 0 – Disable
       | 1 – Enable
     - Enable/Disable packet reception.
   * - show_config
     - N/A
     - Display the current configuration values.
   * - get_stats
     - N/A
     - Display statistics.


.. _wifi_radio_test_stats:

Wi-Fi radio test statistics
===========================

.. list-table:: Wi-Fi radio test statistics
   :header-rows: 1

   * - Statistic
     - Description
   * - rssi_avg
     - Average RSSI value in dBm.
   * - ofdm_crc32_pass_cnt
     - Number of OFDM frames whose CRC32 check passed.
   * - ofdm_crc32_fail_cnt
     - Number of OFDM frames whose CRC32 check failed.
   * - dsss_crc32_pass_cnt
     - Number of DSSS frames whose CRC32 check passed.
   * - dsss_crc32_fail_cnt
     - Number of DSSS frames whose CRC32 check failed.


.. _wifi_radio_test_rf_params:

RF parameters
=============

.. list-table:: RF parameters
   :header-rows: 1

   * - Byte(s)
     - Type
     - Units
     - Description
   * - 0 - 5
     - NA
     - NA
     - Reserved.
   * - 6
     - Unsigned
     - NA
     - XO adjustment.
   * - 7 - 10
     - Signed
     - 0.25 dB
     - Power detector adjustment for MCS7 for channel 7, 36, 100 and 165.
   * - 11 - 14
     - Signed
     - 0.25 dB
     - Power detector adjustment for MCS0 for channel 7, 36, 100 and 165.
   * - 15
     - Signed
     - 0.25 dBm
     - Max output power for 11b for channel 7.
   * - 16 - 17
     - Signed
     - 0.25 dBm
     - Max output power for MCS7 and MCS0 for channel 7.
   * - 18 - 20
     - Signed
     - 0.25 dBm
     - Max output power for MCS7 for channel 36, 100 and 165.
   * - 21 - 23
     - Signed
     - 0.25 dBm
     - Max output power for MCS0 for channel 36, 100 and 165.
   * - 24 - 27
     - Signed
     - 0.25 dBm
     - Rx-Gain offset for channel 7, 36, 100 and 165.
   * - 28
     - Signed
     - degree Celsius
     - Maximum chip temperature.
   * - 29
     - Signed
     - degree Celsius
     - Minimum chip temperature.
   * - 30
     - Signed
     - 0.25 dB
     - TX Power backoff at high temperature (+80 degree Celsius) in 2.4G.
   * - 31
     - Signed
     - 0.25 dB
     - TX Power backoff at low temperature (-20 degree Celsius) in 2.4G.
   * - 32
     - Signed
     - 0.25 dB
     - TX Power backoff at high temperature (+80 degree Celsius) in 5G.
   * - 33
     - Signed
     - 0.25 dB
     - TX Power backoff at low temperature (-20 degree Celsius) in 5G.
   * - 34 - 41
     - Signed
     - 0.25 dBm
     - Voltage related power backoff.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/radio_test`

.. include:: /includes/build_and_run.txt

Currently, the following configurations are supported:

* 7002 DK + QSPI
* 7002 EK + SPIM


To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp

To build for the nRF7002 EK, use the ``nrf7002dk_nrf5340_cpuapp`` build target with the ``SHIELD`` CMake option set to ``nrf7002_ek``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002_ek

See also :ref:`cmake_options` for instructions on how to provide CMake options.


Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Test the sample by running the following commands:

   a. To display the current configuration, use the following command:

      .. code-block:: console

          wifi_radio_test show_config

      The sample shows the following output:

      .. code-block:: console

          ************* Configured Parameters ***********
          rf_params = 00 00 00 00 00 00 2C 00 00 00 00 00 00 00 00 30 20 30 20 20 20 30 30 30 00 00 00 00 50 EC 00 00 00 00 00 00 00 00 00 00 00 00
          tx_pkt_tput_mode = 0
          tx_pkt_sgi = 0
          tx_pkt_preamble = 0
          tx_pkt_mcs = -1
          tx_pkt_rate = -1
          tx_pkt_gap = 200
          phy_calib_rxdc = 1
          phy_calib_txdc = 1
          phy_calib_txpow = 1
          phy_calib_rxiq = 1
          phy_calib_txiq = 1
          chnl_primary = 1
          tx_pkt_num = -1
          tx_pkt_len = 1400
          tx_power = 0
          he_ltf = 0
          he_gi = 0
          tx = 0
          rx = 0

   #. To run a continuous Orthogonal frequency-division mulitplexing (OFDM) TX traffic sequence with the following configuration:

      * Channel: 14
      * Frame duration: 5484 us
      * Inter-frame gap: 4200 us

      Execute the following sequence of commands:

      .. code-block:: console

          wifi_radio_test rx 0
          wifi_radio_test tx 0
          wifi_radio_test tx_pkt_tput_mode 0
          wifi_radio_test tx_pkt_len 4095
          wifi_radio_test chnl_primary 14
          wifi_radio_test tx_pkt_rate 6
          wifi_radio_test tx_power 0
          wifi_radio_test tx_pkt_gap 4200
          wifi_radio_test tx 1


   #. To run a continuous Direct-sequence spread spectrum (DSSS) TX traffic sequence with the following configuration:

      * Channel: 1
      * Frame duration: 8500 us
      * Inter-frame gap: 8600 us

      Execute the following sequence of commands:

      .. code-block:: console

          wifi_radio_test rx 0
          wifi_radio_test tx 0
          wifi_radio_test tx_pkt_tput_mode 0
          wifi_radio_test tx_pkt_len 1024
          wifi_radio_test chnl_primary 1
          wifi_radio_test tx_pkt_rate 1
          wifi_radio_test tx_power 0
          wifi_radio_test tx_pkt_gap 8600
          wifi_radio_test tx 1

.. note::

   * For regulatory certification, it is advisable to run the TX streams in Legacy OFDM or DSSS modes only (``wifi_radio_test tx_pkt_tput_mode 0``).
   * The frame duration can be calculated using the formula::

         D = ((L * 8) / R ) + P

      where::

         D : Frame duration (us)
         L : Frame length (bytes)
         R : Data rate (Mbps)
         P : PHY overhead duration (us) (Values: 24us - Legacy OFDM, 192us - DSSS)

Dependencies
************

This sample uses the following Zephyr library:

* :ref:`zephyr:shell_api`:

  * ``include/shell/shell.h``

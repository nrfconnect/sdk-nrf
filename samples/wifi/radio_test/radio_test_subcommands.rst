.. _wifi_radio_subcommands:

Radio test subcommands
######################

.. contents::
   :local:
   :depth: 2

``wifi_radio_test`` is the Wi-Fi radio test command and it supports the following subcommands.

.. _wifi_radio_test_subcmds:

Wi-Fi radio test subcommands
****************************

.. list-table:: Wi-Fi radio test subcommands
   :header-rows: 1

   * - Subcommand
     - Argument
     - Description
   * - phy_calib_rxdc
     - | 0 - Disable
       | 1 - Enable
     - Enable/Disable RX DC calibration.
   * - phy_calib_txdc
     - | 0 - Disable
       | 1 - Enable
     - Enable/Disable TX DC calibration.
   * - phy_calib_txpow
     - | 0 - Disable
       | 1 - Enable
     - Enable/Disable TX power calibration.
   * - phy_calib_rxiq
     - | 0 - Disable
       | 1 - Enable
     - Enable/Disable RX IQ calibration.
   * - phy_calib_txiq
     - | 0 - Disable
       | 1 - Enable
     - Enable/Disable TX IQ calibration.
   * - he_ltf
     - | 0 - 1x HE LTF
       | 1 - 2x HE LTF
       | 2 - 3x HE LTF
     - Configure HE long training field (LTF) value while transmitting the packet.
   * - he_gi
     - | 0 - 0.8 us
       | 1 - 1.6 us
       | 2 - 3.2 us
     - Configure HE guard interval (GI) while transmitting the packet.
   * - rf_params
     - Hex value string
     - Hexadecimal value string for RF related parameters. See :ref:`wifi_radio_test_rf_params` for the description.
   * - tx_pkt_tput_mode
     - | 0 - Legacy
       | 1 - HT mode
       | 2 - VHT mode
       | 3 - HE (SU) mode
       | 4 - HE (ERSU) mode
     - Throughput mode to be used for transmitting the packet.
   * - tx_pkt_sgi
     - | 0 - Disable
       | 1 - Enable
     - Enable/Disable Short guard interval (GI) while transmitting the packet.
   * - tx_pkt_preamble
     - | 0 - Short Preamble
       | 1 - Long Preamble
       | 2 - Mixed Preamble
     - Type of preamble to be used for each packet. Short/Long Preamble are applicable only when tx_pkt_tput_mode is set to Legacy and Mixed Preamble is applicable only when tx_pkt_tput_mode is set to HT/VHT.
   * - tx_pkt_mcs
     - | -1 - Not being used
       | <val> - MCS index to be used
     - MCS index at which TX packet will be transmitted. Mutually exclusive with tx_pkt_rate.
   * - tx_pkt_rate
     - | -1 - Not being used
       | <val> - Legacy rate to be used (1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54)
     - Legacy rate at which packets will be transmitted. Mutually exclusive with tx_pkt_mcs.
   * - tx_pkt_gap
     - <val> - (Min: 200, Max: 200000, Default: 200)
     - Interval between TX packets in microseconds.
   * - chnl_primary
     - <val> - Primary channel number (Default: 1)
     - Configures the Primary channel to be used.
   * - tx_pkt_num
     - | -1 - Transmit infinite packets
       | <val> - Number of packets to transmit
     - Number of packets to transmit before stopping. Applicable only when tx_mode is set to Regular Tx.
   * - tx_pkt_len
     - <val> - Desired packet length (Default: 1400)
     - Packet data length to be used for the TX stream.
   * - tx_power
     - <val> - Transmit power in dBm.
     - Transmit power for frame transmission.
   * - tx
     - | 0 - Disable
       | 1 - Enable
     - Enable/Disable packet transmission. Transmits configured number of packets (tx_pkt_num) of packet length (tx_pkt_len).
   * - rx
     - | 0 - Disable
       | 1 - Enable
     - Enable/Disable packet reception.
   * - show_config
     - N/A
     - Display the current configuration values.
   * - get_stats
     - N/A
     - Display statistics.


.. _wifi_radio_test_stats:

Wi-Fi radio test statistics
***************************

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
*************

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

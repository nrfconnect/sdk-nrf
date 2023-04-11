.. _wifi_radio_subcommands:

Radio test subcommands
######################

.. contents::
   :local:
   :depth: 2

``wifi_radio_test`` is the Wi-Fi® radio test command and it supports the following subcommands.

.. _wifi_radio_test_subcmds:

Wi-Fi radio test subcommands
****************************

.. list-table:: Wi-Fi radio test subcommands
   :header-rows: 1

   * - Subcommand
     - Argument
     - Default value
     - Type
     - Description
   * - set_defaults
     - N/A
     - N/A
     - Configuration
     - Reset all configuration parameters to their default values.
   * - phy_calib_rxdc
     - | 0 - Disable
       | 1 - Enable
     - 1
     - Configuration
     - Enable/Disable RX DC calibration.
   * - phy_calib_txdc
     - | 0 - Disable
       | 1 - Enable
     - 1
     - Configuration
     - Enable/Disable TX DC calibration.
   * - phy_calib_txpow
     - | 0 - Disable
       | 1 - Enable
     - 0
     - Configuration
     - Enable/Disable TX power calibration.
   * - phy_calib_rxiq
     - | 0 - Disable
       | 1 - Enable
     - 1
     - Configuration
     - Enable/Disable RX IQ calibration.
   * - phy_calib_txiq
     - | 0 - Disable
       | 1 - Enable
     - 1
     - Configuration
     - Enable/Disable TX IQ calibration.
   * - he_ltf
     - | 0 - 1x HE LTF
       | 1 - 2x HE LTF
       | 2 - 4x HE LTF
     - 2
     - Configuration
     - Configure HE long training field (LTF) value while transmitting the packet.
   * - he_gi
     - | 0 - 0.8 us
       | 1 - 1.6 us
       | 2 - 3.2 us
     - 2
     - Configuration
     - Configure HE guard interval (GI) while transmitting the packet.
   * - tx_pkt_tput_mode
     - | 0 - Legacy
       | 1 - HT mode
       | 2 - VHT mode
       | 3 - HE (SU) mode
       | 4 - HE (ERSU) mode
       | 5 - HE (TB) mode
     - 0
     - Configuration
     - Throughput mode to be used for transmitting the packet.
   * - tx_pkt_sgi
     - | 0 - Disable
       | 1 - Enable
     - 0
     - Configuration
     - Enable/Disable Short guard interval (GI) while transmitting the packet.
   * - tx_pkt_preamble
     - | 0 - Short Preamble
       | 1 - Long Preamble
       | 2 - Mixed Preamble
     - 1
     - Configuration
     - Type of preamble to be used for each packet. Short/Long Preamble are applicable only when tx_pkt_tput_mode is set to Legacy and Mixed Preamble is applicable only when tx_pkt_tput_mode is set to HT/VHT.
   * - tx_pkt_mcs
     - | -1 - Not being used
       | <val> - MCS index to be used
     - 0
     - Configuration
     - MCS index at which TX packet will be transmitted. Mutually exclusive with tx_pkt_rate.
   * - tx_pkt_rate
     - | -1 - Not being used
       | <val> - Legacy rate to be used (1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54)
     - 6
     - Configuration
     - Legacy rate at which packets will be transmitted. Mutually exclusive with tx_pkt_mcs.
   * - tx_pkt_gap
     - <val> - (Min: 0, Max: 200000)
     - 0
     - Configuration
     - Interval between TX packets in microseconds.
   * - tx_pkt_num
     - | -1 - Transmit infinite packets
       | <val> - Number of packets to transmit
     - -1
     - Configuration
     - Number of packets to transmit before stopping.
   * - tx_pkt_len
     - <val> - Desired packet length
     - 1400
     - Configuration
     - Packet data length to be used for the TX stream.
   * - tx_power
     - <val> - Transmit power in dBm (Min: 0, Max: 24)
     - 0
     - Configuration
     - Transmit power for frame transmission.
   * - ru_tone
     - <val> – Desired resource unit (RU) size (26, 52, 106 or 242).
     - 26
     - Configuration
     - Configure the resource unit (RU) size.
   * - ru_index
     - | <val> – Valid values:
       | For 26 ru_tone: 1 to 9
       | For 52 ru_tone: 1 to 4
       | For 106 ru_tone: 1 to 2
       | For 242 ru_tone: 1
     - 1
     - Configuration
     - Configure the location of resource unit (RU) in 20 MHz spectrum.
   * - rx_capture_length
     - | <val> (Min: 0, Max: 16384)
     - 0
     - Configuration
     - Number of RX samples to be captured.
   * - rx_lna_gain
     - | 0 = 24 dB
       | 1 = 18 dB
       | 2 = 12 dB
       | 3 = 0 dB
       | 4 = -12 dB
     - 0
     - Configuration
     - LNA gain to be configured.
   * - rx_bb_gain
     - | <val>
       | 5 bit value. Supports 64 dB range in steps of 2 dB
     - 0
     - Configuration
     - Baseband gain to be configured.
   * - tx_tone_freq
     - | <val> (Min: -10, Max: 10)
     - 0
     - Configuration
     - Transmit tone frequency in the range of -10 MHz to 10 MHz.
   * - dpd
     - | 0 - DPD bypass
       | 1 - Enable DPD
     - 0
     - Configuration
     - Enable or bypass DPD.
   * - set_xo_val
     - | <val> - XO value (Min:0, Max: 127)
     - 42 or value programmed in OTP
     - Configuration
     - Set XO value.
   * - show_config
     - N/A
     - N/A
     - Configuration
     - Display the current configuration values.
   * - init
     - <val> - Primary channel number
     - 1
     - Action
     - Initialize the radio to a default state with the configured channel. This will also reset all other configuration parameters to their default values.
   * - tx
     - | 0 - Disable
       | 1 - Enable
     - 0
     - Action
     - Enable/Disable packet transmission. Transmits configured number of packets (tx_pkt_num) of packet length (tx_pkt_len).
   * - rx
     - | 0 - Disable
       | 1 - Enable
     - 0
     - Action
     - Enable/Disable packet reception.
   * - rx_cap
     - | 0 = ADC capture
       | 1 = Static packet capture
       | 2 = Dynamic packet capture
     - N/A
     - Action
     - Capture RX ADC samples, static or dynamic packets.
   * - tx_tone
     - | 0: Disable tone
       | 1: Enable tone
     - 0
     - Action
     - Enable/Disable transmit tone.
   * - get_temperature
     - | No arguments required
     - N/A
     - Action
     - Get temperature.
   * - get_rf_rssi
     - | No arguments required
     - N/A
     - Action
     - Get RF RSSI.
   * - compute_optimal_xo_val
     - N/A
     - N/A
     - Action
     - Compute optimal XO value. Note: This is still experimental and to be used at own risk.
   * - get_stats
     - N/A
     - N/A
     - Action
     - Display statistics.
   * - tx_pkt_cw
     - <val> - Contention window value to be used (0, 3, 7, 15, 31, 63, 127, 255, 511, 1023).
     - 15
     - Configuration
     - Contention window for transmitted packets.
   * - reg_domain
     - <country code> – Desired country code(for example: NO, US, GB, IN).
     - 00 (World regulatory)
     - Action
     - Configure WLAN regulatory domain country code.
   * - bypass_reg_domain
     - | 0: Use reg_domain
       | 1: Do not use reg_domain
     - 0
     - Configuration
     - Configure WLAN to bypass current regulatory domain in TX test.




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

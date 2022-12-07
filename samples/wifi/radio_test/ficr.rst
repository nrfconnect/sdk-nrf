.. _wifi_ficr_prog:

FICR programming subcommands
############################

``wifi_radio_ficr_prog`` is the Wi-Fi radio FICR programming command and it supports the following subcommands.

.. _wifi_radio_ficr_prog_subcmds:

Wi-Fi radio FICR subcommands
****************************

.. list-table:: Wi-Fi radio FICR subcommands
   :widths: 15 15 10 30 70
   :header-rows: 1

   * - Subcommand
     - Register
     - Offset
     - Argument(s)
     - Description
   * - otp_get_status
     - N/A
     - N/A
     - N/A
     - Reads out the OTP status of user region and status of each field (programmed or not).
   * - otp_read_params
     - N/A
     - N/A
     - N/A
     - Reads out all the OTP parameters (excluding QSPI_KEY which cannot be read).
   * - otp_write_params
     - REGION_PROTECT
     - 0x100
     - arg
     - | arg = 0x50FA50FA : Enable R/W permission
       | arg = 0x00000000 : Lock Region from Writing
       | arg = Others : Invalid
       | All four REGION_PROTECT registers are written with this 32-bit argument.
   * - otp_write_params
     - QSPI_KEY
     - 0x110
     - arg1 arg2 arg3 arg4
     - All four key arguments are 32-bit values forming the required 128-bit (Q)SPI key for encryption.
   * - otp_write_params
     - MAC_ADDRESS0
     - 0x120
     - arg1 arg2
     - If MAC address is AA:BB:CC:DD:EE:FF, then arg1=0xDDCCBBAA and arg2=0xFFEE.
   * - otp_write_params
     - MAC_ADDRESS1
     - 0x128
     - arg1 arg2
     - If MAC address is AA:BB:CC:DD:EE:FF, then arg1=0xDDCCBBAA and arg2=0xFFEE.
   * - otp_write_params
     - CALIB_XO
     - 0x130
     - arg
     - arg is 7-bit unsigned XO value. Bits [31:7] unused. Adjusts capacitor bank, 0 : Lowest capacitance (Highest frequency), 255 : Highest capacitance (Lowest frequency).
   * - otp_write_params
     - CALIB_PDADJM7
     - 0x134
     - arg
     - arg is 32-bit power detector adjustment for MCS7 for channel 7, 36, 100, 165 (channel 7 in bits[7:0] - signed).
   * - otp_write_params
     - CALIB_PDADJM0
     - 0x138
     - arg
     - arg is 32-bit power detector adjustment for MCS0 for channel 7, 36, 100, 165 (channel 7 in bits[7:0] - signed).
   * - otp_write_params
     - CALIB_PWR2G
     - 0x13C
     - arg
     - arg is 32-bit value specifying maximum output power for DSSS, MCS0, MCS7 (DSSS in bits[7:0] - unsigned). Measured in channel 7. Bits [31:24] unused.
   * - otp_write_params
     - CALIB_PWR5GM7
     - 0x140
     - arg
     - arg is 32-bit value specifying maximum output power for MCS7 in channel 36, 100, 165 (channel 36 in bits[7:0] - unsigned). Bits [31:24] unused.
   * - otp_write_params
     - CALIB_PWR5GM0
     - 0x144
     - arg
     - arg is 32-bit value specifying maximum output power for MCS0 in channel 36, 100, 165 (channel 36 in bits[7:0] - unsigned). Bits [31:24] unused.
   * - otp_write_params
     - CALIB_RXGNOFF
     - 0x148
     - arg
     - arg is 32-bit value specifying RX gain offset for channel 7, 36, 100, 165 (channel 7 in bits[7:0] - signed).
   * - otp_write_params
     - CALIB_TXPBACKOFFT
     - 0x14C
     - arg
     - arg is 32-bit value specifying TX power backoff versus temperature. Bits[7:0] 2.4G at +80 degree Celsius, bits[15:8] 2.4G at -20 degree Celsius, bits[23:16] 5.0G at +80 degree Celsius, bits[31:24] 5.0G at -20 degree Celsius. Each 8-bit field is a signed value.
   * - otp_write_params
     - CALIB_TXPBACKOFFV
     - 0x150
     - arg
     - arg is 32-bit value specifying TX power backoff versus voltage. Backoff in 2.4GHz, 5GHz bands  at very low voltage. Backoff in 2.4GHz, 5GHz bands  at low voltage. Each 8-bit field is signed value.
   * - otp_write_params
     - REGION_DEFAULTS
     - 0x154
     - arg
     - | Register default enable control.
       | arg is 32-bit value where a specific bit is set to ``0`` implies that the corresponding OTP field is programmed. The following list shows the bit and field mapping:
       |
       | bit 0  : QSPI_KEY
       | bit 1  : MAC0 Address
       | bit 2  : MAC1 Address
       | bit 3  : CALIB_XO
       | bit 4  : CALIB_PDADJM7
       | bit 5  : CALIB_PDADJM0
       | bit 6  : CALIB_PWR2G
       | bit 7  : CALIB_PWR5GM7
       | bit 8  : CALIB_PWR5GM0
       | bit 9  : CALIB_RXGNOFF
       | bit 10 : CALIB_TXPBACKOFFT
       | bit 11 : CALIB_TXPBACKOFFV
       | bit 12-31 : Reserved

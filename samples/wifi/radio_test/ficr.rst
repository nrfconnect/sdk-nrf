.. _wifi_ficr_prog:

FICR programming subcommands
############################

.. contents::
   :local:
   :depth: 2

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
     - | All four key arguments are 32-bit values forming the required 128-bit (Q)SPI key for encryption.
       | For example, if QSPI key is "112233445566778899aabbccddeeff00" then arg1=0x44332211 arg2=0x88776655 arg3=0xccbbaa99 arg4=0x00ffeedd
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
     - arg is 7-bit unsigned XO value. Bits [31:7] unused. Adjusts capacitor bank, 0 : Lowest capacitance (Highest frequency), 127 : Highest capacitance (Lowest frequency).
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
       | bit 4  : Reserved
       | bit 5  : Reserved
       | bit 6  : Reserved
       | bit 7  : Reserved
       | bit 8  : Reserved
       | bit 9  : Reserved
       | bit 10 : Reserved
       | bit 11 : Reserved
       | bit 12-31 : Reserved

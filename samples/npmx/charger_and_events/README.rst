.. _nPM1300:

nPM1300
################

Overview
********
This sample demonstrates usage of nPM1300 PMIC. Connect the NordicSemiconductor nRF5340 DK (nrf5340dk_nrf5340) to host device and open COM port. 
For testing connect and disconnect battery and USB-C cable to see various event messages.
Battery charging will also result in event messages being printed.

Requirements
************
This sample has been tested on the Nordic NordicSemiconductor nRF5340 DK (nrf5340dk_nrf5340) board with nPM1300 GoBoard.

.. list-table:: Connection table
   :widths: 25 25
   :header-rows: 1

   * - nrf5340dk
     - nPM1300 GoBoard
   * - P1.03
     - SDA
   * - P1.02
     - SCL
   * - P1.10
     - IO0 (IRQ)
   * - P1.11
     - IO1 (POF)
   * - VDD nRF (P20)
     - VDDIO (P3)
   * - External supply (P21)
     - VOUT2, GND (P6)

On GoBoard:
 - Connect battery to J4 or J3
 - For batteries without NTC, connect NTC resistor with jumper on P5
 - On P12, PVDD should be connected with VSYS
 - VBUS should be connected with VBUS USBC
 - On P6 connect with jumpers: (VOUT1 - LS1IN), (LS2IN - VSYS), (LS1OUT - ), (LS2OUT - )
 - On P8 connect with jumpers: (LED0, LED1, LED2), (VLED - VSYS)
 - On P5 connect with jumpers: (VSET1, VSET2, SDA, SCL)
 - Connect USBC to J1

On nrf5340dk:
 - Connect USB to host device (J2)
 - Switch nRF power source (SW9) to VDD
 - Switch VEXT -> nFR (SW10) to ON

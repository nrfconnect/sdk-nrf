.. _npmx_loadsw_sample:

npmx: LOADSW
############

.. contents::
   :local:
   :depth: 2

The npmx LOADSW sample demonstrates how to control PMIC LOADSWs using npmx drivers.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp

The sample also requires an nPM1300 EK.

Overview
********

This sample uses the npmx LOADSW driver to do the following:

  * Control enable mode for the LOADSW with software.
  * Control enable mode for the LOADSW with GPIO.

Wiring
******

#. Connect the TWI interface between the chosen DK and the nPM1300 EK as in the following table:

   .. list-table:: nPM1300 EK connections.
     :widths: 25 25 25
     :header-rows: 1

     * - nPM1300 EK pins
       - nRF5340 DK pins
       - nRF52840 DK pins
     * - GPIO0
       - P1.10
       - P1.10
     * - SDA
       - P1.02
       - P0.26
     * - SCL
       - P1.03
       - P0.27
     * - VOUT2 & GND
       - External supply (P21)
       - External supply (P21)

#. Make the following connections on the nPM1300 EK:

   * Connect a USB power supply to the **J3** connector.
   * Connect a suitable battery to the **J2** connector.
   * On the **P18** pin header, connect **VOUT2** and **VDDIO** pins with a jumper.
   * On the **P2** pin header, connect **VBAT** and **VBATIN** pins with a jumper.
   * On the **P17** pin header, connect all LEDs with jumpers.
   * On the **P13** pin header, connect **RSET1** and **GND** pins with a jumper.
   * On the **P14** pin header, connect **RSET2** and **VSET2** pins with a jumper.

#. Make the following connections on the chosen DK:

   * Set the **SW9** nRF power source switch to **VDD**.
   * Set the **SW10** VEXT → nRF switch to **ON**.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following sample-specific Kconfig options to specify which testcase you want to run:

.. CONFIG_TESTCASE_SW_ENABLE:

CONFIG_TESTCASE_SW_ENABLE
  This sample configuration enables controlling LOADSWs using software.
  In an infinite loop, switches are enabled and disabled alternately, with a delay of 5 seconds in between.

.. CONFIG_TESTCASE_GPIO_ENABLE:

CONFIG_TESTCASE_GPIO_ENABLE
  This sample configuration enables controlling LOADSWs using GPIOs.
  GPIO1 and GPIO2 are switched to input mode and selected GPIO pins (`NPMX_LDSW_GPIO_1` and `NPMX_LDSW_GPIO2`) are configured to control the switches.
  When you change the input states of the selected GPIOs you should see voltage changes in the load switches 1 and 2 outputs.

.. note::

   Only one Kconfig option may be enabled at a time.

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/npmx/loadsw`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by verifying the output voltage from LOADSWs.

#. Connect a multimeter to **LSOUT1** that is the output voltage pin of LOADSW 1.
#. Connect a multimeter to **LSOUT2** that is the output voltage pin of LOADSW 2.
#. |connect_kit|
#. |connect_terminal|
#. Depending on the chosen testcase, do the following:

   * For simple output voltage testcase: Check output voltage on EK **VOUT1** pin - it should be 2.4 V.
   * For advanced output voltage testcase: Check output voltage on EK **VOUT1** pin - it should start from 1.0 V and increase by 200 ms up to 3.3 V.
   * For retention voltage testcase: Connect EK **GPIO1** pin to GND.
     Check output voltage on EK **VOUT1** pin - it should be 1.7 V.
     Connect EK **GPIO1** pin to VDD.
     Check output voltage on EK **VOUT1** pin - it should be 3.3 V.
   * For enable LOADSWs using pin testcase:
     Connect EK **GPIO1** or **GPIO2** pin to GND.
     Check output voltage on EK **LSOUT1** or **LSOUT2** - it should be 0.0 V.
     Connect EK **GPIO1** or **GPIO2** pin to VDD.
     Check output voltage on EK **LSOUT1** or **LSOUT2** pin - it should be 3.3 V or 1.7 V.

Dependencies
************

This sample uses drivers from npmx.

.. _npmx_buck_sample:

npmx: BUCK
##########

.. contents::
   :local:
   :depth: 2

The npmx BUCK sample demonstrates how to control PMIC BUCKs using npmx drivers.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp

The sample also requires an nPM1300 EK.

Overview
********

This sample uses the npmx BUCK driver to do the following:

  * Set output voltage for the BUCK converter.
  * Set retention voltage for the BUCK converter with GPIO.
  * Control enable mode for the BUCK converter with GPIO.

Voltage can be changed only for BUCK 1.
BUCK 2 voltage is set by default to 3.0 V.

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

.. CONFIG_TESTCASE_SET_BUCK_VOLTAGE:

CONFIG_TESTCASE_SET_BUCK_VOLTAGE
  This sample configuration enables a simple output voltage testcase.
  The voltage is set to 2.4 V (:c:enumerator:`NPMX_BUCK_VOLTAGE_2V4`) on BUCK 1.

.. CONFIG_TESTCASE_OUTPUT_VOLTAGE:

CONFIG_TESTCASE_OUTPUT_VOLTAGE
  This sample configuration enables an advanced output voltage testcase.
  The BUCK 1 starts from voltage 1.0 V (:c:enumerator:`NPMX_BUCK_VOLTAGE_1V0`).
  The output voltage increases from the selected voltage to the maximum one (3.3 V).

.. CONFIG_TESTCASE_RETENTION_VOLTAGE:

CONFIG_TESTCASE_RETENTION_VOLTAGE
  This sample configuration enables a retention voltage testcase.
  GPIO1 (:c:enumerator:`NPMX_GPIO_INSTANCE_1`) is set to the input mode and is configured to be used as a retention input selector for BUCK 1 (:c:enumerator:`NPMX_BUCK_INSTANCE_1`).
  The normal mode voltage is set to 1.7 V (:c:enumerator:`NPMX_BUCK_VOLTAGE_1V7`) and the retention voltage to 3.3 V (:c:enumerator:`NPMX_BUCK_VOLTAGE_3V3`).
  When you change the input state of the selected GPIO, the voltage in the BUCK 1 output will change.

.. CONFIG_TESTCASE_ENABLE_BUCKS_USING_PIN:

CONFIG_TESTCASE_ENABLE_BUCKS_USING_PIN
  GPIO3 (:c:enumerator:`NPMX_GPIO_INSTANCE_3`) is set to the input mode and is configured to be used as an enable mode selector for BUCK 1 (:c:enumerator:`NPMX_BUCK_INSTANCE_1`).
  The output voltage of BUCK 1 is set to 3.3 V (:c:enumerator:`NPMX_BUCK_VOLTAGE_3V3`).
  When you change the input state of the selected GPIO, the voltage in the BUCK 1 output will change from 0 V to the previously selected voltage.

.. note::

   Only one Kconfig option may be enabled at a time.

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/npmx/buck`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by verifying the voltage.

#. Connect a multimeter to VOUT1 that is the output voltage pin of BUCK 1.
#. |connect_kit|
#. |connect_terminal|
#. Depending on the chosen testcase, do the following:

   * For simple output voltage testcase: Check output voltage on EK **VOUT1** pin - it should be 2.4 V.
   * For advanced output voltage testcase: Check output voltage on EK **VOUT1** pin - it should start from 1.0 V and increase by 200 ms up to 3.3 V.
   * For retention voltage testcase: Connect EK **GPIO1** pin to GND.
     Check output voltage on EK **VOUT1** pin - it should be 1.7 V.
     Connect EK **GPIO1** pin to VDD.
     Check output voltage on EK **VOUT1** pin - it should be 3.3 V.
   * For enable bucks using pin testcase: Connect EK **GPIO3** pin to GND.
     Check output voltage on EK **VOUT1** pin - it should be 0.0 V.
     Connect EK **GPIO3** pin to VDD.
     Check output voltage on EK **VOUT1** pin - it should be 3.3 V.

Dependencies
************

This sample uses drivers from npmx.

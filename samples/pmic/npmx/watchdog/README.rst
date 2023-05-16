.. _npmx_watchdog_sample:

npmx: WATCHDOG
##############

.. contents::
   :local:
   :depth: 2

The npmx WATCHDOG sample demonstrates how to configure watchdog TIMER using npmx drivers.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp

The sample also requires an nPM1300 EK.

Overview
********

This sample uses the npmx TIMER driver to do the following:

  * Configure TIMER in watchdog mode.
  * Register handler to be called when watchdog occurs.
  * Kick watchdog TIMER in a loop.

The timer is configured to generate a watchdog warning interrupt ~500 ms after user interaction.

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
     * - GPIO1
       - P1.11
       - P1.11
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

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/npmx/watchdog`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by decreasing the battery voltage below the threshold.

#. |connect_kit|
#. |connect_terminal|
#. Press the button on the console to stop kicking watchdog.
#. Wait ~500 ms to generate watchdog callback.
#. Wait another 16 ms to see pin reset is set low.

Dependencies
************

This sample uses drivers from npmx.

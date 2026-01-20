.. _npm13xx_fuel_gauge:

nPM1300 and nPM1304: Fuel gauge
###############################

.. contents::
   :local:
   :depth: 2

The Fuel gauge sample demonstrates how to calculate the state of charge of a development kit battery using `nPM1300 <nPM1300 product website_>`_ or  `nPM1304 <nPM1304 product website_>`_ and the :ref:`nrfxlib:nrf_fuel_gauge`.

For more information about fuel gauging with the nPM1300 or nPM1304, see `Using the nPM1300 and nPM1304 Fuel Gauge`_.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an `nPM1300 EK <nPM1300 EK product page_>`_ or an `nPM1304 EK <nPM1304 EK product page_>`_ that you need to connect to the development kit as described in `Wiring`_.

Overview
********

This sample allows to calculate the state of charge, time to empty, and time to full information from a battery on the development kit connected to the nPM1300 or nPM1304 PMIC.

.. _npm13xx_fuel_gauge_wiring:

Wiring
******

To connect your DK to the nPM1300 or nPM1304 EK, complete the following steps:

#. Make the following connections on the EK:

   a. Remove all existing connections, including jumpers and USB-C cables.
   #. On the **P1** pin header, connect **VBATIN** and **VBAT** pins with a jumper.
   #. On the **P17** pin header, connect all LEDs with jumpers.
   #. On the **P13** pin header, connect **RSET1** and **VSET1** pins with a jumper.
   #. On the **P14** pin header, connect **RSET2** and **VSET2** pins with a jumper.
   #. If your battery does not include an NTC thermistor, connect the **NTC** and **10kOhm** pins on the **P3** pin header with a jumper.
   #. Connect a suitable battery to either the **J2** or **J1** connector.
      When using the nPM1304 EK, the **J3** connector can also be used.
   #. Connect a USB power supply to the **J3** connector on nPM1300 EK or **J4** connector on nPM1304 EK.

   With these connections, the USB power supply is powering the EK, the BUCK regulators are enabled, and the I/O reference voltage is supplied by the DK as described in the next step.

   .. note::
      Battery charging will not begin if the NTC signal is disconnected.

#. Connect the chosen DK to the EK as in the following table:

   .. list-table:: nPM1300/nPM1304 EK connections.
      :widths: auto
      :header-rows: 1

      * - EK pins
        - nRF52 DK pins
        - nRF52840 DK pins
        - nRF5340 DK pins
        - nRF54L15/nRF54LM20 DK pins
        - nRF54H20 DK pins
        - nRF9160 DK pins
      * - SDA
        - P0.26
        - P0.26
        - P1.02
        - P1.11
        - P0.05
        - P0.30
      * - SCL
        - P0.27
        - P0.27
        - P1.03
        - P1.12
        - P0.00
        - P0.31
      * - GPIO3
        - P0.04
        - P0.04
        - P0.04
        - P0.04
        - P0.04
        - P0.04
      * - VDDIO
        - VDD
        - VDD
        - VDD
        - VDDIO
        - VDD_P0
        - VDD
      * - GND
        - GND
        - GND
        - GND
        - GND
        - GND
        - GND

.. note::
   When using the :zephyr:board:`nrf54l15dk`, the PMIC **GPIO3** interrupt pin assignment uses the DK's **BUTTON 3** pin.

Building and running
********************

.. |sample path| replace:: :file:`samples/pmic/native/npm13xx_fuel_gauge`

.. include:: /includes/build_and_run.txt

To build this sample for either nPM1300 or nPM1304, you need to apply the respective extra DTC overlay.
You can use either the nRF Connect for VS Code extension or the command line.

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      To apply an extra overlay, choose the respective file from the **Extra Devicetree overlays** drop-down menu

   .. group-tab:: Command line

      To apply the appropriate configuration, use the ``-DEXTRA_DTC_OVERLAY`` CMake argument.
      For example, to build for an nRF54L15 DK and an nPM1300 EK use the following command:

      .. code-block:: bash

         west build -b nrf54l15dk/nrf54l15/cpuapp samples/pmic/native/npm13xx_fuel_gauge -- -DEXTRA_DTC_OVERLAY=npm1300.overlay

Testing
*******

|test_sample|

#. |connect_kit|
#. |connect_terminal|

If the initialization was successful, the terminal displays the following message with status information:

.. code-block:: console

   PMIC device ok
   V: 4.101, I: 0.000, T: 23.06, SoC: 93.09, TTE: nan, TTF: nan

.. _table::
   :widths: auto

   ======  ===============  ==================================================
   Symbol  Description      Units
   ======  ===============  ==================================================
   V       Battery voltage  Volts
   I       Current          Amps (negative for charge, positive for discharge)
   T       Temperature      Degrees C
   SoC     State of Charge  Percent
   TTE     Time to Empty    Seconds (may be NaN)
   TTF     Time to Full     Seconds (may be NaN)
   ======  ===============  ==================================================

Dependencies
************

The sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_fuel_gauge`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:shell_api`

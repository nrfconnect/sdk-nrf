.. _ug_npm1300_gs:

Getting started with nPM1300 EK
###############################

.. contents::
   :local:
   :depth: 2

This section gets you started with your nPM1300 :term:`Evaluation Kit (EK)` using the |NCS|.
It tells you how to build and flash the :ref:`npm1300_fuel_gauge` sample and perform a quick test of your EK.

If this is your first time developing with a Nordic DK, read the appropriate getting started tutorial first:

* :ref:`ug_nrf52_gs`
* :ref:`ug_nrf5340_gs`
* :ref:`ug_nrf9160_gs`

Minimum requirements
********************

Make sure you have all the required hardware and that your computer has one of the supported operating systems.

Hardware
========

* One of the following development kits:

  * nRF52 DK
  * nRF52840 DK
  * nRF5340 DK
  * nRF9120 DK

* nPM1300 EK
* A suitable battery
* Micro-USB 2.0 cable
* USB-C charger
* Jumper wires

Software
========

On your computer, one of the following operating systems:

* Microsoft Windows
* macOS
* Ubuntu Linux

|Supported OS|

.. _npm1300_gs_installing_software:

Installing the required software
********************************

Install `nRF Connect for Desktop`_.
After installing and starting the application, install the Programmer app.

.. _npm1300_gs_building:

Building and programming the sample
***********************************

Follow the detailed instructions in the :ref:`npm1300_fuel_gauge` sample documentation to build the sample and flash it to the DK.

.. _npm1300_gs_testing:

Testing the sample
******************

When the fuel gauge software has connected successfully to the PMIC on the EK, it will display the following status information:

.. code-block:: console

   PMIC device ok
   V: 4.101, I: 0.000, T: 23.06, SoC: 93.09, TTE: nan, TTF: nan

Next steps
**********

You have now completed getting started with the nPM1300 EK.
See the following links for where to go next:

* The EK `User Guide <nPM1300 EK User Guide_>`_ for detailed information related to the nPM1300 EK.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.

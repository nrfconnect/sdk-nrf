.. _build_pgm_nrf9160:

Programming onto nRF91 Series devices
#####################################

.. contents::
   :local:
   :depth: 2

You can program applications and samples onto the nRF91 Series devices after :ref:`building` the corresponding firmware images.

To program firmware onto nRF91 Series devices, follow the process described in the :ref:`programming` section.
Pay attention to the exceptions specific to the nRF91 Series, which are listed below.

.. |exceptions_step| replace:: Make sure to check the programming exceptions for the nRF91 Series in the sections below.

.. include:: /includes/vsc_build_and_run_series.txt

Compatible versions of modem and application
********************************************

When you update the application firmware on an nRF91 Series DK, make sure that the modem firmware and application firmware are compatible versions.

.. _build_pgm_nrf9160_board_controller:

Selecting board controller on nRF9160 DK
****************************************

When programming on the nRF9160 DK, make sure to set the **SW10** switch (marked PROG/DEBUG) in the **nRF91** position to program the nRF9160 application.
In nRF9160 DK board revision v0.9.0 and earlier, the switch is called **SW5**.

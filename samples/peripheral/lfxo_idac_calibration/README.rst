.. _lfxo_idac_calibration_sample:

.. ncs-sample::
   :title: LFXO IDAC calibration

   The LFXO IDAC calibration sample finds the initial IDAC code for the Pierce mode of the low-frequency crystal oscillator (LFXO) on your board.
   You can use the result to configure the ``idac-init`` devicetree property of the LFXO.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

In Pierce mode, the amplitude of the LFXO oscillation depends on the current that the IDAC supplies to the oscillator.
The IDAC code that keeps the amplitude within the peak detector thresholds depends on the crystal, the load capacitance, and the board design.

The sample keeps the LFXO in Pierce mode and runs an amplitude search in the following steps:

1. Disables the automatic amplitude control, so that the IDAC follows the initial IDAC code directly.
#. Runs a successive approximation over the 5-bit IDAC code.
   For each code, the sample drives the IDAC to its minimum or maximum value until the amplitude is within the peak detector thresholds.
   It then applies the code and monitors the peak detector.
   If the amplitude stays within the thresholds, the code is accepted.
   Otherwise, the sample tries a lower or higher code.
#. Prints the devicetree configuration to use.

The result is valid only for the load capacitance used during the search.
Run the sample again if you change the crystal or the ``load-capacitance-femtofarad`` devicetree property of the LFXO.

Building and running
********************

.. |sample path| replace:: :file:`samples/peripheral/lfxo_idac_calibration`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.
#. Wait until the sample prints the result.

   The output is similar to the following:

   .. code-block:: console

      LFXO IDAC calibration
      Add the following to the devicetree:
      &lfxo {
              idac-init = <8>;
      };

#. Add the printed configuration to the devicetree overlay of your application.

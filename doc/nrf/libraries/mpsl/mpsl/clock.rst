.. _mpsl_clock:

MPSL Clock
##########

.. contents::
    :local:
    :depth: 2

The MPSL clock module provides access to the low-frequency clock (LFCLK) configuration and allows the application to request the crystal oscillator source of the high-frequency clock (HFCLK).

See the following `Low-frequency clock (LFCLK)`_ and `High-frequency clock (HFCLK)`_ sections for a description of the design.

Low-frequency clock (LFCLK)
***************************

MPSL can use one of the following available low-frequency clock (LFCLK) sources:

* 32.768 kHz internal RC oscillator (LFRC)
* 32.768 kHz crystal oscillator (LFXO)

When MPSL is enabled, the application must provide the selected LFCLK source, calibration configurations, and clock accuracy.
The selected accuracy is meant to be forwarded only to the protocol stacks.

The MPSL clock module configures the LFCLK source and keeps it calibrated when the RC oscillator is the selected clock source.

Variations in temperature affect the RC oscillator frequency.
As such, it requires additional calibration for temperature changes of more than 0.5°C.

To ensure the required accuracy, the MPSL clock module must calibrate periodically the RC oscillator using the HFCLK crystal oscillator as a reference.
See the relevant SoC product specification for more information.
As the MPSL clock module runs the calibration automatically, the application must specify how often the clock module takes measurements to detect temperature changes.

.. note::
   You should consider how frequently significant temperature changes are expected to occur in the intended environment of the end product.
   It is recommended to use a temperature polling interval of 4 seconds, and to force the clock calibration every second interval (``.rc_ctiv=16``, ``.rc_temp_ctiv=2``).

High-frequency clock (HFCLK)
****************************

The high-frequency clock (HFCLK) controller supports the following HFCLK sources:

* 64 MHz internal oscillator (HFINT)
* 64 MHz crystal oscillator (HFXO)

The HFXO must be running to use the ``RADIO`` peripheral.
MPSL makes sure the HFXO is started in time for every scheduled ``RADIO`` activity.

.. note::
    For the correct operation of the ``RADIO`` peripheral, you must set the HFXO startup time to less than 1.5 ms.
    As such, you must choose the external crystal oscillator and the other related components accordingly.
    See the relevant SoC product specification for more information.

The MPSL clock module API enables the application to safely request and release the HFXO while the protocol stacks are enabled.
MPSL can continue to use the HFXO after it is released if it is requested by protocol stacks.
HFXO is stopped when it is no longer needed.

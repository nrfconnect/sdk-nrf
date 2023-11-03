.. _bluetooth_coex:

Bluetooth-Only External Radio Coexistence
#########################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® coexistence feature can use the 1-wire LTE coexistence protocol.

**Application Programming Interface:**

You can find the details of the API in the :file:`nrfxlib/mpsl/include/mpsl_coex.h` file.


.. NOTE::
   This implementation is only supported on nRF52 devices.

.. _mpsl_bluetooth_coex_1wire:

1-Wire coexistence protocol
---------------------------


Interface
**********

The 1-wire protocol lets Bluetooth LE nRF chips coexist alongside an LTE device on a separate chip.
It was specifically designed for the `coex interface of nRF9160`_.


Hardware resources
==================

The 1-wire Coexistence protocol requires the following peripherals:

.. table:: 1-wire coexistence protocol HW resources

   ===============  =====  ========================================================
   Peripheral       Count  Description
   ===============  =====  ========================================================
   GPIO pins        1      Pin selected for the 1-wire interface BLE_GRANT
   GPIOTE channels  1      One GPIOTE channel for registering BLE_GRANT pin changes
   PPI channels     1      Connecting the BLE_GRANT to Radio DISABLE task
   ===============  =====  ========================================================

Interface signals
=================

The 1-wire interface is a single unidirectional input controlled by the LTE device:

.. table:: 1-wire LTE coexistence protocol pin

   =========  =========  =====================================
   Pin        Direction  Description
   =========  =========  =====================================
   BLE_GRANT  In         Used by the LTE to grant radio access
   =========  =========  =====================================

The interface consists of a BLE_GRANT signal mapped to the GPIO pin by the application's interface configuration.
The signal's active level (high or low) is programmable.
Whenever the nRF SoC requires the use of the radio for any RF activity, it needs to test that the BLE_GRANT pin level is valid for Bluetooth LE radio use (such as 0 for the nRF9160).

The figure below illustrates these timings with regard to the LTE and BLE_GRANT signal.

.. figure:: pic/mpsl_coex_1wire_timing_grant.svg

   1-wire LTE Coexistence protocol timing

When the BLE_GRANT is revoked (for example the pin de-asserts for the nRF9160), the Bluetooth LE radio immediately gets disabled with a maximum delay of 7us.

When the coexistence interface is disabled, the associated GPIO pins are set to a high impedance state.

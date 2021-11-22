.. _ug_bt_coex:

Using Bluetooth external radio coexistence
##########################################

.. contents::
   :local:
   :depth: 2

This guide describes how to add support for Bluetooth® coexistence to your application in |NCS|.

.. _ug_bt_coex_overview:

Overview
********

The coexistence feature can be used to reduce radio interference when multiple devices are located close to each other.
This feature puts the Bluetooth stack under the control of a Packet Traffic Arbitrator (PTA) through a three-wire interface.
The feature can only be used with the SoftDevice Controller, and only on nRF52 Series devices.

The implementation is based on :ref:`nrfxlib:bluetooth_coex` which is integrated into nrfxlib's MPSL library.

.. _ug_bt_coex_requirements:

Resources
*********

The bluetooth coexistence implementation requires exclusive access to NRF_TIMER1, in addition to the GPIO, GPIOTE and PPI resources listed in :ref:`nrfxlib:bluetooth_coex`.

Enabling coexistence and MPSL
*****************************

Make sure that the following Kconfig options are enabled:

   * :kconfig:`CONFIG_MPSL_CX`
   * :kconfig:`CONFIG_MPSL_CX_BT_3WIRE`

.. _ug_bt_coex_config:

Configuring coexistence
***********************

Configuration is set using the devicetree (DTS).
For more information about devicetree overlays, see :ref:`zephyr:use-dt-overlays`.
A sample devicetree overlay is available at :file:`samples/bluetooth/radio_coex_3wire/boards/nrf52840dk_nrf52840.overlay`.
The elements are described in the bindings: :file:`dts/bindings/radio_coex/sdc-radio-coex-three-wire.yaml`.

.. _ug_bt_coex_sample:

Sample application
******************

A sample application can be found at :file:`samples/bluetooth/radio_coex_3wire`.

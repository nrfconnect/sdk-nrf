.. _ug_bt_coex:

Bluetooth external radio coexistence integration
################################################

.. contents::
   :local:
   :depth: 2

This guide describes how to add support for Bluetooth® coexistence to your application in |NCS|.

The coexistence feature can be used to reduce radio interference when multiple devices are located close to each other.
This feature puts the Bluetooth stack under the control of a Packet Traffic Arbitrator (PTA) through either a three-wire interface or a one-wire interface.
The feature can only be used with the SoftDevice Controller, and only on nRF52 Series devices.

The implementation is based on :ref:`nrfxlib:bluetooth_coex` which is integrated into nrfxlib's MPSL library.

1-Wire coexistence protocol
---------------------------

.. _ug_bt_coex_1w_requirements:

Resources
*********

The Bluetooth 1-wire coexistence implementation requires access to the GPIO, GPIOTE and PPI resources listed in :ref:`nrfxlib:bluetooth_coex`.

Enabling 1-wire coexistence and MPSL
************************************

Make sure that the following Kconfig options are enabled:

* :kconfig:option:`CONFIG_MPSL_CX`
* :kconfig:option:`CONFIG_MPSL_CX_BT_1WIRE`

.. _ug_bt_coex_1w_config:

Configuring 1-wire coexistence
******************************

Configuration is set using the devicetree (DTS).
For more information about devicetree overlays, see :ref:`zephyr:use-dt-overlays`.
A sample devicetree overlay is available at :file:`samples/bluetooth/radio_coex_1wire/boards/nrf52840dk_nrf52840.overlay`.
The elements are described in the bindings: :file:`dts/bindings/radio_coex/sdc-radio-coex-one-wire.yaml`.

.. _ug_bt_coex_1w_sample:

Sample application
******************

A sample application for 1-wire coexistence can be found at :file:`samples/bluetooth/radio_coex_1wire`.

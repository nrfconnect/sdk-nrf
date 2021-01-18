.. _multiprotocol-rpmsg-sample:

Multiprotocol RPMsg
###################

Overview
********

This sample exposes :ref:`bluetooth_controller` and IEEE 802.15.4 radio driver
support to another device or CPU using RPMsg transport which is
a part of `OpenAMP <https://github.com/OpenAMP/open-amp/>`__.

Requirements
************

* A board with :ref:`ipm_api` driver, Bluetooth LE, and IEEE 802.15.4 support

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/multiprotocol`
in the Zephyr tree.

To use this application, you need a board with a Bluetooth controller,
IPM and IEEE 802.15.4 drivers.
You can then build this application and flash it onto your board in
the usual way. See :ref:`boards` for board-specific building and
programming information.

To test this sample, you need a separate device/CPU that acts as Bluetooth
HCI RPMsg peer and as 802.15.4 serialization RPMsg peer.
This sample is compatible with the HCI RPMsg driver provided by
Zephyr's Bluetooth :ref:`bt_hci_drivers` core. See the
:option:`CONFIG_BT_RPMSG` configuration option for more information.

You might need to adjust the Kconfig configuration of this sample to make it
compatible with the peer application. For example, :option:`CONFIG_BT_MAX_CONN`
must be equal to the maximum number of connections supported by the peer application.

Refer to :ref:`bluetooth-samples` for general information about Bluetooth samples.

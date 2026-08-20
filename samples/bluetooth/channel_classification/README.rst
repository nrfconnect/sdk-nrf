.. _ble_channel_classification:

.. ncs-sample::
   :title: Bluetooth: Channel Classification

The Channel Classification sample demonstrates Channel Classification reporting between a central and a peripheral.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any combination of the development kits mentioned in this sample in your testing setup.

Additionally, the sample requires a connection to a computer with a serial terminal for each of the development kits.

Overview
********

When connected, the peripheral cycles through three predefined host channel classifications, submitting a new one every five seconds.

The central enables reporting with :c:func:`hci_vs_sdc_channel_reporting_enable`, receives each report from the peripheral in a HCI VS event callback, and applies the peripheral's channel classification to the connection.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/channel_classification`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

After programming the sample to both development kits, test it by performing the following steps:

1. Connect to both kits with a terminal emulator (for example, the `Serial Terminal app`_).
   See :ref:`test_and_optimize` for the required settings and steps.
#. Reset both kits.
#. Start the application on the connected board in the central role by typing ``c`` in one of the terminal emulators.
#. Start the application in the peripheral role by typing ``p`` in the other terminal emulator.
#. Observe that the kits establish a connection.
#. Observe a new channel classification printed every five seconds on the peripheral.
#. Observe that the central prints a matching channel classification from the received classification reports and applies it as a channel map.

Sample output
=============

The following is an example of the peripheral output:

.. code-block:: console

   *** Booting nRF Connect SDK v3.4.99-b15b0977d5d7 ***
   *** Using Zephyr OS v4.4.99-bf52a9edec7e ***
   I: Starting Bluetooth Channel Classification sample
   I: SoftDevice Controller build revision:
   I: 2e 0b 04 cf 06 5e 2e 8f |.....^..
   I: a8 14 89 4d b6 1b 7a 8c |...M..z.
   I: d4 e4 bb 1f             |....
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF54Lx (0x0005)
   I: Firmware: Standard Bluetooth controller (0x00) Version 46.1035 Build 777914063
   I: HCI transport: SDC
   I: Identity: E9:A2:74:BE:64:BA (random)
   I: HCI: version 6.3 (0x11) revision 0x306a, manufacturer 0x0059
   I: LMP: version 6.3 (0x11) subver 0x306a
   I: Bluetooth initialized
   I: Choose device role - type c (central) or p (peripheral):
   I:
   I: Selected Peripheral
   I: Advertising successfully started
   I: Connected as peripheral
   I: Using channel map 0
   I: Peripheral channel map ff ff ff ff 1f
   I: Using channel map 1
   I: Peripheral channel map ff fb ef bf 1f
   I: Using channel map 2
   I: Peripheral channel map df 7f ff fd 1f

The following is an example of the central output:

.. code-block:: console

   *** Booting nRF Connect SDK v3.4.99-b15b0977d5d7 ***
   *** Using Zephyr OS v4.4.99-bf52a9edec7e ***
   I: Starting Bluetooth Channel Classification sample
   I: SoftDevice Controller build revision:
   I: 2e 0b 04 cf 06 5e 2e 8f |.....^..
   I: a8 14 89 4d b6 1b 7a 8c |...M..z.
   I: d4 e4 bb 1f             |....
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF54Lx (0x0005)
   I: Firmware: Standard Bluetooth controller (0x00) Version 46.1035 Build 777914063
   I: HCI transport: SDC
   I: Identity: F1:6C:D4:5E:80:EA (random)
   I: HCI: version 6.3 (0x11) revision 0x306a, manufacturer 0x0059
   I: LMP: version 6.3 (0x11) subver 0x306a
   I: Bluetooth initialized
   I: Choose device role - type c (central) or p (peripheral):
   I:
   I: Selected Central
   I: Scanning successfully started
   I: Filters matched. Address: E9:A2:74:BE:64:BA (random) connectable: 1
   I: Connected as central
   I: Channel classification reporting enabled
   I: Central channel map from report ff ff ff ff 1f
   I: Central applied peripheral channel classification
   I: Central channel map from report ff fb ef bf 1f
   I: Central applied peripheral channel classification
   I: Central channel map from report df 7f ff fd 1f

Dependencies
************

This sample uses the following |NCS| library:

* :file:`include/bluetooth/hci_vs_sdc.h`

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:softdevice_controller`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`

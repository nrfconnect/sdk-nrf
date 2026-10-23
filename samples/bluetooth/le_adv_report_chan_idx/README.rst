.. _ble_le_adv_report_chan_idx:

.. ncs-sample::
   :title: Bluetooth: LE Advertising Report with channel index

This sample demonstrates the channel index appended to HCI advertising reports
of legacy advertising PDUs using the SoftDevice Controller proprietary extension.

Overview
********

The sample enables the feature at build time with the
:kconfig:option:`CONFIG_BT_CTLR_SDC_CHAN_IDX_IN_ADV_REPORT` and
:kconfig:option:`CONFIG_BT_HCI_ADV_REPORT_CHAN_IDX` Kconfig options.
The controller links the feature through the
:c:func:`sdc_support_chan_idx_in_adv_report` function and appends a ``chan_idx``
octet to advertising reports of legacy advertising PDUs.
The scan receive callback exposes the value in the
:c:member:`bt_le_scan_recv_info.chan_idx` field.

The channel index is available for legacy advertising PDUs with both scanning
interfaces:

* When :kconfig:option:`CONFIG_BT_EXT_ADV` is disabled, the host uses the legacy
  scanning HCI commands.
  The controller appends the ``chan_idx`` octet after the ``rssi`` field in
  LE Advertising Report events.
* When :kconfig:option:`CONFIG_BT_EXT_ADV` is enabled, the host uses the
  extended scanning HCI commands.
  The controller appends the ``chan_idx`` octet after the advertising data in
  LE Extended Advertising Report events for legacy advertising PDUs.

The channel index is not available for extended advertising PDUs, and the
:c:member:`bt_le_scan_recv_info.chan_idx` field is set to ``UINT8_MAX``.

The sample prints each advertising report twice, first from the scan callback
passed to :c:func:`bt_le_scan_start`, and then from the scan receive callback
registered with :c:func:`bt_le_scan_cb_register`.
Only the scan receive callback prints the channel index.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need one development kit that runs this scanner sample and a nearby device
that sends legacy advertising PDUs. Any standard advertiser sample from Zephyr
or the |NCS| works as the advertiser.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/le_adv_report_chan_idx`

.. include:: /includes/build_and_run.txt

To use the extended scanning HCI commands, build the sample with
:kconfig:option:`CONFIG_BT_EXT_ADV` enabled for both the application and the
``ipc_radio`` image, for example, by adding the
``-DCONFIG_BT_EXT_ADV=y -Dipc_radio_CONFIG_BT_EXT_ADV=y`` CMake arguments.

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Start an advertiser on a nearby device.
#. Observe that the terminal shows advertising reports with channel index values.

Sample output
=============

The output should look similar to the following:

.. code-block:: console

   LE Advertising Report with Channel Index sample
   Starting scan
   scan_cb: Device found: D4:3B:2A:5C:11:07 (random) (RSSI -45), type 0, AD data len 31
   scan_recv: Device found: D4:3B:2A:5C:11:07 (random) (chan_idx 37 RSSI -45), type 0, AD data len 31
   scan_cb: Device found: D4:3B:2A:5C:11:07 (random) (RSSI -47), type 0, AD data len 31
   scan_recv: Device found: D4:3B:2A:5C:11:07 (random) (chan_idx 38 RSSI -47), type 0, AD data len 31

When the sample is built with :kconfig:option:`CONFIG_BT_EXT_ADV` enabled, reports
of extended advertising PDUs have the channel index set to ``255`` (``UINT8_MAX``):

.. code-block:: console

   scan_cb: Device found: E1:62:9F:03:4A:B2 (random) (RSSI -52), type 5, AD data len 50
   scan_recv: Device found: E1:62:9F:03:4A:B2 (random) (chan_idx 255 RSSI -52), type 5, AD data len 50

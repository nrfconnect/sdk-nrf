.. _ble_le_adv_report_chan_idx:

.. ncs-sample::
   :title: Bluetooth: LE Advertising Report with channel index

This sample demonstrates channel index append to standard HCI LE Advertising
Report events using the SoftDevice Controller proprietary extension.

Enable the feature at build time with
:kconfig:option:`CONFIG_BT_CTLR_SDC_CHAN_IDX_IN_ADV_REPORT` and
:kconfig:option:`CONFIG_BT_HCI_ADV_REPORT_CHAN_IDX`. The controller links the
feature via :c:func:`sdc_support_chan_idx_in_adv_report()` and appends a
``chan_idx`` octet after ``rssi`` in standard LE Advertising Report events. The
scan receive callback exposes the value in
:c:member:`bt_le_scan_recv_info.chan_idx`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need one development kit running this scanner sample and a nearby device
advertising legacy advertising PDUs (any standard Zephyr/nRF advertiser sample
works).

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/le_adv_report_chan_idx`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

When an advertiser is nearby, the scanner console shows advertising reports
with channel index values.

Sample output
=============

The output should look similar to the following:

.. code-block:: console

   LE Advertising Report with Channel Index sample
   Starting scan
   scan_recv: Device found: ... (chan_idx 37 RSSI -45), type 0, AD data len 31

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

When :kconfig:option:`CONFIG_BT_EXT_ADV` is enabled, the host uses the
extended scanning HCI commands. The controller then appends the ``chan_idx``
octet after the advertising data in LE Extended Advertising Report events for
legacy advertising PDUs. For extended advertising PDUs,
:c:member:`bt_le_scan_recv_info.chan_idx` is set to ``UINT8_MAX``.

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

To use the extended scanning HCI commands, build the sample with
:kconfig:option:`CONFIG_BT_EXT_ADV` enabled for both the application and the
``ipc_radio`` image, for example, by adding the
``-DCONFIG_BT_EXT_ADV=y -Dipc_radio_CONFIG_BT_EXT_ADV=y`` CMake arguments.

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

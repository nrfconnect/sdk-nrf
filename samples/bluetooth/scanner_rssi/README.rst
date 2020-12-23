.. _bluetooth-scanner-rssi-sample:

Bluetooth: Scanner RSSI
#######################

Overview
********

A simple application to demonstrate how to read/print RSSI values returned while
scanning for BLE devices.

It also demonstrates how to apply temperature compensation to the RSSI values,
depending on the SoC the sample is compiled for.

The description of the anomalies and compensation that has to be applied can be
found on the Nordic Semiconductor infocenter:

`Errata 87 (nRF5340)
<https://infocenter.nordicsemi.com/topic/errata_nRF5340_Rev1/ERR/nRF5340/Rev1/latest/anomaly_340_87.html>`_.

`Errata 153 (nRF52840)
<https://infocenter.nordicsemi.com/topic/errata_nRF52840_Rev2/ERR/nRF52840/Rev2/latest/anomaly_840_153.html>`_.

`Errata 225 (nRF52833)
<https://infocenter.nordicsemi.com/topic/errata_nRF52833_Rev1/ERR/nRF52833/Rev1/latest/anomaly_833_225.html>`_.

`Errata 225 (nRF52820)
<https://infocenter.nordicsemi.com/topic/errata_nRF52820_Rev2/ERR/nRF52820/Rev2/latest/anomaly_820_225.html>`_.

The sample should print on the console scanned addresses,
(temperature-compensated) RSSI values for close devices, and the current
temperature in Celsius when compensation is applied.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52dk_nrf52832, nrf52840dk_nrf52840, nrf52833dk_nrf52820, nrf52833dk_nrf52833

Building and Running
********************

.. |sample path| replace:: :file:`samples/bluetooth/scanner_rssi`

.. include:: /includes/build_and_run.txt

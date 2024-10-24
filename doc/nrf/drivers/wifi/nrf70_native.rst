.. _nrf70_wifi:

nRF Wi-Fi driver
################

.. contents::
   :local:
   :depth: 2

The nRF Wi-Fi driver implements the Wi-Fi® protocol for the nRF70 FullMAC Series of devices.
FullMAC devices implement the Wi-Fi protocol in the chipset.
The driver configures the chipset and transfers the frames to and from the device to the networking stack.

The nRF70 Series device is a companion IC and can be used with any Nordic Semiconductor System-on-Chips (SoCs), such as the nRF53 and nRF91 Series SoCs.

You can enable the driver by using the following Kconfig options:

  * When using :ref:`zephyr:sysbuild` to build your application, set the :kconfig:option:`SB_CONFIG_WIFI_NRF70` Kconfig option to ``y`` in your :file:`sysbuild.conf` file.`
  * When not using :ref:`zephyr:sysbuild`, set the :kconfig:option:`CONFIG_WIFI_NRF70` Kconfig option to ``y`` in your application's :file:`prj.conf` file.

Architecture
*************

The following figure illustrates the architecture of the nRF Wi-Fi driver.

.. figure:: /images/nrf700x_wifi_driver.svg
   :alt: nRF Wi-Fi driver block diagram
   :align: center
   :figclass: align-center

   nRF Wi-Fi driver architecture overview

Design overview
***************

The nRF Wi-Fi driver follows an OS agnostic design, and the driver implementation is split into OS-agnostic and OS (Zephyr)-specific code.
The OS-agnostic code is located in the :file:`${ZEPHYR_BASE}/../modules/hal/nordic/drivers/nrf_wifi/` folder, and the Zephyr OS port is located in the :file:`${ZEPHYR_BASE}/drivers/wifi/nrfwifi/` folder.

The driver supports two modes of operation:

Wi-Fi mode
==========

In this mode, the driver is designed to be used with the Zephyr networking stack.
It is implemented as a network interface driver.

The driver supports the following IEEE 802.11 features:

* Wi-Fi 6 (802.11ax) support
* WPA3™/WPA2™ personal security
* IEEE 802.11 Power Save modes
* Scan-only mode
* IEEE 802.11 :term:`Station mode (STA)`
* :term:`Software-enabled Access Point (SoftAP or SAP)` mode

The Wi-Fi Direct® mode feature is in the driver code but is not yet supported.

Except for scan-only mode, the driver uses the host access point daemon (hostapd) to implement AP Media Access Control (MAC) Sublayer Management Entity (AP MLME) and wpa_supplicant to implement 802.1X supplicant.

Radio Test mode
===============

The nRF Wi-Fi driver supports Radio Test mode, which you can use to test the RF performance of the nRF70 Series device.
This is a build time option that you can enable using the :kconfig:option:`CONFIG_NRF70_RADIO_TEST` Kconfig option.

For more details about using this driver in Radio Test mode, see :ref:`wifi_radio_test`.

Driver to nRF70 Series device communication
*******************************************

The driver communicates with the nRF70 Series device using the QSPI/SPI interface.
The driver uses the QSPI/SPI interface to send commands to the nRF70 Series device, and to transfer data to and from the device.
The nRF7002 DK uses QSPI, whereas the nRF7002 EK uses SPI.

To connect the nRF7002 EK to the SoC, the ``nrf7002ek`` shield is required.

Configuration
*************

The nRF Wi-Fi driver has the following configuration options:

Kconfig configuration
=====================

.. options-from-kconfig:: /../../../../../zephyr/drivers/wifi/nrfwifi/Kconfig.nrfwifi
   :show-type:

Devicetree specification configuration
======================================

The maximum transmit power achieved on an nRF70 Series device-based product depends on the frequency band and operating channel.
This varies across different :term:`Printed Circuit Board (PCB)` designs.

Multiple calibrations and checks are implemented to ensure consistency across channels and devices.
However, these values depend on PCB design, which may result in Error Vector Magnitude (EVM) and spectral mask failures.
To avoid this problem, you can specify the power ceiling at which the EVM and spectral mask are met for a given PCB design.
Additionally, build-time parameters are made available to drivers through the DTS overlay file.

The following code snippet shows an example of the DTS overlay file.
Note that the numbers filled in the following example do not represent any particular PCB design or package type.

You must replace these values with measurements obtained from transmitter testing on your own PCB designs.
The values are represented in 0.25 dB increments.
To configure 15 dBm, use the value ``60``.

.. code-block:: devicetree

   &nrf70 {
      wifi-max-tx-pwr-2g-dsss = <84>;
      wifi-max-tx-pwr-2g-mcs0 = <64>;
      wifi-max-tx-pwr-2g-mcs7 = <64>;
      wifi-max-tx-pwr-5g-low-mcs0 = <56>;
      wifi-max-tx-pwr-5g-low-mcs7 = <56>;
      wifi-max-tx-pwr-5g-mid-mcs0 = <56>;
      wifi-max-tx-pwr-5g-mid-mcs7 = <56>;
      wifi-max-tx-pwr-5g-high-mcs0 = <56>;
      wifi-max-tx-pwr-5g-high-mcs7 = <56>;
   };

See the DTS binding documentation for more information.

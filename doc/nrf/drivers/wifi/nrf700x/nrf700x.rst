.. _nrf700x_wifi:

nRF Wi-Fi driver
################

.. contents::
   :local:
   :depth: 2

The nRF Wi-Fi driver implements the Wi-Fi® protocol for the nRF70 FullMAC Series of devices.
FullMAC devices implement the Wi-Fi protocol in the chipset.
The driver configures the chipset and transfers the frames to and from the device to the networking stack.

nRF70 Series device is a companion IC and can be used with any Nordic Semiconductor System-on-Chips (SoCs), such as the nRF53 and nRF91 Series SoCs.

You can enable the driver by using the :kconfig:option:`CONFIG_WIFI_NRF700X` Kconfig option.

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

The nRF Wi-Fi driver follows an OS agnostic design, and the driver implementation is split into OS agnostic and OS (Zephyr) specific code.
The OS agnostic code is located in the :file:`drivers/wifi/nrf700x/osal/` folder and the OS specific code is located in the :file:`drivers/wifi/nrf700x/zephyr/` folder.

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
This is a build time option that you can enable using the :kconfig:option:`CONFIG_NRF700X_RADIO_TEST` Kconfig option.

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

.. options-from-kconfig::
   :show-type:

Devicetree Specification configuration
======================================

The maximum transmit power achieved on a nRF70 Series device-based product depends on the frequency band and operating channel.
This varies from chip to chip as well as over different :term:`Printed Circuit Board (PCB)` designs.

Multiple calibrations and checks are implemented to ensure consistency across channels and devices.
However, these values have a dependency on PCB design, which may result in Error Vector Magnitude (EVM) and spectral mask failures.
To avoid this problem, you can specify the power ceiling at which the EVM and spectral mask are met for a given PCB design.
Additionally, build-time parameters are made available to drivers through the DTS overlay file.

The following code snippet shows an example of the DTS overlay file:

.. code-block:: devicetree

	/
	{
		nrf70_tx_power_ceiling: nrf70_tx_power_ceiling_node {
			status = "okay";
			compatible = "nordic,nrf700x-tx-power-ceiling";
			max-pwr-2g-dsss = <0x54>;
			max-pwr-2g-mcs0 = <0x40>;
			max-pwr-2g-mcs7 = <0x40>;
			max-pwr-5g-low-mcs0 = <0x38>;
			max-pwr-5g-low-mcs7 = <0x38>;
			max-pwr-5g-mid-mcs0 = <0x38>;
			max-pwr-5g-mid-mcs7 = <0x38>;
			max-pwr-5g-high-mcs0 = <0x38>;
			max-pwr-5g-high-mcs7 = <0x38>;
		};

	};


The following table lists the parameters (8-bit unsigned values) defined in the DTS overlay board files:

.. list-table:: DTS file parameters
   :header-rows: 1

   * - DTS parameter
     - Description
   * - max-pwr-2g-dsss
     - Transmit power ceiling for DSSS data rate in 0.25 dBm steps.
       This is applicable for all DSSS data rates.
   * - max-pwr-2g-mcs0
     - Transmit power ceiling for MCS0 data rate in 2.4 GHz band in steps of 0.25 dBm steps.
   * - max-pwr-2g-mcs7
     - Transmit power ceiling for MCS7 data rate in 2.4 GHz band in steps of 0.25 dBm steps.
   * - max-pwr-5g-low-mcs0
     - Transmit power ceiling for MCS0 in lower 5 GHz frequency band in steps of 0.25 dBm.
       Lower 5 GHz frequency band refers to channels from 36 to 64.
   * - max-pwr-5g-low-mcs7
     - Transmit power ceiling for MCS7 in lower 5 GHz frequency band in steps of 0.25 dBm.
   * - max-pwr-5g-mid-mcs0
     - Transmit power ceiling for MCS0 in mid 5 GHz frequency band in steps of 0.25 dBm.
       Mid 5 GHz frequency band refers to channels from 100 to 132.
   * - max-pwr-5g-mid-mcs7
     - Transmit power ceiling for MCS7 in mid 5 GHz frequency band in steps of 0.25 dBm.
   * - max-pwr-5g-high-mcs0
     - Transmit power ceiling for MCS0 in high 5 GHz frequency band in steps of 0.25 dBm.
       High 5 GHz frequency band refers to channels from 136 to 177.
   * - max-pwr-5g-mid-mcs7
     - Transmit power ceiling for MCS7 in mid 5 GHz frequency band in steps of 0.25 dBm.


API documentation
*****************

After the nRF Wi-Fi driver has been initialized, the application will see it as an Ethernet interface.
To use the Ethernet interface, the application can use `Zephyr Network APIs`_.

See the :ref:`nrfxlib:nrf_wifi_api` to learn more about various modes of low-level API.

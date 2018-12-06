.. _changelog:

Change log
##########

This change log contains information about the current state of the |NCS| repositories.

For information about stable releases, see the release notes for the respective release.

.. important::
   The |NCS| follows `Semantic Versioning 2.0.0`_.
   The "99" in the version number of this documentation indicates continuous updates on the master branch since the previous major and minor release.
   There might be additional changes on the master branch that are not listed here.


Repositories
************
.. list-table::
   :header-rows: 1

   * - Component
     - Branch
   * - `fw-nrfconnect-nrf <https://github.com/NordicPlayground/fw-nrfconnect-nrf>`_
     - master
   * - `nrfxlib <https://github.com/NordicPlayground/nrfxlib>`_
     - master
   * - `_fw-nrfconnect-zephyr <https://github.com/NordicPlayground/_fw-nrfconnect-zephyr>`_
     - nrf91
   * - `fw-nrfconnect-mcuboot <https://github.com/NordicPlayground/fw-nrfconnect-mcuboot>`_
     - master


Supported boards
****************

* PCA10028 (nRF51 Development Kit)
* PCA10040 (nRF52 Development Kit)
* PCA10056 (nRF52840 Development Kit)
* PCA10059 (nRF52840 Dongle)
* PCA63519 (Smart Remote 3 DK add-on)
* PCA20041 (TBD)
* PCA10090 (nRF9160 DK)


Required tools
**************

In addition to the tools mentioned in :ref:`gs_installing`, the following tool versions are required to work with the |NCS|:

.. list-table::
   :header-rows: 1

   * - Tool
     - Version
     - Download link
   * - SEGGER J-Link
     - V6.40
     - `J-Link Software and Documentation Pack`_
   * - nRF5x Command Line Tools
     - v9.8.1
     - * `nRF5x Command Line Tools Linux 32`_
       * `nRF5x Command Line Tools Linux 64`_
       * `nRF5x Command Line Tools Windows 32`_
       * `nRF5x Command Line Tools Windows 64`_
       * `nRF5x Command Line Tools OSX`_

Major changes
*************

The following list contains the most important changes since the last release:

* Added a secure boot sample for PCA10090, which provides a reference implementation of a first-stage boot firmware. The sample configures resources for the secure domain and boots an application from the non-secure domain. See :ref:`secure_boot`.
* Added **at_host** library. This library helps creating an AT command socket and forwards requests and responses from and to the modem.
* Added **at_client** sample. This sample uses the **at_host** library to provide a UART interface for AT commands.
* Added a porting library for the BSD socket library that is located in the nrfxlib repository.
* Added a library that uses the MQTT protocol over BSD sockets. This library will be replaced by the upstream Zephyr library in the future.
* Added **nrf_cloud** library. This library implements features to connect and send data to nRF Cloud services.
* Added **nrf_cloud** sample for PCA10090. This sample uses the **nrf_cloud** library.
* Added **gps_sim** library. This library simulates a simple GPS device providing NMEA strings with generated data that can be accessed through the GPS API.
* Added **sensor_sim** library. This library simulates a sensor device that can be accessed through the sensor API, currently supporting the acceleration channels in the API.
* Added :doc:`MCUboot <mcuboot:index>` documentation.
* Added documentation for the :ref:`peripheral_uart` sample and the :ref:`nus_service_readme`.
* Added **scan** library for BLE. This library handles BLE scanning for your application. See :ref:`nrf_bt_scan_readme`.
* Added **gatt_dm** library for BLE. This library handles service discovery on BLE GATT servers. See :ref:`gatt_dm_readme`.
* Added **central_hids** BLE sample. This sample connects to HID devices and uses **gatt_dm** library to perform HID service discovery.
* Added **ndef** libraries for NFC. These libraries handle NDEF records and message generation. Text and URI records are supported for now.
* Added **record_text** NFC sample. This sample uses the NFC Type 2 Tag to expose a Text record to NFC polling devices. It requires the binary libraries in the nrfxlib repository.
* Added **writable_ndef_msg** NFC sample. This sample uses the NFC Type 4 Tag to expose an NDEF message, which can be overwritten by NFC polling devices. It requires the binary libraries in the nrfxlib repository.

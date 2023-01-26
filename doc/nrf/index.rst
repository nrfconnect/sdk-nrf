.. _index:

Welcome to the |NCS|!
#####################

.. important::

   |NCS| v2.2.99-dev2 is a development tag that replaces |NCS| v2.2.99-dev1.
   |NCS| v2.2.99-dev2 will be replaced by v2.3.0 in the future.

   The development tag contains the following major updates:

   Wi-Fi® features:

   * MAC address provisioning from host
   * Regulatory domain configuration and IEEE 802.11d support
   * Updated TX power settings

   Samples:

   * Added :ref:`wifi_sr_coex_sample` sample demonstrating coexistence of concurrent Wi-Fi and Bluetooth® Low Energy operation.
   * Enhancements to :ref:`wifi_radio_test` sample.
   * Added host sleep functionality to :ref:`wifi_station_sample` sample.
   * Added QSPI master encryption key programming to :ref:`wifi_station_sample` sample.

   For other changes that are included in this development tag, see :ref:`ncs_release_notes_changelog`.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.

The SDK contains optimized cellular IoT (LTE-M and NB-IoT), Bluetooth LE, Thread, Zigbee, and Bluetooth mesh stacks, a range of applications, samples, and reference implementations, as well as a full suite of drivers for Nordic Semiconductor's devices.
The |NCS| includes the Zephyr™ real-time operating system (RTOS), which is built for connected low power products.

To access different versions of the |NCS| documentation, use the version drop-down in the top right corner.
A "99" at the end of the version number of this documentation indicates continuous updates on the main branch since the previous major.minor release.

.. toctree::
   :maxdepth: 2
   :caption: Contents

   introduction
   getting_started
   ug_dev_model
   ug_app_dev
   security_chapter
   ug_nrf91
   ug_nrf70
   ug_nrf53
   ug_nrf52
   ug_nrf_cloud
   protocols
   applications
   samples
   drivers
   libraries/index
   scripts
   release_notes
   software_maturity
   documentation
   glossary

..   cheat_sheet

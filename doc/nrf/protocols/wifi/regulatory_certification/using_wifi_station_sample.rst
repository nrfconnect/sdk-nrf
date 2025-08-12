.. _ug_using_wifi_station_sample:

Using the Wi-Fi Station sample
##############################

.. contents::
   :local:
   :depth: 2

The Wi-Fi® Station (STA) sample allows you to test Nordic Semiconductor’s Wi-Fi chipsets and is used to demonstrate how to connect a Wi-Fi station to a specified access point using the Dynamic Host Configuration Protocol (DHCP).
This section describes the build and usage of the Wi-Fi STA sample.

For more information on the samples in |NCS|, see the following:

* Overall description: :ref:`wifi_station_sample` sample

Build instructions
******************

For information on the build instructions, see :ref:`Wi-Fi Station sample building and running <wifi_station_sample_building_and_running>`.

Set the :kconfig:option:`CONFIG_NRF70_UTIL` Kconfig option to ``y`` in the :file:`<ncs_repo>/nrf/samples/wifi/sta/prj.conf` file to enable the ``nrf70 util`` commands.

Wi-Fi Station sample
********************

The Wi-Fi Station (STA) sample is designed to be built with an SSID and a password, which are set in the :file:`prj.conf` file.
When run on an nRF70 Series board, such as the nRF7002 Development Kit (DK), it automatically connects to the Wi-Fi access point.
If the connection is successful, **LED1** on the DK blinks.
If the connection is lost, **LED1** stops blinking.
The process is repeated every time a board reset button is pressed.

By default, the nRF7002 board obtains an IP address through DHCP protocol exchanges with the access point.
If the DHCP exchange fails and the board does not acquire an IP address, you can set a static IP address in the :file:`prj.conf` file, which then serves as the default.
If the DHCP exchange succeeds, the acquired IP address is used in place of any static IP address settings.

.. note::
   This sample does not support UART shell.
   The UART console only displays debug information from the sample.

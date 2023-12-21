.. _ug_nrf70_developing_raw_ieee_80211_packet_reception_using_monitor_mode:

Raw IEEE 802.11 packet reception using Monitor mode
###################################################

.. contents::
   :local:
   :depth: 2

The nRF70 Series device supports Monitor mode, that is, listening for and receiving all IEEE 802.11 traffic on a given channel, not restricted to a certain SSID. With Monitor mode raw IEEE 802.11 packets can be received directly at the user application layer.
The nRF70 device has to be configured in monitor mode in order to operate as an 802.11 wireless packet sniffer.

In Monitor mode, raw 802.11 packets received at the nRF70 Series device may be presented directly at the host application layer. When forwarding the received IEEE 802.11 packets to the user application, the Wi-Fi host driver will append a raw RX header to each received packet and deliver it up to the application layer using the raw socket API.
The raw RX header holds the receive parameters for the received packet, such as data rate, receive signal strength, frequency of reception, and packet type (whether it is Legacy, HT, VHT, or HE).

.. _ug_nrf70_developing_enabling_802.11_monitor_mode:

Enabling Monitor mode
*********************

To be able to use the Monitor mode in your application, you need to enable the :kconfig:option:`CONFIG_NRF700X_RAW_DATA_RX` Kconfig option in the project configuration.

.. _ug_nrf70_developing_monitor_mode_operation:

Monitor mode operation
**********************

To enable the Monitor mode in the nRF Wi-Fi driver at runtime, use the ``NET_REQUEST_WIFI_MODE`` network management runtime API.
This runtime API can be used to disable the Monitor mode in the driver, when packet sniffer operation is not required anymore in the user application.

When the nRF70 device is configured to operate in Monitor mode, simultaneous connected Station operation is not permitted (the nRF70 Series device operates exclusively as a Wi-Fi sniffer).

The user needs to configure the operating Wi-Fi channel on which the nRF70 device will monitor and receive 802.11 packets.
When the device operates in Monitor mode, all 802.11 packets received on the configured channel will be sent up the stack by the nRF70 Series device and the Wi-Fi driver.

To set the desired channel for 802.11 packet reception, use the ``NET_REQUEST_WIFI_CHANNEL`` runtime NET-management API.
The channel configuration runtime NET-management API can be used to set the channel after setting the nRF70 Series device in Monitor mode.

See the :ref:`wifi_shell_sample` sample for more information on configuring mode and channel settings for raw packet reception through shell commands.

The following table lists the shell commands and NET-management APIs that are used to enable Monitor and TX injection mode and select the desired Wi-Fi channel:

.. list-table:: Wi-Fi packet reception network management APIs
   :header-rows: 1

   * - Network management APIs
     - Wi-Fi shell command
     - Example usage
     - Description
   * - net_mgmt(NET_REQUEST_WIFI_MODE)
     - ``wifi mode -i<interface instance> <configuration>``
     - ``wifi mode -i1 -m``
     - Configure interface instance 1 to Monitor mode
   * - net_mgmt(NET_REQUEST_WIFI_MODE)
     - ``wifi mode -i<interface instance> <configuration>``
     - ``wifi mode -i1 -s``
     - Change interface instance 1 to Station mode
   * - net_mgmt(NET_REQUEST_WIFI_CHANNEL)
     - ``wifi channel -i<interface instance> <channel number to set>``
     - ``wifi channel -i1 -c6``
     - Set the raw transmission channel to ``6`` for interface 1

.. _ug_nrf70_developing_monitor_mode_receive_packet_metadata:

Monitor mode receive packet metadata
************************************

The IEEE 802.11 packet that is captured by the nRF70 Series device will prepend a proprietary header ``raw_rx_pkt_header`` which contains the following information about the received 802.11 packet.

.. list-table:: Wi-Fi packet reception header elements
   :header-rows: 1

   * - Parameter name
     - Type
     - Description
   * - frequency
     - unsigned short
     - Provides the current frequency in which the packet was received.
       The frequecy element can be used to compute the packet receive channel.
   * - signal
     - signed short
     - Signal strength of the received packet
   * - rate_flags
     - unsigned char
     - Provides data on whether the received 802.11 packet is a legacy, HT, VHT or a HE packet
   * - rate
     - unsigned char
     - Provides the data rate at which the packet was received.
       It can be a ``legacy rate`` or an ``MCS rate`` based on the ``rate_flags`` value.

You can refer to the relevant structure at:

:file:`nrfxlib/nrf_wifi/fw_if/umac_if/inc/default/fmac_structs.h` - for the raw RX header

.. _ug_nrf70_developing_monitor_mode_receive_operation:

Monitor mode receive operation
******************************

An IEEE 802.11 packet captured by the nRF70 Series device in Monitor mode will be sent up to the Wi-Fi driver which will append a radio information header to the received packet and present the resulting set to the user application.
To receive the captured traffic in Monitor mode, the user application layer needs to open a raw socket to the Wi-Fi driver.

The following figure illustrates the packet structure and IEEE 802.11 packet sniffer operational flow:

.. figure:: images/nrf7000_packet_sniffer_operation.svg
   :alt: IEEE 802.11 packet sniffer operational flow

.. _ug_nrf70_developing_monitor_mode_receive_operation_with_tx_injection:

Monitor mode receive operation in conjunction with TX injection
***************************************************************

TX injection mode can be enabled for operation when the nRF70 Series device is configured to operate in Monitor mode.

TX injection mode allows the transmission of a raw 802.11 transmit packet.
Raw IEEE 802.11 packets are packets that are not modified by the 802.11 Medium Access Control (MAC) layer during transmission by the nRF70 Series device.

TX injection mode can be enabled by invoking the API ``net_eth_txinjection_mode``.
The API has the following parameters as described below.

.. list-table:: TX injection mode API
   :header-rows: 1

   * - Parameter name
     - Type
     - Description
   * - iface
     - struct net_if
     - Network Interface structure
   * - enable
     - boolean
     - Parameter to enable/disable TX injection mode.
       ``1`` enables TX injection and ``0`` disables TX injection.

.. note::
   You must enable the :kconfig:option:`CONFIG_NRF700X_RAW_DATA_TX` Kconfig option for TX injection mode

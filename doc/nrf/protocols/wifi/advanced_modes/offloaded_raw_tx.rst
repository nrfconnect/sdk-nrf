.. _ug_nrf70_developing_offloaded_raw_tx:

Offloaded raw transmit operation
################################

.. contents::
   :local:
   :depth: 2

The nRF70 Series ICs can be used as offloaded raw transmit devices, where the nRF70 Series device can transmit frames at regular intervals utilizing very low power.
The contents of the frame as well as parameters such as frequency and channel of transmission are programmable.

The major functionality of transmitting the frames is offloaded to the nRF70 device, thereby placing minimal requirements on the host (mainly programming capability).
This results in minimal host memory requirements (RAM and flash memory).

This can be used for applications such as indoor navigation and tracking, where it is essential for anchor nodes to perform low-power beaconing.
Anchor devices can transmit beacon-compliant packets containing tracking or location information inside the BSSID or SSID fields.
Devices scanning for these beacon-compliant packets can use this information.

Offloaded raw TX mode in Wi-Fi driver
*************************************

The offloaded raw transmit operation is supported as a separate stand-alone compile-time mode of operation in the nRF Wi-Fi driver and is exclusive to the following existing modes of operation:

* Wi-Fi mode
* Radio Test mode

In addition to providing start or stop control over the offloaded raw transmit operation, the driver supports the update of the following configuration parameters:

* Frame contents
* Channel of operation
* Data rate
* Rate flags
* Periodicity of transmission
* Transmit power

.. _ug_nrf70_developing_enabling_offloaded_raw_tx:

Offloaded raw transmit API
**************************

The offloaded raw transmit functionality of nRF70 Series ICs can be utilized by using the APIs provided by the driver.
The API reference can be found at:

| Header file: :file:`zephyr/drivers/wifi/nrfwifi/off_raw_tx/off_raw_tx_api.h`


See the :ref:`Offloaded raw transmit sample <wifi_offloaded_raw_tx_packet_sample>` to know more about the offloaded raw transmit API.

.. _ug_nrf70_developing_offloaded_raw_tx_power_consumption:

Power consumption
*****************

The power consumed by the nRF70 Series device during the offloaded raw TX operation depends on the following parameters:

* Operating data rate (for example, 6 Mbps, MCS0) : Power consumption decreases as the data rate increases.
* Payload length : Power consumption increases with the payload length.
* Periodicity of transmission : Power consumption increases as the period between successive transmissions decreases.
* Transmit power : Power consumption increases as the transmit power increases.

For optimizing the power consumption of your application, you can use the `Online Power Profiler for Wi-Fi`_ tool.

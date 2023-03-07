.. _ug_nrf_cloud:

Using nRF Cloud with the |NCS|
##############################

.. contents::
   :local:
   :depth: 2

`nRF Cloud`_ is `Nordic Semiconductor's IoT cloud platform`_ that offers services in the fields of connectivity, device management, and location services.

Overview
********

nRF Cloud allows you to remotely manage and update your IoT devices using :term:`Firmware Over-the-Air (FOTA) <Firmware Over-the-Air (FOTA) update>`.
FOTA can be used to update the device application, bootloader, and modem.
The modem can be incrementally updated with a modem delta image.
If the device has sufficiently large external flash storage, the modem can be entirely updated with a full modem image.

nRF Cloud also helps your devices determine their location using assisted GPS (A-GPS) and predicted GPS (P-GPS).
It can determine device location from cellular and Wi-Fi network information sent by the device.

Additionally, nRF Cloud allows devices to report data to the cloud for collection and analysis later.
To read more about nRF Cloud, see the `nRF Cloud`_ website and the `nRF Cloud documentation`_.

You can use the services offered by nRF Cloud in the following scenarios:

* Device connected to nRF Cloud over MQTT. The services can be used from nRF Cloud.
* Device connected to nRF Cloud over MQTT, with a customer-developed website or application that interacts with the `nRF Cloud REST API`_ to display device data and manage it in a customized way.
* Device connected to nRF Cloud over REST, interacting using the `nRF Cloud REST API`_.
* Device connected to a customer cloud service in a suitable manner. The services can be used from the customer cloud service that communicates over REST to the nRF Cloud REST API in a proxy configuration.

Choosing a protocol: MQTT or REST
*********************************

When choosing a protocol, consider the following:

* How often does the device transmit data?
* Which cloud APIs does the device need to access?
* What are the power consumption requirements for the device?
* What are the network data usage requirements for the device?
* What are the carrier's network settings (NAT timeout, eDRX/PSM) and how will the settings affect device behavior?

MQTT has a higher (data/power) cost to set up a connection.  However, the data size of an MQTT publish event is smaller than a comparable REST transaction.
MQTT may be preferred if a device is able to maintain a connection to the broker and sends/receives data frequently.
REST may be preferred if a device sends data infrequently or does not need to receive unsolicited data from the cloud.

REST overview
=============

* The device initiates a TLS connection to nRF Cloud.
* nRF Cloud supports a connection keep-alive/idle time of 60 seconds for REST API sockets.
* For authentication, the device must send a JSON Web Token (JWT) with each REST transaction.
  The JWT is approximately 450 bytes, but can be larger depending on the claims.
* Each REST transaction contains HTTP headers, including the JWT, and any API specific payload.

MQTT overview
=============

* The device initiates a mutual-TLS (mTLS) connection to the nRF Cloud MQTT broker.
* The MQTT keep-alive time can be set by the device and can be longer than 60s.
* Device authentication through mTLS lasts throughout the MQTT connection.
* Once connected, the device subscribes to the desired MQTT topics.
* Each MQTT publish event contains the MQTT topic and the payload.

Security
********

A device can successfully connect to `nRF Cloud`_ using REST if the following requirements are met:

* The device contains a correct x509 CA certificate, and private key.
* The public key derived from the private key is registered with an nRF Cloud account.
* The device calls nRF Cloud REST APIs which require a JSON Web Token (JWT) by providing a JWT signed by the private key

A device can successfully connect to `nRF Cloud`_ using MQTT if the following requirements are met:

* The device contains a correct x509 CA certificate, device certificate, and private key.
* The device ID and device certificate are provisioned with nRF Cloud.
* The device ID is associated with an nRF Cloud account.

`nRF Cloud`_ supports the following two ways for creating and installing these certificates both in the device and the cloud:

* Just in time provisioning

  1. In your nRF Cloud account, enter the device ID in a web form, then download a JSON file containing the CA certificate, device certificate, and private key.

     Alternatively, use the nRF Cloud REST API to do this.

  #. Program the credentials in the JSON file into the device using LTE Link Monitor.

  The private key is exposed during these steps, and therefore, this is the less secure option.
  See :ref:`nrf9160_ug_updating_cloud_certificate` for details.

* Preconnect provisioning

  1. Run the `device_credentials_installer.py`_ Python script to create and install credentials on the device:

     * You need to specify a number of parameters including the device ID.
     * The script instructs the device to securely generate and store a private key.
     * The private key never leaves the device, which makes this a more secure option.
     * It creates a device certificate and signs it with the specified CA.
     * It writes the device certificate and AWS CA certificate to the device.

  #. Run the `nrf_cloud_provision.py`_ script to provision and associate the device with your nRF Cloud account.

  For more details about the scripts, refer to the `nRF Cloud Utilities documentation`_.

  See `Securely generating credentials on the nRF9160`_  and `nRF Cloud Provisioning`_ for more details.


|NCS| library support
*********************

The |NCS| provides the :ref:`lib_nrf_cloud` library, which if enabled, allows you to connect your devices to nRF Cloud and use the update, location, and connectivity services using MQTT or REST.

For more information on the various services, see the following documentation:

1. :ref:`lib_nrf_cloud_agps`
#. :ref:`lib_nrf_cloud_cell_pos`
#. :ref:`lib_nrf_cloud_fota`
#. :ref:`lib_nrf_cloud_pgps`

Applications and samples
************************

The following application uses the :ref:`lib_nrf_cloud` for services in |NCS|:

* :ref:`asset_tracker_v2`

The following sample demonstrates nRF Cloud-specific functionality using MQTT:

* :ref:`nrf_cloud_mqtt_multi_service`

The following samples demonstrate nRF Cloud-specific functionality using REST:

* :ref:`nrf_cloud_rest_fota`
* :ref:`nrf_cloud_rest_device_message`
* :ref:`nrf_cloud_rest_cell_pos_sample`

Other related samples and applications that use nRF Cloud services:

* :ref:`gnss_sample`
* :ref:`modem_shell_application`
* :ref:`lte_sensor_gateway`
* :ref:`location_sample`
* :ref:`serial_lte_modem`

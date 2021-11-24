.. _lib_nrf_cloud:

nRF Cloud
#########

.. contents::
   :local:
   :depth: 2

The nRF Cloud library enables applications to connect to Nordic Semiconductor's `nRF Cloud`_.
It abstracts and hides the details of the transport and the encoding scheme that is used for the payload and provides a simplified API interface for sending data from supported sensor types to the cloud.
The current implementation supports the following technologies:

* GPS and FLIP sensor data
* TLS secured MQTT as the communication protocol
* JSON as the data format


.. _lib_nrf_cloud_init:

Initializing
************
Before using any other APIs of the module, the application must call :c:func:`nrf_cloud_init`.
If this API fails, the application must not use any APIs of the module.

.. note::
   Initialize the module before starting any timers, sensor drivers, or communication on the link.

.. _lib_nrf_cloud_connect:

Connecting
**********
The application can use :c:func:`nrf_cloud_connect` to connect to the cloud.
This API triggers a series of events and actions in the system.
If the API fails, the application must retry to connect.
If the :kconfig:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option is enabled, an nRF Cloud library thread monitors the connection socket.
When :kconfig:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` is enabled, an additional event, :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECTING`, is sent to the application.
The status field of :c:struct:`nrf_cloud_evt` contains the connection status that is defined by :c:enumerator:`nrf_cloud_connect_result`.
The event :c:enumerator:`NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED` also contains additional information in the status field that is defined by :c:enumerator:`nrf_cloud_disconnect_status`.
See :ref:`connect_to_cloud` for additional information on :kconfig:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD`.
When the poll thread option is used directly with the nRF Cloud library, the enumeration values are prefixed with ``NRF``.

First, the library tries to establish the transport for communicating with the cloud.
This procedure involves a TLS handshake that might take up to three seconds.
The API blocks for the duration of the handshake.

The cloud uses the certificates of the device for authentication.
See `Updating the nRF Cloud certificate`_ and the :ref:`modem_key_mgmt` library for more information on modem credentials.
The certificates are generated using the device ID and PIN or HWID.
The device ID is also the MQTT client ID.
There are multiple configuration options for the device or client ID.
See :ref:`config_device_id` for more information.

As the next step, the API subscribes to an MQTT topic to start receiving user association requests from the cloud.

Every time nRF Cloud starts a communication session with a device, it verifies whether the device is uniquely associated with a user.
If not, the user association procedure is triggered.
When adding the device to an nRF Cloud account, the user must provide the correct device ID and PIN (for Thingy:91 and custom hardware) or HWID (for nRF9160 DK) to nRF Cloud.

The following message sequence chart shows the flow of events and the expected application responses to each event during the user association procedure:

.. msc::
   hscale = "1.3";
   Module,Application;
   Module<<Application      [label="nrf_cloud_connect() returns successfully"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST"];
    ---                     [label="Add the device to nRF Cloud account"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
   Module<<Application      [label="nrf_cloud_disconnect() returns successfully"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED"];
   Module<<Application      [label="nrf_cloud_connect() returns successfully"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
   Module>>Application      [label="NRF_CLOUD_EVT_READY"];

The chart shows the sequence of successful user association of an unassociated device.

.. note::

   Currently, nRF Cloud requires that communication is re-established to update the device's permission to send user data.
   The application must disconnect using :c:func:`nrf_cloud_disconnect` and then reconnect using :c:func:`nrf_cloud_connect`.

When the device is successfully associated with a user on the cloud, subsequent connections to the cloud (also across power cycles) occur in the following sequence:

.. msc::
   hscale = "1.3";
   Module,Application;
   Module<<Application      [label="nrf_cloud_connect() returns successfully"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
   Module>>Application      [label="NRF_CLOUD_EVT_READY"];

After receiving :c:enumerator:`NRF_CLOUD_EVT_READY`, the application can start sending sensor data to the cloud.

.. _config_device_id:

Configuration options for device ID
===================================

* :kconfig:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI` - If you enable this option, the ID is automatically generated using a prefix and the modem's IMEI (``<prefix><IMEI>``). You can configure the prefix by using :kconfig:`CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX`. The default format of the prefix is ``nrf-`` and it is valid only for Nordic devices such as Thingy:91 or nRF9160 DK. For custom hardware, use a prefix other than ``nrf-`` by modifying :kconfig:`CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX`.

* :kconfig:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID` - If you enable this option, the ID is automatically generated using the modem's 128-bit internal UUID, which results in a 32-character string with no hyphens. This option requires modem firmware v1.3.0 or higher.

* :kconfig:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME` - If you enable this option, the ID is set at compile time using the value specified by :kconfig:`CONFIG_NRF_CLOUD_CLIENT_ID`.

* :kconfig:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` - If you enable this option, the ID is set at run time. If the nRF Cloud library is used directly, set the NULL-terminated ID string in :c:struct:`nrf_cloud_init_param` when calling :c:func:`nrf_cloud_init`. If the generic Cloud API is used, set the ID in :c:struct:`cloud_backend_config` and then call :c:func:`cloud_init`.

.. _lib_nrf_cloud_fota:

Firmware over-the-air (FOTA) updates
************************************
The nRF Cloud library supports FOTA updates for your nRF9160-based device.
When the library is included by the application, the :kconfig:`CONFIG_NRF_CLOUD_FOTA` option is enabled by default, and the FOTA functionality is made available to the application.

For FOTA updates to work, the device must provide the information about the supported FOTA types to nRF Cloud.
The device passes this information by writing a ``fota_v2`` field containing an array of FOTA types into the ``serviceInfo`` field in the device's shadow.
:c:func:`nrf_cloud_service_info_json_encode` can be used to generate the proper JSON data to enable FOTA.
Additionally, :c:func:`nrf_cloud_shadow_device_status_update` can be used to generate the JSON data and perform the shadow update.

Following are the three supported FOTA types:

* ``APP``
* ``MODEM``
* ``BOOT``

For example, a device that supports all the FOTA types writes the following data into the device shadow:

.. code-block::

   {
   "state": {
      "reported": {
         "device": {
            "serviceInfo": {
               "fota_v2": [
               "APP",
               "MODEM",
               "BOOT"
               ]
   }}}}}

You can initiate FOTA updates through `nRF Cloud`_ or by using the `nRF Cloud Device API`_.
When the device receives the :c:enumerator:`NRF_CLOUD_EVT_FOTA_DONE` event, the application must perform any necessary cleanup, as a reboot will be initiated to complete the update.
The message payload of the :c:enumerator:`NRF_CLOUD_EVT_FOTA_DONE` event contains the :c:enum:`nrf_cloud_fota_type` value.
If the value equals :c:enumerator:`NRF_CLOUD_FOTA_MODEM`, the application can optionally avoid a reboot by performing reinitialization of the modem and calling the :c:func:`nrf_cloud_modem_fota_completed` function.

.. _lib_nrf_cloud_data:

Sending sensor data
*******************
The library offers two APIs, :c:func:`nrf_cloud_sensor_data_send` and :c:func:`nrf_cloud_sensor_data_stream` (lowest QoS), for sending sensor data to the cloud.

To view sensor data on nRF Cloud, the device must first inform the cloud what types of sensor data to display.
The device passes this information by writing a ``ui`` field, containing an array of sensor types, into the ``serviceInfo`` field in the device's shadow.
:c:func:`nrf_cloud_service_info_json_encode` can be used to generate the proper JSON data to enable FOTA.
Additionally, :c:func:`nrf_cloud_shadow_device_status_update` can be used to generate the JSON data and perform the shadow update.

Following are the supported UI types on nRF Cloud:

* ``GPS``
* ``FLIP``
* ``TEMP``
* ``HUMIDITY``
* ``AIR_PRESS``
* ``RSRP``

.. _lib_nrf_cloud_unlink:

Removing the link between device and user
*****************************************

If you want to remove the link between a device and an nRF Cloud account, you must do this from nRF Cloud.
A device cannot remove itself from an nRF Cloud account.

.. _use_nrfcloud_cloudapi:

Using Cloud API with nRF Cloud library
**************************************
You can use this library in conjunction with :ref:`cloud_api_readme`.
The following sections describe the various stages in the process of connection to nRF Cloud.

Initialization
==============

To use a defined Cloud API backend, a binding must be obtained using the Cloud API function :c:func:`cloud_get_binding`, to which you can pass the name of the desired backend.
The nRF Cloud library defines the Cloud API backend as ``NRF_CLOUD`` using the :c:macro:`CLOUD_BACKEND_DEFINE` macro.

The backend must be initialized using the :c:func:`cloud_init` function, with the binding, and a function pointer to user-defined Cloud API event handler as parameters.
If :c:func:`cloud_init` returns success, the backend is ready for use.
The return values for a failure scenario of the :c:func:`cloud_init` function are described below for the nRF Cloud backend:

*	-EACCES - Invalid state. Already initialized.
*	-EINVAL - Invalid event handler provided.
*	-ENOMEM - Error building MQTT topics. The given client ID of the device could be too long.

.. note::
   If :kconfig:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` is enabled, error values could be different or have different error descriptions.

.. _connect_to_cloud:

Connecting to the Cloud
=======================

The nRF Cloud library offers two ways to handle backend connections when the :c:func:`cloud_connect` function is called.
If you enable the :kconfig:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option, a cloud backend thread monitors the connection socket.
If you disable the option, the user application must monitor the socket.

The dual functionalities of the :c:func:`cloud_connect` function in the two scenarios are described below:

* :kconfig:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` - If you enable this option, the function does not block and returns success if the connection monitoring thread has started. Below are some of the error codes that can be returned:

   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_NOT_INITD` - Cloud backend is not initialized
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED` - Connection process has already been started

  Upon success, the monitoring thread sends an event of type :c:enumerator:`CLOUD_EVT_CONNECTING` to the user’s cloud event handler, with the ``err`` field set to success. If an error occurs, another event of the same type is sent, with the ``err`` field set to indicate the cause. These additional errors are described in the following section.

* :kconfig:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` - If you disable this option, the function blocks and returns success when the MQTT connection to the cloud completes. The connection socket is set in the backend binding and it becomes available for the application to use. Below are some of the error codes that can be returned:

   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_NOT_INITD`.
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_NETWORK` - Host cannot be found with the available network interfaces.
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_BACKEND` - A backend-specific error. In the case of nRF Cloud, this can indicate a FOTA initialization error.
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_MISC` -  Error cause cannot be determined.
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_NO_MEM` - MQTT RX/TX buffers were not initialized.
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_PRV_KEY` - Invalid private key.
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_CERT` - Invalid CA or client certificate.
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_CERT_MISC` - Miscellaneous certificate error.
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA` - Timeout. Typically occurs when the inserted SIM card has no data.

  For both connection methods, when a device with JITP certificates attempts to connect to nRF Cloud for the first time, the cloud rejects the connection attempt so that it can provision the device.
  When this occurs, the Cloud API generates a :c:enumerator:`CLOUD_EVT_DISCONNECTED` event with the ``err`` field set to :c:enumerator:`CLOUD_DISCONNECT_INVALID_REQUEST`.
  The device must restart the connection process upon receipt of the :c:enumerator:`CLOUD_EVT_DISCONNECTED` event.

Connected to the Cloud
======================

When the device connects to the cloud successfully, the Cloud API dispatches a :c:enumerator:`CLOUD_EVT_CONNECTED` event.
If the device is not associated with an nRF Cloud account, a :c:enumerator:`CLOUD_EVT_PAIR_REQUEST` event is generated.
The device must wait until it is added to an account, which is indicated by the :c:enumerator:`CLOUD_EVT_PAIR_DONE` event.
If a device pair request is received, the device must disconnect and reconnect after receiving the :c:enumerator:`CLOUD_EVT_PAIR_DONE` event.
This is necessary because the updated policy of the cloud becomes effective only on a new connection.
Following the :c:enumerator:`CLOUD_EVT_PAIR_DONE` event, the Cloud API sends a :c:enumerator:`CLOUD_EVT_READY` event to indicate that the cloud is ready to receive data from the device.

Disconnection from the Cloud
============================

The user application can generate a disconnect request with the :c:func:`cloud_disconnect` function.
A successful disconnection is indicated by the :c:enumerator:`CLOUD_EVT_DISCONNECTED` event.
The ``err`` field in the event message is set to :c:enumerator:`CLOUD_DISCONNECT_USER_REQUEST`.
If an unexpected disconnect event is received, the ``err`` field contains the cause.
If :kconfig:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` is not enabled, the only cause of disconnection is :c:enumerator:`CLOUD_DISCONNECT_MISC`.
The user application must use the connection socket to determine a reason.

The following events can cause disconnection if the backend thread is monitoring the socket:

* :c:enumerator:`CLOUD_DISCONNECT_CLOSED_BY_REMOTE` - The connection was closed by the cloud (POLLHUP).
* :c:enumerator:`CLOUD_DISCONNECT_INVALID_REQUEST` - The connection is no longer valid (POLLNVAL).
* :c:enumerator:`CLOUD_DISCONNECT_MISC` - Miscellaneous error (POLLERR).

.. _lib_nrf_cloud_location_services:

Location services
*****************

`nRF Cloud`_ offers location services that allow you to obtain the location of your device.
The following enhancements to this library can be used to interact with `nRF Cloud Location Services`_:

* Assisted GPS - :ref:`lib_nrf_cloud_agps`
* Predicted GPS - :ref:`lib_nrf_cloud_pgps`
* Cellular Positioning - :ref:`lib_nrf_cloud_cell_pos`
* nRF Cloud REST  - :ref:`lib_nrf_cloud_rest`

.. _nrf_cloud_api:

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud
   :project: nrf
   :members:

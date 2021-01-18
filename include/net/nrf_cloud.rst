.. _lib_nrf_cloud:

nRF Cloud
#########

.. contents::
   :local:
   :depth: 2

The nRF Cloud library enables applications to connect to Nordic Semiconductor's `nRF Connect for Cloud`_.
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
If the :option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option is enabled, an nRF Cloud library thread monitors the connection socket.
When :option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` is enabled, an additional event, :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECTING`, is sent to the application.
The status field of :c:struct:`nrf_cloud_evt` contains the connection status that is defined by :c:enumerator:`nrf_cloud_connect_result`.
The event :c:enumerator:`NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED` also contains additional information in the status field that is defined by :c:enumerator:`nrf_cloud_disconnect_status`.
Additional details for :option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` can be found in the :ref:`connectg_to_cloud` section.
When the poll thread option is used directly with the nRF Cloud library, the enumeration values are prefixed with ``NRF``.

First, the library tries to establish the transport for communicating with the cloud.
This procedure involves a TLS handshake that might take up to three seconds.
The API blocks for the duration of the handshake.

Next, the API subscribes to an MQTT topic to start receiving user association requests from the cloud.
The cloud uses the certificates of the device for authentication.
See `Updating the nRF Connect for Cloud certificate`_ and the :ref:`modem_key_mgmt` library for more information on modem credentials.
The certificates are generated using the device ID and PIN/HWID.

Every time nRF Connect for Cloud starts a communication session with a device, it verifies whether the device is uniquely associated with a user.
If not, the user association procedure is triggered.
When adding the device to an nRF Connect for Cloud account, the user must provide the correct device ID and PIN (for Thingy:91) or HWID (for nRF9160 DK) to nRF Cloud.

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
   
   Currently, nRF Connect for Cloud requires that communication is re-established to update the device's permission to send user data.
   The application must disconnect using :c:func:`nrf_cloud_disconnect` and then reconnect using :c:func:`nrf_cloud_connect`.

When the device is successfully associated with a user on the cloud, subsequent connections to the cloud (also across power cycles) follow this sequence:

.. msc::
   hscale = "1.3";
   Module,Application;
   Module<<Application      [label="nrf_cloud_connect() returns successfully"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
   Module>>Application      [label="NRF_CLOUD_EVT_READY"];

After receiving :c:enumerator:`NRF_CLOUD_EVT_READY`, the application can start sending sensor data to the cloud.

.. _lib_nrf_cloud_data:

Sending sensor data
*******************
The library offers two APIs, :c:func:`nrf_cloud_sensor_data_send` and :c:func:`nrf_cloud_sensor_data_stream`, for sending sensor data to the cloud.
Currently, the supported sensor types are GPS and FLIP (see :c:enum:`nrf_cloud_sensor`).

Use :c:func:`nrf_cloud_sensor_data_stream` to send sensor data with best quality.

Before sending any sensor data, call the function :c:func:`nrf_cloud_sensor_attach` with the type of the sensor.
Note that this function must be called after receiving the event :c:enumerator:`NRF_CLOUD_EVT_READY`.
It triggers the event :c:enumerator:`NRF_CLOUD_EVT_SENSOR_ATTACHED` if the execution was successful.

.. _lib_nrf_cloud_unlink:

Removing the link between device and user
*****************************************

If you want to remove the link between a device and an nRF Connect for Cloud user, you must do this from the nRF Connect for Cloud.
It is not possible for a device to unlink itself.

When a user disassociates a device, the library disallows any further sensor data to be sent to the cloud and generates an :c:enumerator:`NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST` event.
The application can then decide to associate again by responding with :c:func:`nrf_cloud_user_associate` with the new input sequence.
See the following message sequence chart:

.. msc:
   hscale = "1.3";
   Module,Application;
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST"];
   Module<<Application      [label="nrf_cloud_user_associate()"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
   Module>>Application      [label="NRF_CLOUD_EVT_READY"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED"];

.. _use_nrfcloud_cloudapi:

Using Cloud API with nRF Cloud library
**************************************
You can use this library in conjunction with :ref:`cloud_api_readme`.
The following sections describe the various stages in the process of connection to the nRF Connect for Cloud.

Initialization
==============

To use a defined Cloud API backend, a binding must be obtained using the Cloud API function :c:func:`cloud_get_binding`, to which you can pass the name of the desired backend.
The nRF Cloud library defines the Cloud API backend as ``NRF_CLOUD`` via the :c:macro:`CLOUD_BACKEND_DEFINE` macro.

The backend must be initialized using the :c:func:`cloud_init` function, with the binding, and a function pointer to user-defined Cloud API event handler as parameters.
If :c:func:`cloud_init` returns success, the backend is ready for use.
The return values for a failure scenario of the :c:func:`cloud_init` function are described below for the nRF Connect for Cloud backend:

*	-EACCES - Invalid state. Already initialized.
*	-EINVAL - Invalid event handler provided.
*	-ENOMEM - Error building MQTT topics. The given client ID of the device could be too long.

.. note::
   If :option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` is enabled, error values could be different or have different error descriptions.

.. _connectg_to_cloud:

Connecting to the Cloud
=======================

The nRF Cloud library offers two ways to handle backend connections when the :c:func:`cloud_connect` function is called.
If the :option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option is enabled, a cloud backend thread monitors the connection socket.
If the option is not enabled, the user application is responsible for monitoring the socket.

The dual functionalities of the :c:func:`cloud_connect` function in the two scenarios are described below:

:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` enabled
   Function does not block and returns success if the connection monitoring thread has started.
   Below are some of the error codes that can be returned:

   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_NOT_INITD` - Cloud backend is not initialized
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED` - Connection process has already been started

   Upon success, the monitoring thread sends an event of type :c:enumerator:`CLOUD_EVT_CONNECTING` to the user’s cloud event handler, with the ``err`` field set to success.
   If an error occurs, another event of the same type is sent, with the ``err`` field set to indicate the cause.
   These additional errors are described in the following section.

:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` disabled
   Function blocks and returns success when the MQTT connection to the cloud has completed.
   The connection socket is set in the backend binding and it becomes available for the application to use.
   Below are some of the error codes that can be returned:

   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_NOT_INITD`
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_NETWORK`: Host cannot be found with the available network interfaces
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_BACKEND`: A backend-specific error; In the case of nRF Connect for Cloud, this can indicate a FOTA initialization error
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_MISC`: Error cause cannot be determined
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_NO_MEM`: MQTT RX/TX buffers were not initialized
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_PRV_KEY`: Invalid private key
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_CERT`: Invalid CA or client certificate
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_CERT_MISC`: Miscellaneous certificate error
   * :c:enumerator:`CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA`: Timeout; typically occurs when the inserted SIM card has no data

  For both connection methods, when a device with JITP certificates attempts to connect to nRF Connect for Cloud for the first time, the cloud rejects the connection attempt so that it can provision the device.
  When this occurs, the Cloud API generates a :c:enumerator:`CLOUD_EVT_DISCONNECTED` event with the ``err`` field set to :c:enumerator:`CLOUD_DISCONNECT_INVALID_REQUEST`.
  The device should restart the connection process upon receipt of the :c:enumerator:`CLOUD_EVT_DISCONNECTED` event.

Connected to the Cloud
======================

When the connection between the device and the cloud has been successfully established, the Cloud API dispatches a :c:enumerator:`CLOUD_EVT_CONNECTED` event.
If the device is not associated with an nRF Connect for Cloud account, a :c:enumerator:`CLOUD_EVT_PAIR_REQUEST` event is generated.
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
If :option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` is not enabled, the only cause of disconnection is :c:enumerator:`CLOUD_DISCONNECT_MISC`.
The user application should use the connection socket to determine a reason.

If the socket is being monitored by the backend thread, the following causes of disconnection can occur:

* :c:enumerator:`CLOUD_DISCONNECT_CLOSED_BY_REMOTE` - The connection was closed by the cloud (POLLHUP).
* :c:enumerator:`CLOUD_DISCONNECT_INVALID_REQUEST` - The connection is no longer valid (POLLNVAL).
* :c:enumerator:`CLOUD_DISCONNECT_MISC` - Miscellaneous error (POLLERR).

.. _nrf_cloud_api:

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud
   :project: nrf
   :members:

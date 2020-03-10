.. _lib_nrf_cloud:

nRF Cloud library
#################

The nRF Cloud library enables applications to connect to Nordic Semiconductor's nRF Cloud. It abstracts and hides the details of the transport and the encoding scheme that is used for the payload and provides a simplified API interface for sending data from supported sensor types to the cloud. The current implementation supports the following technology:

* GPS and FLIP sensor data
* TLS secured MQTT as communication protocol
* JSON as data format

.. _lib_nrf_cloud_init:

Initializing
************
Before using any other APIs of the module, the application must call :cpp:func:`nrf_cloud_init`. If this API fails, the application must not use any APIs of the module.

.. note::
   Initialize the module before starting any timers, sensor drivers, or communication on the link.

.. _lib_nrf_cloud_connect:

Connecting
**********
The application can use :cpp:func:`nrf_cloud_connect` to connect to the cloud. This API triggers a series of events and actions in the system. If the API fails, the application must retry to connect.
:cpp:func:`nrf_cloud_connect` requires a list of supported features in the application.
Use the following code in your application to initialize this list:


.. code-block:: c

   /* Array of supported user association methods. */
   const enum nrf_cloud_ua supported_uas[] = {
		NRF_CLOUD_UA_BUTTON
   };

   /* Array of supported sensors. */
   const enum nrf_cloud_sensor supported_sensors[] = {
		NRF_CLOUD_SENSOR_GPS,
		NRF_CLOUD_SENSOR_FLIP
   };

   /* List of supported user association methods. */
   const struct nrf_cloud_ua_list ua_list = {
		.size = ARRAY_SIZE(supported_uas),
		.ptr = supported_uas
   };

   /* List of supported sensors. */
   const struct nrf_cloud_sensor_list sensor_list = {
		.size = ARRAY_SIZE(supported_sensors),
		.ptr = supported_sensors
   };

   /* Struct containing both lists. */
   const struct nrf_cloud_connect_param param = {
		.ua = &ua_list,
		.sensor = &sensor_list,
   };

   int err = nrf_cloud_connect(&param);


First, the library tries to establish the transport for communicating with the cloud. This procedure involves a TLS handshake that might take up to three seconds. The API blocks for the duration of the handshake.

Next, the API subscribes to an MQTT topic to start receiving user association requests from the cloud.

Every time nRF Cloud starts a communication session with a device, it verifies if the device is uniquely associated with a user. If not, the user association procedure is triggered. The following message sequence chart shows the flow of events and expected application responses to each event:

.. msc::
   hscale = "1.3";
   Module,Application;
   Module<<Application      [label="nrf_cloud_connect() returns successfully"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST"];
   Module<<Application      [label="nrf_cloud_user_associate()"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
   Module>>Application      [label="NRF_CLOUD_EVT_READY"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED"];

.. note::
   This chart shows the sequence of successful user association of an unassociated device. Currently, nRF Cloud requires that communication is re-established to update the device's permission to send user data. This is why the :cpp:enumerator:`NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED` event occurs. The application must reconnect to the cloud using the :cpp:func:`nrf_cloud_connect` API.

When the device is successfully associated with a user on the cloud, subsequent connections to the cloud (also across power cycles) follow this sequence:

.. msc::
   hscale = "1.3";
   Module,Application;
   Module<<Application      [label="nrf_cloud_connect() returns successfully"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
   Module>>Application      [label="NRF_CLOUD_EVT_READY"];

After receiving :cpp:enumerator:`NRF_CLOUD_EVT_READY`, the application can start sending sensor data to the cloud.

.. _lib_nrf_cloud_ua_failure:

User association failure
************************
User association might fail due to the following reasons:

* Mismatch in the input sequence from the device
* Time-out on the cloud

If there is a mismatch in the sequence, the library generates a new :cpp:enumerator:`NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST` event, and the user can try again. This event may be triggered several times until the cloud receives a matching sequence.

If a time-out occurs, :cpp:enumerator:`NRF_CLOUD_EVT_ERROR` is triggered and sent to the application. If this event is received, disconnect from the cloud using the :cpp:func:`nrf_cloud_disconnect` API. The application must wait for the :cpp:enumerator:`NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED` event before attempting a new connection to the cloud.

.. _lib_nrf_cloud_data:

Sending sensor data
*******************
The library offers two APIs, :cpp:func:`nrf_cloud_sensor_data_send` and :cpp:func:`nrf_cloud_sensor_data_stream`, for sending sensor data to the cloud. Currently, the supported sensor types are GPS and FLIP (see :cpp:enum:`nrf_cloud_sensor`).

Use :cpp:func:`nrf_cloud_sensor_data_stream` to send sensor data with best quality.

Before sending any sensor data, call the function :cpp:func:`nrf_cloud_sensor_attach` with the type of the sensor.
Note that this function must be called after receiving the event :cpp:enumerator:`NRF_CLOUD_EVT_READY`. It triggers the event :cpp:enumerator:`NRF_CLOUD_EVT_SENSOR_ATTACHED` if the execution was successful.

.. _lib_nrf_cloud_unlink:

Removing the link between device and user
*****************************************
If you want to remove the link between a device and an nRF Cloud user, you must do this from the nRF Cloud. It is not possible for a device to unlink itself.

When a user disassociates a device, the library disallows any further sensor data to be sent to the cloud and generates an :cpp:enumerator:`NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST` event. The application can then decide to associate again by responding with :cpp:func:`nrf_cloud_user_associate` with the new input sequence. See the following message sequence chart:

.. msc:
   hscale = "1.3";
   Module,Application;
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST"];
   Module<<Application      [label="nrf_cloud_user_associate()"];
   Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
   Module>>Application      [label="NRF_CLOUD_EVT_READY"];
   Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED"];


API documentation
*****************

| Header file: :file:`include/net/nrf_cloud.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud
   :project: nrf
   :members:

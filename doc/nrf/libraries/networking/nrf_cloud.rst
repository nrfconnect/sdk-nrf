.. _lib_nrf_cloud:

nRF Cloud
#########

.. contents::
   :local:
   :depth: 2

The nRF Cloud library enables applications to connect to Nordic Semiconductor's `nRF Cloud`_.
It abstracts and hides the details of the transport and the encoding scheme that is used for the payload and provides a simplified API interface for sending data from supported sensor types to the cloud.
The current implementation supports the following technologies:

* GNSS and FLIP sensor data
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
If the :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option is not enabled, the application should monitor the connection socket. :c:func:`nrf_cloud_connect` blocks and returns success when the MQTT connection to the cloud completes.
If the :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option is enabled, an nRF Cloud library thread monitors the connection socket. :c:func:`nrf_cloud_connect` does not block and returns success if the connection monitoring thread has started.
When :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` is enabled, an additional event, :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECTING`, is sent to the application.
The status field of :c:struct:`nrf_cloud_evt` contains the connection status that is defined by :c:enumerator:`nrf_cloud_connect_result`.
The event :c:enumerator:`NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED` also contains additional information in the status field that is defined by :c:enumerator:`nrf_cloud_disconnect_status`.

First, the library tries to establish the transport for communicating with the cloud.
This procedure involves a TLS handshake that might take up to three seconds.
The API blocks for the duration of the handshake.

The cloud uses the certificates of the device for authentication.
See :ref:`nrf9160_ug_updating_cloud_certificate` and the :ref:`modem_key_mgmt` library for more information on modem credentials.
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

* :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI` - If you enable this option, the ID is automatically generated using a prefix and the modem's IMEI (``<prefix><IMEI>``). You can configure the prefix by using :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX`. The default format of the prefix is ``nrf-`` and it is valid only for Nordic devices such as Thingy:91 or nRF9160 DK. For custom hardware, use a prefix other than ``nrf-`` by modifying :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX`.

* :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID` - If you enable this option, the ID is automatically generated using the modem's 128-bit internal UUID, which results in a 32-character string with no hyphens. This option requires modem firmware v1.3.0 or higher.

* :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME` - If you enable this option, the ID is set at compile time using the value specified by :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID`.

* :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` - If you enable this option, the ID is set at run time. If the nRF Cloud library is used directly, set the NULL-terminated ID string in :c:struct:`nrf_cloud_init_param` when calling :c:func:`nrf_cloud_init`.

.. _lib_nrf_cloud_fota:

Firmware over-the-air (FOTA) updates
************************************
The nRF Cloud library supports FOTA updates for your nRF9160-based device.
The :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` option is enabled by default when :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` is set.
This enables FOTA functionality in the application.

nRF Cloud FOTA enables the following additional features and libraries:

* :kconfig:option:`CONFIG_FOTA_DOWNLOAD` enables :ref:`lib_fota_download`
* :kconfig:option:`CONFIG_DFU_TARGET` enables :ref:`lib_dfu_target`
* :kconfig:option:`CONFIG_DOWNLOAD_CLIENT` enables :ref:`lib_download_client`
* :kconfig:option:`CONFIG_FOTA_DOWNLOAD_PROGRESS_EVT`
* :kconfig:option:`CONFIG_REBOOT`
* :kconfig:option:`CONFIG_CJSON_LIB`

For FOTA updates to work, the device must provide the information about the supported FOTA types to nRF Cloud.
The device passes this information by writing a ``fota_v2`` field containing an array of FOTA types into the ``serviceInfo`` field in the device's shadow.
:c:func:`nrf_cloud_service_info_json_encode` can be used to generate the proper JSON data to enable FOTA.
Additionally, :c:func:`nrf_cloud_shadow_device_status_update` can be used to generate the JSON data and perform the shadow update.

Following are the three supported FOTA types:

* ``APP``
* ``MODEM``
* ``BOOT``
* ``MDM_FULL``

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
               "MDM_FULL",
               "BOOT"
               ]
   }}}}}

You can initiate FOTA updates through `nRF Cloud`_ or by using the `nRF Cloud Device API`_.
When the device receives the :c:enumerator:`NRF_CLOUD_EVT_FOTA_DONE` event, the application must perform any necessary cleanup and perform a reboot to complete the update.
The message payload of the :c:enumerator:`NRF_CLOUD_EVT_FOTA_DONE` event contains the :c:enum:`nrf_cloud_fota_type` value.
If the value equals :c:enumerator:`NRF_CLOUD_FOTA_MODEM_DELTA`, the application can optionally avoid a reboot by reinitializing the modem library and then calling the :c:func:`nrf_cloud_modem_fota_completed` function.

Building FOTA images
====================
The build system will create the files :file:`dfu_application.zip` and/or :file:`dfu_mcuboot.zip` for a properly configured application.
See :ref:`app_build_fota` for more information about FOTA zip files.

When you use the files :file:`dfu_application.zip` or :file:`dfu_mcuboot.zip` to create an update bundle, the `nRF Cloud`_ UI populates the ``Name`` and ``Version`` fields from the :file:`manifest.json` file contained within.
However, you are free to change them as needed.
The UI populates the ``Version`` field from only the |NCS| version field in the :file:`manifest.json` file.

Alternatively, you can use the :file:`app_update.bin` file to create an update bundle, but you need to enter the ``Name`` and ``Version`` fields manually.
See `nRF Cloud Getting Started FOTA documentation`_ to learn how to create an update bundle.

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

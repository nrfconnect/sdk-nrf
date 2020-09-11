.. _lib_azure_fota:

Azure FOTA
##########

The Azure firmware over-the-air (Azure FOTA) library provides a way to parse `Azure IoT Hub device twin messages`_ to obtain firmware upgrade information and perform FOTA downloads.
This library is used by the :ref:`azure_fota_sample` sample.

The JSON format and content used in this library are based on the recommendations in `Azure Firmware Over The Air (FOTA)`_.
The library processes the incoming JSON string and searches for a specific object in the device twin that contains the firmware upgrade information.
The expected format of a ``firmware`` object is as shown below:

.. parsed-literal::
   :class: highlight

   {
   	"firmware": {
		"fwVersion": "v1.0.3",
		"fwLocation": {
       			"protocol": "https:",
			"host": "<hostname>",
			"path": "<filename>.bin"
		},
		"fwFragmentSize": 1800
	}
   }

The library uses ``fwVersion`` to determine whether the firmware should be downloaded or not.
Currently, a new version of the firmware is downloaded and applied only if the version is different from the current firmware version.

Currently, the library does not use the ``protocol`` field.
Instead, the :option:`CONFIG_AZURE_FOTA_TLS` option is used at compile time to specify if HTTPS should be used as the transport protocol.

The ``fwFragmentSize`` field specifies the maximum fragment size for the file that should be downloaded in each HTTP request.
Below are the maximum total fragment sizes in different scenarios:

* With TLS: 2 KB including the HTTP header
* Without TLS: 4 KB

``fwFragmentSize`` should therefore be set to a value lower than the maximum buffer size to reserve space for the HTTP header.

It is up to the application that uses the library to restart the device when the FOTA completes and an :cpp:enumerator:`AZURE_FOTA_EVT_DONE <azure_fota::AZURE_FOTA_EVT_DONE>` event is received.

.. note::
   The current implementation is experimental and may be enhanced in future releases.

Configuration
*************

Configure the following parameters when using this library:

* :option:`CONFIG_AZURE_FOTA_APP_VERSION` - Defines the application version string. Indicates the current firmware version on the development kit.
* :option:`CONFIG_AZURE_FOTA_APP_VERSION_AUTO` - Automatically generates the application version. If enabled, :option:`CONFIG_AZURE_FOTA_APP_VERSION` is ignored.
* :option:`CONFIG_AZURE_FOTA_TLS` - Enables HTTPS for downloads. By default, TLS is enabled and currently, the transport protocol must be configured at compile time.
* :option:`CONFIG_AZURE_FOTA_SEC_TAG` - Sets the security tag for TLS credentials when using HTTPS as the transport layer. See :ref:`azure_iot_hub_flash_certs` for more details.
* :option:`CONFIG_AZURE_FOTA_HOSTNAME_MAX_LEN` - Sets the host name buffer size.
* :option:`CONFIG_AZURE_FOTA_FILE_PATH_MAX_LEN` - Sets the file path buffer size.


Limitations
***********

The library requires a ``Content-Range`` header to be present in the HTTP response from the server.
This limitation is inherited from the :ref:`lib_download_client` library.

API documentation
*****************

| Header file: :file:`include/net/azure_fota.h`
| Source files: :file:`subsys/net/lib/azure_fota/`

.. doxygengroup:: azure_fota
   :project: nrf
   :members:

.. _lib_azure_fota:

Azure FOTA
##########

.. contents::
   :local:
   :depth: 2

The Azure firmware over-the-air (Azure FOTA) library provides a way to parse `Azure IoT Hub device twin messages`_ to obtain firmware upgrade information and perform FOTA downloads.
This library is used by the :ref:`azure_iot_hub` sample.

The JSON format and content used in this library are based on the recommendations in `Azure Firmware Over The Air (FOTA)`_.
The library processes the incoming JSON string and searches for a specific object in the device twin that contains the firmware upgrade information.
See :ref:`lib_azure_fota_firmware_object` for the format of this object.

It is up to the application that uses the library to restart the device when the FOTA completes and an :c:enumerator:`AZURE_FOTA_EVT_DONE` event is received.

.. note::
   The current implementation is :ref:`experimental <software_maturity>`.

.. _lib_azure_fota_firmware_object:

Firmware object
***************

To start a FOTA update, add a ``firmware`` object to the *desired* properties of the device twin, as shown in the following example:

.. parsed-literal::
   :class: highlight

   {
       "firmware": {
           "fwVersion": "0.0.2-dev",
           "jobId": "ca186d4b-4171-4209-a49d-700c35567d1d",
           "fwLocation": {
               "host": "<storage_account>.blob.core.windows.net",
               "path": "<container>/<blob>?<sas_token>"
           },
           "fwFragmentSize": 1800
       }
   }

The object has the following fields:

``fwVersion``
   The version of the update image, which is the value of the :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION` Kconfig option in the update image.
   The library downloads and applies the image only if this version is different from the current firmware version.
   The length must be less than the value of the :kconfig:option:`CONFIG_AZURE_FOTA_VERSION_MAX_LEN` Kconfig option.

``jobId``
   A unique ID of the update, for example, a UUID (RFC 4122).
   Use a new job ID for each update, because the library does not start a job with a job ID that it has already attempted.
   The length must be less than the value of the :kconfig:option:`CONFIG_AZURE_FOTA_JOB_ID_MAX_LEN` Kconfig option.

``fwLocation.host``
   The host name of the file server, without ``https://``.
   For Azure Blob Storage, the host name is ``<storage_account>.blob.core.windows.net``.
   The length must be less than the value of the :kconfig:option:`CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE` Kconfig option.

``fwLocation.path``
   The path of the file on the server, without a leading ``/``.
   The device downloads the file without authentication, so the file must allow anonymous read access, or the path must include a shared access signature (SAS).
   For a blob in a private Azure Blob Storage container, create a SAS token with read permission for the blob, as described in `Azure shared access signatures`_.
   The path is then ``<container>/<blob>?<sas_token>``.
   The length must be less than the value of the :kconfig:option:`CONFIG_DOWNLOADER_MAX_FILENAME_SIZE` Kconfig option.

``fwLocation.protocol``
   The library does not use this field.
   Instead, the :kconfig:option:`CONFIG_AZURE_FOTA_TLS` Kconfig option sets at compile time if the download uses HTTPS.

``fwFragmentSize``
   The maximum size of the file fragment that the library downloads in each HTTP request.
   This field is required.
   The maximum total fragment sizes are the following:

   * With TLS: 2 KB including the HTTP header
   * Without TLS: 4 KB

   Set ``fwFragmentSize`` to a value lower than the maximum buffer size to leave space for the HTTP header.

Configuration
*************

Configure the following parameters when using this library:

* :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION` - Defines the application version string. Indicates the current firmware version on the development kit.
* :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION_AUTO` - Automatically generates the application version. If enabled, :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION` is ignored.
* :kconfig:option:`CONFIG_AZURE_FOTA_TLS` - Enables HTTPS for downloads. By default, TLS is enabled and currently, the transport protocol must be configured at compile time.
* :kconfig:option:`CONFIG_AZURE_FOTA_SEC_TAG` - Sets the security tag for TLS credentials when using HTTPS as the transport layer. See :ref:`azure_iot_hub_flash_certs` for more details.

Additionally, configure the :ref:`lib_downloader` library:

* :kconfig:option:`CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE` - Sets the maximum length of the host name for the library.
* :kconfig:option:`CONFIG_DOWNLOADER_MAX_FILENAME_SIZE` - Sets the maximum length of the file name for the library.

Limitations
***********

The library requires a ``Content-Range`` header to be present in the HTTP response from the server.
This limitation is inherited from the :ref:`lib_downloader` library.

API documentation
*****************

| Header file: :ncs-file:`/include/net/azure_fota.h`
| Source files: :file:`subsys/net/lib/azure_fota/`

.. doxygengroup:: azure_fota

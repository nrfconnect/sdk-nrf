.. _lib_aws_fota:

AWS FOTA
########

The Amazon Web Services firmware over-the-air (AWS FOTA) library combines the :ref:`lib_aws_jobs` and :ref:`lib_fota_download` libraries to create a user-friendly library that can perform an over-the-air firmware update using HTTP and MQTT TLS.

It connects to the specified broker using the existing or given certificates and uses `TLS`_ for the MQTT connection.
This means that the data sent in each MQTT message is encrypted.

Note that other devices that are connected to the same AWS MQTT broker receive the same messages if:

* The other device has valid (but different) certificates that use the same AWS IoT policy as the original device.
* The other device is subscribed to the same MQTT topic as the original device.

The library uses MQTT to receive notification that an update is available, and to retrieve metadata about the update.
It uses HTTP to download the update payload.

It is up to the application that uses the library to restart the device when the FOTA is complete.

The AWS FOTA library is used in the :ref:`aws_fota_sample` sample.

Configuration
*************

Configure the following parameters when using this library:

- :option:`CONFIG_AWS_FOTA_PAYLOAD_SIZE`
- :option:`CONFIG_AWS_FOTA_HOSTNAME_MAX_LEN`
- :option:`CONFIG_AWS_FOTA_FILE_PATH_MAX_LEN`


Implementation
**************

The following sequence diagram shows how a firmware over-the-air update is implemented through the use of `AWS IoT MQTT`_, `AWS IoT jobs`_, and `AWS Simple Storage Service (S3)`_.

.. figure:: /images/aws_fota_dfu_sequence.svg
   :alt: AWS FOTA sequence diagram for doing FOTA through AWS jobs


.. include:: /includes/aws_s3_bucket.txt


AWS IoT jobs
============

The implementation uses a job document similar to the following (where *bucket_name* is the name of your bucket and *file_name* is the name of your file) for passing information from `AWS IoT jobs`_ to the device:

.. parsed-literal::
   :class: highlight

   {
     "operation": "app_fw_update",
     "fwversion": "v1.0.2",
     "size": 181124,
     "location": {
       "protocol": "http:",
       "host": "*bucket_name*.amazonaws.com",
       "path": "*file_name*.bin"
      }
   }

The current implementation uses information from the ``host`` and ``path`` fields only.


Limitations
***********

* Currently, the library uses HTTP for downloading the firmware.
  To use HTTPS instead, apply the changes described in :ref:`the HTTPS section of the download client documentation <download_client_https>` to the :ref:`lib_fota_download` library.
* The library requires a Content-Range header to be present in the HTTP response from the server.
  This limitation is inherited from the :ref:`lib_download_client` library.

API documentation
*****************

| Header file: :file:`include/net/aws_fota.h`
| Source files: :file:`subsys/net/lib/aws_fota/`

.. doxygengroup:: aws_fota
   :project: nrf
   :members:

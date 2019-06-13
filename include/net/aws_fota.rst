.. _lib_aws_fota:

AWS FOTA
########

This library combines the :ref:`lib_aws_jobs` and :ref:`lib_fota_download` libraries. To create user-friendly
library that can do a Firmware-over-the-air(FOTA) update through the use of HTTP and MQTT TLS.

It connects to the specified broker using the existing or given certificates and uses a `TLS <https://www.ietf.org/rfc/rfc5246.txt>`_
connection for the MQTT connection, this means that the data sent in each MQTT message is encrypted. It should be noted that a device connected to the AWS MQTT broker with a valid but different certificate that has the same security attributes in AWS as the other device and is able to subscribe to the same MQTT topic as the other device will also receive the message sent to that topic.

MQTT is used to receive notification that an update is available, and to retrieve metadata about the update.
HTTP is used to download the update payload.

It's up to the application that uses the library to restart when the FOTA is complete.

Configuration
*************

Use Kconfig to configure the MQTT payload buffer sizes and the buffers used to store the version string, hostname, and file path.


Implementation
**************
The implementation uses the job document shown below for passing information from AWS Jobs to the device:

.. code-block:: javascript

   {
     "operation": "app_fw_update",
     "fwversion": "v1.0.2",
     "size": 181124,
     "location": {
       "protocol": "http:",
       "host": "s3.amazonaws.com",
       "path": "/nordic-firmware-files/0943dfbf-cb10-4eb7-8277-a8b179eaf4ff"
      }
   }

The current implementation only uses information from `host` and `path` in this document while the other fields are intended for use in coming features.

The following sequence diagram shows how a FOTA is implemented through the use of `AWS IoT Jobs <https://docs.aws.amazon.com/iot/latest/developerguide/iot-jobs.html>`_, `AWS IoT MQTT<https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html>`_ and `AWS S3<https://docs.aws.amazon.com/s3/index.html> in this library`_.

.. |AWS FOTA sequence diagram| image:: aws_fota_dfu_sequence.png
  :alt: Sequence diagram for doing FOTA through AWS Jobs



Limitations
***********
* Currently only uses of HTTP for downloading the firmware, but changes can be made to have it work with HTTPS see :ref:`lib_download_client`. These changes need to be applied to :ref:`lib_fota_download` to enable downloading firmware through https.
* Requires Content-range header to be present in the HTTP response from the server this limitation is inherited from :ref:`lib_download_client` which is used by :ref:`lib_fota_download`.

API documentation
*****************

| Header file: :file:`include/aws_fota.h`
| Source files: :file:`subsys/net/lib/aws_fota/`

.. doxygengroup:: aws_fota
   :project: nrf
   :members:

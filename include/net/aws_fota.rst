.. _lib_aws_fota:

AWS FOTA
########

This library combines the :ref:`lib_aws_jobs` and :ref:`lib_fota_download` libraries.

MQTT is used to receive notification that an update is available, and to retrieve metadata about the update.
HTTP is used to download the update payload.

API documentation
*****************

| Header file: :file:`include/aws_fota.h`
| Source files: :file:`subsys/net/lib/aws_fota/`

.. doxygengroup:: aws_fota
   :project: nrf
   :members:
